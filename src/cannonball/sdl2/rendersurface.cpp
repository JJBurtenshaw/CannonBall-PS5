#include "ps5_cached_buffer.hpp"
#include <cstdio>
#ifdef CANNONBALL_PS5_BETA1A
#include "ps5_pause_ui.hpp"
#endif
/***************************************************************************
    SDL2 Hardware Surface Video Rendering.  
    
    Known Bugs:
    - Missing Scanlines

    Copyright Manuel Alfayate, Chris White.
    See license.txt for more details.
***************************************************************************/

#include <iostream>

#include "rendersurface.hpp"
#ifdef CANNONBALL_PS5_FASTBLIT
#include "ps5_fastblit.hpp"
#endif
#include "frontend/config.hpp"


#ifdef CANNONBALL_PS5_NATIVE
extern "C" int sceSystemServiceHideSplashScreen(void);

static bool native_splash_hidden = false;

static bool native_handoff()
{
    if (native_splash_hidden)
        return false;

    sceSystemServiceHideSplashScreen();
    native_splash_hidden = true;

    return true;
}
#endif

Render::Render(void)
{
#ifdef CANNONBALL_PS5_FASTBLIT
    fast_window_surface = NULL;
    fast_active = false;
#endif
}

Render::~Render(void)
{
}

bool Render::init(int src_width, int src_height,
                    int scale,
                    int video_mode,
                    int scanlines)
{
    this->src_width  = src_width;
    this->src_height = src_height;
    this->scale      = scale;
    this->video_mode = video_mode;
    this->scanlines  = scanlines;

    // Setup SDL Screen size
    if (!RenderBase::sdl_screen_size())
        return false;

    int flags = SDL_WINDOW_SHOWN;

    // In SDL2, we calculate the output dimensions, but then in draw_frame() we won't do any scaling: SDL2
    // will do that for us, using the rects passed to SDL_RenderCopy().
    // scn_* -> physical screen dimensions OR window dimensions. On FULLSCREEN MODE it has the physical screen
    //		dimensions and in windowed mode it has the window dimensions.
    // src_* -> real, internal, frame dimensions. Will ALWAYS be 320 or 398 x 224. NEVER CHANGES. 
    // corrected_scn_width_* -> output screen size for scaling.
    // In windowed mode it's the size of the window. 
   
    // --------------------------------------------------------------------------------------------
    // Full Screen Mode
    // --------------------------------------------------------------------------------------------
    if (video_mode == video_settings_t::MODE_FULL || video_mode == video_settings_t::MODE_STRETCH)
    {
	    flags |= (SDL_WINDOW_FULLSCREEN); // Set SDL flag

	    // Fullscreen window size: SDL2 ignores w and h in SDL_CreateWindow() if FULLSCREEN flag
	    // is enable, which is fine, so the window will be fullscreen of the physical videomode
	    // size, but then, if we want to preserve ratio, we need dst_width bigger than src_width.	
	    scn_width  = orig_width;
        scn_height = orig_height;

	    src_rect.w = src_width;
	    src_rect.h = src_height;
	    src_rect.x = 0;
	    src_rect.y = 0;
        
        if (video_mode == video_settings_t::MODE_FULL)
        {
            uint32_t w = (scn_width << 16) / src_width;
            uint32_t h = (scn_height << 16) / src_height;
            dst_rect.w = (src_width * std::min(w, h)) >> 16;
            dst_rect.h = (src_height * std::min(w, h)) >> 16;

            screen_xoff = scn_width - dst_rect.w;
            if (screen_xoff)
                screen_xoff = (screen_xoff / 2);
            
            screen_yoff = scn_height - dst_rect.h;
            if (screen_yoff)
                screen_yoff = (screen_yoff / 2) * scn_width;

            dst_rect.x = screen_xoff;
            dst_rect.y = screen_yoff;
        }
        else
        {
            dst_rect.x = 0;
            dst_rect.y = 0;
            dst_rect.w = scn_width;
            dst_rect.h = scn_height;
        }


        SDL_ShowCursor(false);
     }
   
    // --------------------------------------------------------------------------------------------
    // Windowed Mode
    // --------------------------------------------------------------------------------------------
    else
    {
        this->video_mode = video_settings_t::MODE_WINDOW;
       
        scn_width  = src_width  * scale;
        scn_height = src_height * scale;

	    src_rect.w = src_width;
	    src_rect.h = src_height;
	    src_rect.x = 0;
	    src_rect.y = 0;
	    dst_rect.w = scn_width;
	    dst_rect.h = scn_height;
	    dst_rect.x = 0;
	    dst_rect.y = 0;

        SDL_ShowCursor(true);
    }

    //int bpp = info->vfmt->BitsPerPixel;
    const int bpp = 32;

    // Frees (Deletes) existing surface
    if (surface)
    {
        SDL_FreeSurface(surface);
        surface = NULL;
    }

#ifdef CANNONBALL_PS5_FASTBLIT
    // PS5 SDL window surface uses ABGR8888. Generate game palette in that format.
    #ifdef CANNONBALL_PS5_NATIVE
    static void* native_surface_pixels = nullptr;
    const size_t max_pixels = size_t(796) * 448;

    if (!native_surface_pixels)
        native_surface_pixels =
            cbps5::cached_buffer(max_pixels * 4);

    if (!native_surface_pixels ||
        src_width <= 0 || src_height <= 0 ||
        size_t(src_width) * src_height > max_pixels)
        return false;

    surface = SDL_CreateRGBSurfaceWithFormatFrom(
        native_surface_pixels,
        src_width, src_height, bpp,
        src_width * 4,
        SDL_PIXELFORMAT_ABGR8888);
#else
    surface = SDL_CreateRGBSurfaceWithFormat(
        0, src_width, src_height, bpp,
        SDL_PIXELFORMAT_ABGR8888);
#endif
#else
    surface = SDL_CreateRGBSurface(0,
                                  src_width,
                                  src_height,
                                  bpp,
                                  0,
                                  0,
                                  0,
                                  0);
#endif

    if (!surface)
    {
        std::cerr << "Surface creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, config.video.filtering ? "linear" : "nearest");
    window = SDL_CreateWindow(
        "Cannonball", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, scn_width, scn_height,
        flags);

    if (!window)
    {
        std::cerr << "SDL_CreateWindow: " << SDL_GetError() << std::endl;
        return false;
    }
#ifdef CANNONBALL_PS5_FASTBLIT
    const char *fast_reason = "unsupported framebuffer";
    renderer = NULL;
    texture = NULL;
    fast_active = false;
    fast_window_surface = NULL;
    if (scanlines != 0) {
        fast_reason = "scanlines enabled (using SDL fallback)";
    } else if (config.video.filtering != 0) {
        fast_reason = "linear filtering enabled (using SDL fallback)";
    } else if (video_mode != video_settings_t::MODE_FULL &&
               video_mode != video_settings_t::MODE_STRETCH) {
        fast_reason = "windowed mode unsupported";
    } else {
        fast_window_surface = SDL_GetWindowSurface(window);
        if (!fast_window_surface || !fast_window_surface->pixels ||
            !fast_window_surface->format || !surface->format ||
            fast_window_surface->format->format != surface->format->format ||
            fast_window_surface->format->BytesPerPixel != 4 ||
            SDL_MUSTLOCK(fast_window_surface)) {
            fast_reason = "SDL_GetWindowSurface missing or incompatible";
        } else if (!fast_plan.configure(src_width, src_height,
                    fast_window_surface->w, fast_window_surface->h,
                    ps5fast::Rect{dst_rect.x, dst_rect.y, dst_rect.w, dst_rect.h})) {
            fast_reason = "output dimensions invalid";
        } else if (SDL_FillRect(fast_window_surface, NULL,
                    SDL_MapRGB(fast_window_surface->format, 0, 0, 0)) != 0) {
            fast_reason = "black border initialization failed";
        } else {
            fast_active = true;
            fast_reason = "nearest-neighbour ABGR8888 window-surface scaling";
        }
    }
    if (!fast_active)
        std::fprintf(stderr, "CannonBall FastBlit fallback: %s\n",
                     fast_reason);
    if (!fast_active) {
#endif
#ifdef CANNONBALL_PS5
    // A PS5 SDL2 build may only provide the software renderer.
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer)
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
#else
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
#endif
    if (!renderer)
    {
        std::cerr << "SDL_CreateRenderer: " << SDL_GetError() << std::endl;
        return false;
    }
    texture = SDL_CreateTexture(renderer,
#ifdef CANNONBALL_PS5_FASTBLIT
                                surface->format->format,
#else
                               SDL_PIXELFORMAT_ARGB8888,
#endif
                               SDL_TEXTUREACCESS_STREAMING,
                               src_width, src_height);
    if (!texture)
    {
        std::cerr << "SDL_CreateTexture: " << SDL_GetError() << std::endl;
        return false;
    }
#ifdef CANNONBALL_PS5_FASTBLIT
    } // !fast_active: fall back to the existing SDL_Renderer path
#endif

    // Convert the SDL pixel surface to 32 bit.
    // This is potentially a larger surface area than the internal pixel array.
    screen_pixels = (uint32_t*)surface->pixels;
    
    // SDL Pixel Format Information
    Rshift = surface->format->Rshift;
    Gshift = surface->format->Gshift;
    Bshift = surface->format->Bshift;
    Rmask  = surface->format->Rmask;
    Gmask  = surface->format->Gmask;
    Bmask  = surface->format->Bmask;

    return true;
}

void Render::disable()
{
#ifdef CANNONBALL_PS5_FASTBLIT
    if (fast_active) {
        // SDL owns the window surface; destroying the window frees it.
        SDL_DestroyWindow(window);
        fast_window_surface = NULL;
        fast_active = false;
        return;
    }
#endif
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

bool Render::start_frame()
{
    return true;
}

bool Render::finalize_frame()
{
#ifdef CANNONBALL_PS5_BETA1A
    // Draw after palette conversion, but BEFORE FastBlit scales the image.
    if (ps5_pause_ui.active() && surface && surface->format && screen_pixels) {
        const SDL_PixelFormat* fmt = surface->format;
        ps5_pause_ui.draw(screen_pixels, src_width, src_height,
                          surface->pitch / sizeof(Uint32),
                          SDL_MapRGB(fmt, 15, 28, 45),
                          SDL_MapRGB(fmt, 11, 90, 113),
                          SDL_MapRGB(fmt, 20, 126, 143),
                          SDL_MapRGB(fmt, 255, 255, 255),
                          SDL_MapRGB(fmt, 172, 197, 202),
                          SDL_MapRGB(fmt, 3, 8, 15));
    }
#endif
#ifdef CANNONBALL_PS5_FASTBLIT
    if (fast_active) {
if (!fast_plan.draw(surface->pixels, surface->pitch,
                            fast_window_surface->pixels, fast_window_surface->pitch)) {
            std::fprintf(stderr, "CannonBall FastBlit: invalid framebuffer dimensions\n");
            return false;
        }
// Uses the existing PS5 SDL flip queue and correct double-buffer ownership.
        if (SDL_UpdateWindowSurface(window) != 0) {
            std::fprintf(stderr, "CannonBall FastBlit: SDL_UpdateWindowSurface: %s\n", SDL_GetError());
            return false;
        }
        #ifdef CANNONBALL_PS5_NATIVE
        if (native_handoff())
            SDL_UpdateWindowSurface(window);
#endif
        return true;
    }
#endif
    SDL_UpdateTexture(texture, NULL, screen_pixels, src_width * sizeof (Uint32));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, &src_rect, &dst_rect);

    // Very basic scanlines
    if (scanlines && scale != 1)
    {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, ((scanlines - 1) << 8) / 100);
        SDL_Rect r;
        r.x = dst_rect.x;
        r.w = dst_rect.w;
        r.h = scale >> 1;
        r.y = scale >> 1;

        for (; r.y < dst_rect.h; r.y += scale)
            SDL_RenderDrawRect(renderer, &r);
    }

    SDL_RenderPresent(renderer);

#ifdef CANNONBALL_PS5_NATIVE
    if (native_handoff())
        SDL_RenderPresent(renderer);
#endif

    return true;
}

void Render::draw_frame(uint16_t* pixels)
{

    uint32_t* spix = screen_pixels;

    // Lookup real RGB value from rgb array for backbuffer
    for (int i = 0; i < (src_width * src_height); i++)
        *(spix++) = rgb[*(pixels++)];
}