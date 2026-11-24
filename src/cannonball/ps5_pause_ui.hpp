// CannonBall-PS5 Beta 1B: small platform-independent pause-menu state machine.
// Separate original work; no copyrighted OutRun assets or ROMs.
#pragma once
#include <cstdint>

class Ps5PauseUI
{
public:
    enum Button { NONE, TOUCHPAD, UP, DOWN, LEFT, RIGHT, ACCEPT, BACK };
#ifdef CANNONBALL_PS5_BETA1B
    enum Action { NO_ACTION, RESTART_GAME, RETURN_TO_MENU, TOGGLE_FPS, TOGGLE_ATTRACT, TOGGLE_FREEPLAY, SAVE_SETTINGS };
    enum Screen { MAIN, SETTINGS, CONFIRM_RESTART, CONFIRM_MENU };
#else
    enum Action { NO_ACTION, RESTART_GAME, RETURN_TO_MENU };
    enum Screen { MAIN, SETTINGS_NOTICE, CONFIRM_RESTART, CONFIRM_MENU };
#endif

    Ps5PauseUI();
    bool active() const { return active_; }
    int selection() const { return selected_; }
    Screen screen() const { return screen_; }
    bool confirm_yes() const { return confirm_yes_; }
    Action press(Button);
    void reset();
    #ifdef CANNONBALL_PS5_BETA1B
    // Called on entry to the pause screen to reflect live Cannonball values.
    void sync_settings(bool fps, bool attract, bool freeplay);
    void set_save_result(bool ok);
    bool fps_visible() const { return fps_visible_; }
    bool attract_music() const { return attract_music_; }
    bool freeplay() const { return freeplay_; }
    #endif
    // Draw over the already-converted 32-bit game framebuffer.
    // All colour values must already be packed for the target SDL pixel format.
    void draw(uint32_t* dst, int width, int height, int stride,
              uint32_t panel, uint32_t heading, uint32_t highlight,
              uint32_t white, uint32_t muted, uint32_t black) const;
private:
    bool active_;
    int selected_;
    Screen screen_;
    bool confirm_yes_;
    #ifdef CANNONBALL_PS5_BETA1B
    bool fps_visible_;
    bool attract_music_;
    bool freeplay_;
    int save_result_; // 0 = no message, 1 = saved, -1 = failed
    #endif

};

extern Ps5PauseUI ps5_pause_ui;
