#ifdef CANNONBALL_PS5_BETA1A
#include "ps5_pause_ui.hpp"
#include <cstdio>
#endif
/***************************************************************************
    Cannonball Main Entry Point.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <cstring>
#include <iostream>
#ifdef CANNONBALL_PS5
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef CANNONBALL_PS5_NATIVE
#include "ps5_native_storage.hpp"
#endif

// SDL Library
#include <SDL.h>

// SDL Specific Code
#include "sdl2/timer.hpp"
#include "sdl2/input.hpp"

#include "video.hpp"

#include "romloader.hpp"
#include "trackloader.hpp"
#include "stdint.hpp"
#include "main.hpp"
#include "engine/outrun.hpp"
#include "frontend/config.hpp"
#include "frontend/menu.hpp"

#include "engine/oinputs.hpp"
#include "engine/ooutputs.hpp"
#include "engine/omusic.hpp"
#ifdef CANNONBALL_PS5_BETA1C2
#include "engine/ohud.hpp"
#endif

// Direct X Haptic Support.
// Fine to include on non-windows builds as dummy functions used.
#include "directx/ffeedback.hpp"

// ------------------------------------------------------------------------------------------------
// Initialize Shared Variables
// ------------------------------------------------------------------------------------------------
using namespace cannonball;

int    cannonball::state       = STATE_BOOT;
double cannonball::frame_ms    = 0;
int    cannonball::frame       = 0;
bool   cannonball::tick_frame  = true;
int    cannonball::fps_counter = 0;

// ------------------------------------------------------------------------------------------------
// Main Variables and Pointers
// ------------------------------------------------------------------------------------------------
Audio cannonball::audio;
Menu* menu;
bool pause_engine;


// ------------------------------------------------------------------------------------------------

#ifdef CANNONBALL_PS5_NATIVE
// CBPS5_SYSTEM_EXIT_V1
// Native PS5 titles request closure through the system shell.
// Do not terminate through the C runtime or run exit-time destructors.

extern "C" int sceSystemServiceLoadExec(
    const char* path, const char** argv);
extern "C" int sceKernelUsleep(unsigned int microseconds);

[[noreturn]] static void ps5_system_exit()
{
    (void)sceSystemServiceLoadExec("exit", nullptr);

    // The shell owns application termination. If the request
    // returns, remain idle until the system closes the title.
    for (;;)
        (void)sceKernelUsleep(100000);
}
#endif

static void quit_func(int code)
{
    audio.stop_audio();
    input.close_joy();
    forcefeedback::close();
    delete menu;
    menu = nullptr;
    SDL_Quit();

#ifdef CANNONBALL_PS5_NATIVE
    (void)code;
    ps5_system_exit();
#else
    exit(code);
#endif
}

#ifdef CANNONBALL_PS5_BETA1A
// DualSense touchpad click is SDL_CONTROLLER_BUTTON_TOUCHPAD in the PS5 SDL2
// backend. Options stays SDL_CONTROLLER_BUTTON_START (arcade Start).


static bool ps5_beta1a_input_event(const SDL_Event& e)
{
    switch (e.type) {
        case SDL_CONTROLLERBUTTONDOWN: case SDL_CONTROLLERBUTTONUP:
        case SDL_CONTROLLERAXISMOTION:
        case SDL_JOYBUTTONDOWN: case SDL_JOYBUTTONUP:
        case SDL_JOYAXISMOTION: case SDL_JOYHATMOTION:
        case SDL_KEYDOWN: case SDL_KEYUP:
            return true;
        default:
            return false;
    }
}

static bool ps5_beta1a_handle_event(const SDL_Event& e)
{
    const bool was_paused = ps5_pause_ui.active();
    Ps5PauseUI::Button b = Ps5PauseUI::NONE;
    bool is_down = false;
    bool button_event = false;

    if (e.type == SDL_CONTROLLERBUTTONDOWN || e.type == SDL_CONTROLLERBUTTONUP) {
        button_event = true;
        is_down = e.type == SDL_CONTROLLERBUTTONDOWN;
        switch (e.cbutton.button) {
            case SDL_CONTROLLER_BUTTON_TOUCHPAD: b = Ps5PauseUI::TOUCHPAD; break;
            case SDL_CONTROLLER_BUTTON_DPAD_UP: b = Ps5PauseUI::UP; break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN: b = Ps5PauseUI::DOWN; break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT: b = Ps5PauseUI::LEFT; break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: b = Ps5PauseUI::RIGHT; break;
            case SDL_CONTROLLER_BUTTON_A: b = Ps5PauseUI::ACCEPT; break; // Cross
            case SDL_CONTROLLER_BUTTON_B: b = Ps5PauseUI::BACK; break;   // Circle
            default: break; // Options remains mapped to arcade Start.
        }
    } else if ((e.type == SDL_JOYBUTTONDOWN || e.type == SDL_JOYBUTTONUP) &&
               e.jbutton.button == 4 && !SDL_IsGameController(0)) {
        // Fallback if the PS5 pad is recognized as a bare joystick, not a GameController.
        button_event = true;
        is_down = e.type == SDL_JOYBUTTONDOWN;
        b = Ps5PauseUI::TOUCHPAD;
    } else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
        button_event = true;
        is_down = e.type == SDL_KEYDOWN;
        switch (e.key.keysym.sym) {
            case SDLK_F1: b = Ps5PauseUI::TOUCHPAD; break; // keyboard pause fallback
            case SDLK_UP: b = Ps5PauseUI::UP; break;
            case SDLK_DOWN: b = Ps5PauseUI::DOWN; break;
            case SDLK_LEFT: b = Ps5PauseUI::LEFT; break;
            case SDLK_RIGHT: b = Ps5PauseUI::RIGHT; break;
            case SDLK_RETURN: case SDLK_SPACE: b = Ps5PauseUI::ACCEPT; break;
            case SDLK_ESCAPE: case SDLK_BACKSPACE: b = Ps5PauseUI::BACK; break;
            default: break;
        }
        if (e.type == SDL_KEYDOWN && e.key.repeat && b == Ps5PauseUI::TOUCHPAD)
            return true; // never auto-repeat pause toggles
    }

    // While playing, ONLY the touchpad is intercepted. Normal DualSense
    // gameplay buttons (including Options = Start) continue to Cannonball.
    if (!was_paused && (state != STATE_GAME || b != Ps5PauseUI::TOUCHPAD))
        return false;

    if (button_event && b != Ps5PauseUI::NONE) {
        if (is_down) {
#ifdef CANNONBALL_PS5_BETA1B
            // Sync live config just before entering the pause menu.
            if (b == Ps5PauseUI::TOUCHPAD && !was_paused)
                ps5_pause_ui.sync_settings(config.video.fps_count != 0,
                                            config.sound.advertise != 0,
                                            config.engine.freeplay);
#endif
            const Ps5PauseUI::Action action = ps5_pause_ui.press(b);
            const bool paused = ps5_pause_ui.active();
            if (paused != was_paused) {
                audio.ps5_beta1a_pause(paused);
            }
#ifdef CANNONBALL_PS5_BETA1B
            if (action == Ps5PauseUI::TOGGLE_FPS ||
                action == Ps5PauseUI::TOGGLE_ATTRACT ||
                action == Ps5PauseUI::TOGGLE_FREEPLAY ||
                action == Ps5PauseUI::SAVE_SETTINGS) {
                if (action == Ps5PauseUI::TOGGLE_FPS)
                    config.video.fps_count = ps5_pause_ui.fps_visible() ? 1 : 0;
                else if (action == Ps5PauseUI::TOGGLE_ATTRACT)
                    config.sound.advertise = ps5_pause_ui.attract_music() ? 1 : 0;
                else if (action == Ps5PauseUI::TOGGLE_FREEPLAY)
                    config.engine.freeplay = ps5_pause_ui.freeplay();
                // Save immediately to avoid losing options on an hbldr crash.
#ifdef CANNONBALL_PS5_BETA1C2
                if (action == Ps5PauseUI::TOGGLE_FPS && !config.video.fps_count)
                {
                    // FPS tiles persist in game text RAM while paused.
                    fps_counter = 0;
                    const uint32_t fps_addr = ohud.translate(30, 0);
                    for (uint32_t i = 0; i < 10; ++i)
                        video.write_text16(fps_addr + i * 2, 0);
                }
#endif
                const bool saved = config.save();
                ps5_pause_ui.set_save_result(saved);
            }
#endif
            if (action == Ps5PauseUI::RESTART_GAME) {
                state = STATE_INIT_GAME;
            } else if (action == Ps5PauseUI::RETURN_TO_MENU) {
                state = STATE_INIT_MENU;
            }
        }
        return true;
    }
    // Block unrecognized gamepad input while paused to prevent it being
    // accidentally applied to gameplay when the player resumes.
    return was_paused && ps5_beta1a_input_event(e);
}
#endif

static void process_events(void)
{
    SDL_Event event;

    // Grab all events from the queue.
    #ifdef CANNONBALL_PS5_NATIVE
    SDL_JoystickUpdate();

    while (SDL_PeepEvents(
        &event, 1, SDL_GETEVENT,
        SDL_FIRSTEVENT, SDL_LASTEVENT) > 0)
#else
    while (SDL_PollEvent(&event))
#endif
    {
#ifdef CANNONBALL_PS5_BETA1A
        if (ps5_beta1a_handle_event(event)) continue;
#endif
        switch(event.type)
        {
            case SDL_KEYDOWN:
                // Handle key presses.
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    state = STATE_QUIT;
                else
                    input.handle_key_down(&event.key.keysym);
                break;

            case SDL_KEYUP:
                input.handle_key_up(&event.key.keysym);
                break;

            case SDL_JOYAXISMOTION:
                input.handle_joy_axis(&event.jaxis);
                break;

            case SDL_JOYBUTTONDOWN:
                input.handle_joy_down(&event.jbutton);
                break;

            case SDL_JOYBUTTONUP:
                input.handle_joy_up(&event.jbutton);
                break;

            case SDL_CONTROLLERAXISMOTION:
                input.handle_controller_axis(&event.caxis);
                break;

            case SDL_CONTROLLERBUTTONDOWN:
                input.handle_controller_down(&event.cbutton);
                break;

            case SDL_CONTROLLERBUTTONUP:
                input.handle_controller_up(&event.cbutton);
                break;

            case SDL_JOYHATMOTION:
                input.handle_joy_hat(&event.jhat);
                break;

            case SDL_JOYDEVICEADDED:
                input.open_joy();
                break;

            case SDL_JOYDEVICEREMOVED:
                input.close_joy();
                break;

            case SDL_QUIT:
                // Handle quit requests (like Ctrl-c).
                state = STATE_QUIT;
                break;
        }
    }
}

static void tick()
{
    frame++;

    // Non standard FPS: Determine whether to tick certain logic for the current frame.
    if (config.fps == 60)
        tick_frame = frame & 1;
    else if (config.fps == 120)
        tick_frame = (frame & 3) == 1;

    process_events();
#ifdef CANNONBALL_PS5_BETA1A
    if (ps5_pause_ui.active() && state == STATE_GAME) {
        input.frame_done();
        return; // No OutRun logic, physics, countdown or sound-chip tick.
    }
#endif

    if (tick_frame)
    {
        oinputs.tick();           // Do Controls
        oinputs.do_gear();        // Digital Gear
    }
     
    switch (state)
    {
        case STATE_GAME:
        {
            if (tick_frame)
            {
                if (input.has_pressed(Input::TIMER)) outrun.freeze_timer = !outrun.freeze_timer;
                if (input.has_pressed(Input::PAUSE)) pause_engine = !pause_engine;
                if (input.has_pressed(Input::MENU))  state = STATE_INIT_MENU;
            }

            if (!pause_engine || input.has_pressed(Input::STEP))
            {
                outrun.tick(tick_frame);
                if (tick_frame) input.frame_done();
                osoundint.tick();
            }
            else
            {                
                if (tick_frame) input.frame_done();
            }
        }
        break;

        case STATE_INIT_GAME:
            if (config.engine.jap && !roms.load_japanese_roms())
            {
                state = STATE_QUIT;
            }
            else
            {
                tick_frame = true;
                pause_engine = false;
                outrun.init();
                state = STATE_GAME;
            }
            break;

        case STATE_MENU:
            menu->tick();
            input.frame_done();
            osoundint.tick();
            break;

        case STATE_INIT_MENU:
            oinputs.init();
            outrun.outputs->init();
            menu->init();
            state = STATE_MENU;
            break;
    }

    // Map OutRun outputs to CannonBall devices (SmartyPi Interface / Controller Rumble)
    outrun.outputs->writeDigitalToConsole();
    if (tick_frame)
    {
         input.set_rumble(outrun.outputs->is_set(OOutputs::D_MOTOR), config.controls.rumble);
    }
}



static void main_loop()
{
    // FPS Counter (If Enabled)
    Timer fps_count;
    int frame = 0;
    fps_count.start();

    // General Frame Timing
    bool vsync = config.video.vsync == 1 && video.supports_vsync();
    Timer frame_time;
    int t;                              // Actual timing of tick in ms as measured by SDL (ms)
    double deltatime  = 0;              // Time we want an entire frame to take (ms)
    int deltaintegral = 0;              // Integer version of above

    while (state != STATE_QUIT)
    {
        frame_time.start();
        // Tick Engine
        tick();

#ifdef CANNONBALL_PS5_NATIVE
        // Do not render or queue audio after an exit request.
        if (state == STATE_QUIT)
            break;
#endif

        // Draw SDL Video
        video.prepare_frame();
        video.render_frame();

        // Fill SDL Audio Buffer For Callback
#ifdef CANNONBALL_PS5_BETA1A
        if (!ps5_pause_ui.active())
#endif
        audio.tick();
        
        // Calculate Timings. Cap Frame Rate. Note this might be trumped by V-Sync
        if (!vsync)
        {
#ifdef CANNONBALL_PS5_BETA1A
            // While the audio device is paused, use normal video pacing.
            deltatime += frame_ms * (ps5_pause_ui.active() ? 1.0 : audio.adjust_speed());
#else
            deltatime += (frame_ms * audio.adjust_speed());
#endif
            deltaintegral = (int)deltatime;
            t = frame_time.get_ticks();
            
            if (t < deltatime)
                SDL_Delay((Uint32)(deltatime - t));

            deltatime -= deltaintegral;
        }

#ifdef CANNONBALL_PS5_BETA1B
        // Count continuously even when hidden. This makes live toggles accurate.
        if (true)
        {
#else
        if (config.video.fps_count)
        {
#endif
            frame++;
            // One second has elapsed
            if (fps_count.get_ticks() >= 1000)
            {
                fps_counter = frame;
                frame       = 0;
                fps_count.start();
            }
        }
    }

    quit_func(0);
}

// Very (very) simple command line parser.
// Returns true if everything is ok to proceed with launching th engine.
static bool parse_command_line(int argc, char* argv[])
{
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-cfgfile") == 0 && i+1 < argc)
        {
            config.set_config_file(argv[i+1]);
        }
        else if (strcmp(argv[i], "-file") == 0 && i+1 < argc)
        {
            if (!trackloader.set_layout_track(argv[i+1]))
                return false;
        }
        else if (strcmp(argv[i], "-help") == 0)
        {
            std::cout << "Command Line Options:\n\n" <<
                         "-cfgfile: Location and name of config.xml\n" <<
                         "-file   : LayOut Editor track data to load\n" << std::endl;
            return false;
        }
    }
    return true;
}


#ifdef CANNONBALL_PS5_NATIVE
extern "C" int sceSystemServiceHideSplashScreen(void);

// Record genuine native startup failures only.
static void native_error_log(const char* message)
{
    FILE* f = std::fopen(
        "/download0/cannonball-native-game.log", "a");

    if (f)
    {
        std::fprintf(f, "%s\n", message);
        std::fflush(f);
        std::fclose(f);
    }
}

static int native_fail(const char* message, int stage)
{
    native_error_log(message);

    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) != 0)
        return 1;

    SDL_Window* w = SDL_CreateWindow(
        "CannonBall Startup Failure",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        1920, 1080, SDL_WINDOW_FULLSCREEN);

    if (!w) return 1;

    SDL_Surface* screen = SDL_GetWindowSurface(w);

    if (!screen) return 1;

    SDL_FillRect(screen, NULL,
        SDL_MapRGB(screen->format, 150, 15, 15));

    for (int n = 0; n < stage; ++n)
    {
        SDL_Rect box = {
            120 + n * 180, 480, 140, 120
        };

        SDL_FillRect(screen, &box,
            SDL_MapRGB(screen->format, 255, 255, 255));
    }

    if (SDL_UpdateWindowSurface(w) == 0)
    {
        sceSystemServiceHideSplashScreen();
        SDL_UpdateWindowSurface(w);

        while (true)
            SDL_Delay(1000);
    }

    return 1;
}
#endif

int main(int argc, char* argv[])
{

#if defined(CANNONBALL_PS5_NATIVE)
    // Native titles mount the read-only application under /app0 and provide
    // writable persistent state under /download0 (if download data is mounted).
    
    // Verify the packaged configuration is readable.
    // Keep the native application working directory unchanged.

    FILE* config_probe =
        std::fopen("/app0/config.xml", "rb");

    if (!config_probe)
        return native_fail("APP0 CONFIG UNREADABLE", 1);

    std::fclose(config_probe);



    if (!ps5_native::seed_config(
            "/app0/config.xml",
            "/download0/config.xml"))
        return native_fail("CONFIG SEED FAILED", 2);

#elif defined(CANNONBALL_PS5)
    // Existing hbldr launch path: unchanged.
    const char* home = std::getenv("CANNONBALL_PS5_HOME");
    if (!home || !*home) home = "/data/homebrew/CannonBall";
    if (chdir(home) != 0)
    {
        std::fprintf(stderr, "CannonBall: chdir(%s) failed: %d\n", home, errno);
        return 1;
    }
#endif
    // Parse command line arguments (config file location, LayOut data) 
    bool ok = parse_command_line(argc, argv);
#ifdef CANNONBALL_PS5_NATIVE
    // Disallow launcher-specific arguments redirecting writes into /app0.
    config.set_config_file("/download0/config.xml");
#endif

    if (ok)
    {
        config.load();

#ifdef CANNONBALL_PS5_NATIVE
        config.data.rom_path = "/app0/roms/";
        config.data.res_path = "/app0/res/";
        config.data.save_path = "/download0/saves/";

        config.data.crc32 = 0;
        config.sound.enabled = 1;
        config.sound.fix_samples = 0;
        config.engine.jap = false;

        config.video.mode = 1;
        // CBPS5: honour saved FPS counter preference.
        config.video.scanlines = 0;
        config.video.filtering = 0;
        config.video.vsync = 0;

        config.menu.enabled = 1;

#endif
        ok = roms.load_revb_roms(config.sound.fix_samples);

    }
    if (!ok)
    {
#ifdef CANNONBALL_PS5_NATIVE
        return native_fail("ROM LOADING FAILED", 3);
#else
        quit_func(1);
        return 0;
#endif
    }

#ifndef CANNONBALL_PS5_NATIVE
    // Load gamecontrollerdb.txt mappings
    if (SDL_GameControllerAddMappingsFromFile((config.data.res_path + "gamecontrollerdb.txt").c_str()) == -1)
        std::cout << "Unable to load controller mapping" << std::endl;

#endif

    // Initialize timer and video systems
    #ifdef CANNONBALL_PS5_NATIVE

    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) != 0)
#else
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_AUDIO |
#ifdef CANNONBALL_PS5
                 SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER
#else
                 SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC
#endif
                 ) == -1)
#endif
    {
        std::cerr << "SDL Initialization Failed: " << SDL_GetError() << std::endl;
        return 1;
    }


    // Load patched widescreen tilemaps
    if (!omusic.load_widescreen_map(config.data.res_path))
        std::cout << "Unable to load widescreen tilemaps" << std::endl;

    // Initialize SDL Video
    config.set_fps(config.video.fps);
    if (!video.init(&roms, &config.video))
    {
#ifdef CANNONBALL_PS5_NATIVE
        return native_fail("VIDEO INIT FAILED", 4);
#else
        quit_func(1);
#endif
    }

#ifdef CANNONBALL_PS5_NATIVE


    if (SDL_InitSubSystem(
            SDL_INIT_JOYSTICK |
            SDL_INIT_GAMECONTROLLER) != 0)
    {
        native_error_log("CONTROLLER INIT FAILED");
        native_error_log(SDL_GetError());
    }
    else
    {
        SDL_JoystickEventState(SDL_ENABLE);
        SDL_GameControllerEventState(SDL_ENABLE);


        const std::string mapping_file =
            config.data.res_path +
            "gamecontrollerdb.txt";

        if (SDL_GameControllerAddMappingsFromFile(
                mapping_file.c_str()) < 0)
        {
            native_error_log("CONTROLLER MAPPING LOAD FAILED");
            native_error_log(SDL_GetError());
        }
    }
#endif

    // Initialize SDL Audio
#ifndef CANNONBALL_PS5_NATIVE
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) == 0)
    {
        audio.init();
    }
    else
    {
        config.sound.enabled = 0;
    }
#endif

    state = config.menu.enabled ? STATE_INIT_MENU : STATE_INIT_GAME;

    // Initalize SDL Controls
    input.init(config.controls.pad_id,
               config.controls.keyconfig, config.controls.padconfig, 
               config.controls.analog,    config.controls.axis, config.controls.invert, config.controls.asettings);

    #ifndef CANNONBALL_PS5_NATIVE
    if (config.controls.haptic)
        config.controls.haptic = forcefeedback::init(
            config.controls.max_force,
            config.controls.min_force,
            config.controls.force_duration);
#endif
        
    // Populate menus
    menu = new Menu();
    menu->populate();

    main_loop();  // Loop until we quit the app

    // Never Reached
    return 0;
}
