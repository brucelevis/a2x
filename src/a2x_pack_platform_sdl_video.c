/*
    Copyright 2010, 2016-2019 Alex Margarit <alex@alxm.org>
    This file is part of a2x, a C video game framework.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "a2x_pack_platform_sdl_video.v.h"

#if A_CONFIG_LIB_SDL
#include <SDL.h>

#include "a2x_pack_main.v.h"
#include "a2x_pack_out.v.h"
#include "a2x_pack_platform_wiz.v.h"
#include "a2x_pack_screen.v.h"
#include "a2x_pack_str.v.h"

#if A_CONFIG_LIB_SDL == 1
    static SDL_Surface* g_sdlScreen;
#elif A_CONFIG_LIB_SDL == 2
    SDL_Renderer* a__sdlRenderer;
    static SDL_Window* g_sdlWindow;
    static int g_clearR, g_clearG, g_clearB;

    #if A_CONFIG_LIB_RENDER_SOFTWARE
        static SDL_Texture* g_sdlTexture;
    #endif
#endif

#if A_CONFIG_LIB_SDL == 2 || A_CONFIG_SYSTEM_EMSCRIPTEN
    static bool g_vsync = A_CONFIG_SCREEN_VSYNC;
#else
    static const bool g_vsync = false;
#endif

void a_platform_sdl_video__init(void)
{
    #if A_CONFIG_SYSTEM_PANDORA
        putenv("SDL_VIDEODRIVER=omapdss");
        putenv("SDL_OMAP_LAYER_SIZE=pixelperfect");
    #endif

    if(SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        A__FATAL("SDL_InitSubSystem: %s", SDL_GetError());
    }
}

void a_platform_sdl_video__uninit(void)
{
    #if A_CONFIG_LIB_SDL == 1
        #if !A_CONFIG_SCREEN_ALLOCATE
            if(SDL_MUSTLOCK(g_sdlScreen)) {
                SDL_UnlockSurface(g_sdlScreen);
            }
        #endif
    #elif A_CONFIG_LIB_SDL == 2
        #if A_CONFIG_LIB_RENDER_SOFTWARE
            SDL_DestroyTexture(g_sdlTexture);
        #endif

        SDL_DestroyRenderer(a__sdlRenderer);
        SDL_DestroyWindow(g_sdlWindow);
    #endif

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void a_platform__screenInit(int Width, int Height)
{
    #if A_CONFIG_LIB_SDL == 1
        int bpp = 0;
        uint32_t videoFlags = SDL_SWSURFACE;

        #if A_CONFIG_SCREEN_FULLSCREEN
            videoFlags |= SDL_FULLSCREEN;
        #endif

        bpp = SDL_VideoModeOK(Width, Height, A_CONFIG_SCREEN_BPP, videoFlags);

        if(bpp == 0) {
            A__FATAL("SDL_VideoModeOK: %dx%d:%d video not available",
                     Width,
                     Height,
                     A_CONFIG_SCREEN_BPP);
        }

        g_sdlScreen = SDL_SetVideoMode(Width * A_CONFIG_SCREEN_ZOOM,
                                       Height * A_CONFIG_SCREEN_ZOOM,
                                       A_CONFIG_SCREEN_BPP,
                                       videoFlags);

        if(g_sdlScreen == NULL) {
            A__FATAL("SDL_SetVideoMode: %s", SDL_GetError());
        }

        SDL_SetClipRect(g_sdlScreen, NULL);

        #if !A_CONFIG_SCREEN_ALLOCATE
            if(SDL_MUSTLOCK(g_sdlScreen)) {
                if(SDL_LockSurface(g_sdlScreen) < 0) {
                    A__FATAL("SDL_LockSurface: %s", SDL_GetError());
                }
            }

            a__screen.pixels = g_sdlScreen->pixels;
        #endif
    #elif A_CONFIG_LIB_SDL == 2
        int ret;
        uint32_t windowFlags = SDL_WINDOW_RESIZABLE;

        #if A_CONFIG_SCREEN_MAXIMIZED
            windowFlags |= SDL_WINDOW_MAXIMIZED;
        #endif

        #if A_CONFIG_SCREEN_FULLSCREEN
            windowFlags |= SDL_WINDOW_FULLSCREEN;
        #endif

        g_sdlWindow = SDL_CreateWindow("",
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       Width * A_CONFIG_SCREEN_ZOOM,
                                       Height * A_CONFIG_SCREEN_ZOOM,
                                       windowFlags);
        if(g_sdlWindow == NULL) {
            A__FATAL("SDL_CreateWindow: %s", SDL_GetError());
        }

        #if A_CONFIG_SCREEN_FULLSCREEN
            if(SDL_SetWindowFullscreen(
                g_sdlWindow, SDL_WINDOW_FULLSCREEN) < 0) {

                a_out__error("SDL_SetWindowFullscreen: %s", SDL_GetError());
            }
        #endif

        uint32_t rendererFlags = SDL_RENDERER_ACCELERATED
                               | SDL_RENDERER_TARGETTEXTURE;

        #if A_CONFIG_SCREEN_VSYNC
            rendererFlags |= SDL_RENDERER_PRESENTVSYNC;
        #endif

        a__sdlRenderer = SDL_CreateRenderer(g_sdlWindow, -1, rendererFlags);

        if(a__sdlRenderer == NULL) {
            A__FATAL("SDL_CreateRenderer: %s", SDL_GetError());
        }

        SDL_RendererInfo info;
        SDL_GetRendererInfo(a__sdlRenderer, &info);

        if(!(info.flags & SDL_RENDERER_TARGETTEXTURE)) {
            A__FATAL("SDL_CreateRenderer: "
                     "System does not support SDL_RENDERER_TARGETTEXTURE");
        }

        if(info.flags & SDL_RENDERER_ACCELERATED) {
            a_out__message("Using SDL2 accelerated renderer");
        } else {
            a_out__message("Using SDL2 software renderer");
        }

        g_vsync = info.flags & SDL_RENDERER_PRESENTVSYNC;

        ret = SDL_RenderSetLogicalSize(a__sdlRenderer, Width, Height);

        if(ret < 0) {
            A__FATAL("SDL_RenderSetLogicalSize: %s", SDL_GetError());
        }

        #if A_CONFIG_LIB_RENDER_SOFTWARE
            g_sdlTexture = SDL_CreateTexture(a__sdlRenderer,
                                             A_SDL__PIXEL_FORMAT,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             Width,
                                             Height);
            if(g_sdlTexture == NULL) {
                A__FATAL("SDL_CreateTexture: %s", SDL_GetError());
            }
        #endif

        SDL_SetHintWithPriority(
            SDL_HINT_RENDER_SCALE_QUALITY, "nearest", SDL_HINT_OVERRIDE);

        a_pixel_toRgb(a_pixel_fromHex(A_CONFIG_COLOR_SCREEN_BORDER),
                      &g_clearR,
                      &g_clearG,
                      &g_clearB);
    #endif

    a_out__message("V-sync is %s", g_vsync ? "on" : "off");

    #if A_CONFIG_TRAIT_DESKTOP
        const char* caption = a_str__fmt512("%s %s",
                                            A_CONFIG_APP_TITLE,
                                            A_CONFIG_APP_VERSION_STRING);

        #if A_CONFIG_LIB_SDL == 1
            SDL_WM_SetCaption(caption, NULL);
        #elif A_CONFIG_LIB_SDL == 2
            SDL_SetWindowTitle(g_sdlWindow, caption);
        #endif
    #endif

    a_platform__screenMouseCursorSet(A_CONFIG_INPUT_MOUSE_CURSOR);

    #if A_CONFIG_SCREEN_WIZ_FIX
        a_platform_wiz__portraitModeSet();
    #endif
}

void a_platform__screenShow(void)
{
    #if A_CONFIG_LIB_SDL == 1
        #if A_CONFIG_SCREEN_WIZ_FIX
            // The Wiz screen has diagonal tearing in landscape mode. As a slow
            // but simple workaround, the screen is set to portrait mode where
            // 320,0 is top-left and 0,240 is bottom-right, and the game's
            // landscape pixel buffer is rotated to this format every frame.

            if(SDL_MUSTLOCK(g_sdlScreen)) {
                if(SDL_LockSurface(g_sdlScreen) < 0) {
                    A__FATAL("SDL_LockSurface: %s", SDL_GetError());
                }
            }

            #define A__SCREEN_TOTAL (A_CONFIG_SCREEN_HARDWARE_WIDTH \
                                        * A_CONFIG_SCREEN_HARDWARE_HEIGHT)

            APixel* dst = (APixel*)g_sdlScreen->pixels + A__SCREEN_TOTAL;
            const APixel* src = a__screen.pixels;

            for(int i = A_CONFIG_SCREEN_HARDWARE_HEIGHT;
                i--;
                dst += A__SCREEN_TOTAL + 1) {

                for(int j = A_CONFIG_SCREEN_HARDWARE_WIDTH; j--; ) {
                    dst -= A_CONFIG_SCREEN_HARDWARE_HEIGHT;
                    *dst = *src++;
                }
            }

            if(SDL_MUSTLOCK(g_sdlScreen)) {
                SDL_UnlockSurface(g_sdlScreen);
            }

            SDL_Flip(g_sdlScreen);

            return;
        #endif

        #if A_CONFIG_SCREEN_ALLOCATE
            if(SDL_MUSTLOCK(g_sdlScreen)) {
                if(SDL_LockSurface(g_sdlScreen) < 0) {
                    A__FATAL("SDL_LockSurface: %s", SDL_GetError());
                }
            }

            int zoom = a_screen__zoomGet();

            if(zoom <= 1) {
                memcpy(g_sdlScreen->pixels,
                       a__screen.pixels,
                       a__screen.pixelsSize);
            } else {
                int realH = g_sdlScreen->h / zoom;
                int realW = g_sdlScreen->w / zoom;

                APixel* dst = (APixel*)g_sdlScreen->pixels;
                const APixel* srcStart = a__screen.pixels;

                ptrdiff_t dstRemainderInc =
                    (int)g_sdlScreen->pitch / (int)sizeof(APixel)
                        - g_sdlScreen->w;
                ptrdiff_t srcStartInc = g_sdlScreen->w / zoom;

                for(int y = realH; y--; ) {
                    for(int z = zoom; z--; ) {
                        const APixel* src = srcStart;

                        for(int x = realW; x--; ) {
                            for(int z = zoom; z--; ) {
                                *dst++ = *src;
                            }

                            src++;
                        }

                        dst += dstRemainderInc;
                    }

                    srcStart += srcStartInc;
                }
            }

            if(SDL_MUSTLOCK(g_sdlScreen)) {
                SDL_UnlockSurface(g_sdlScreen);
            }

            SDL_Flip(g_sdlScreen);
        #else // !A_CONFIG_SCREEN_ALLOCATE
            if(SDL_MUSTLOCK(g_sdlScreen)) {
                SDL_UnlockSurface(g_sdlScreen);
            }

            SDL_Flip(g_sdlScreen);

            if(SDL_MUSTLOCK(g_sdlScreen)) {
                if(SDL_LockSurface(g_sdlScreen) < 0) {
                    A__FATAL("SDL_LockSurface: %s", SDL_GetError());
                }
            }

            a__screen.pixels = g_sdlScreen->pixels;
        #endif
    #elif A_CONFIG_LIB_SDL == 2
        #if A_CONFIG_LIB_RENDER_SDL
            if(SDL_SetRenderTarget(a__sdlRenderer, NULL) < 0) {
                A__FATAL("SDL_SetRenderTarget: %s", SDL_GetError());
            }

            if(SDL_RenderSetClipRect(a__sdlRenderer, NULL) < 0) {
                a_out__error("SDL_RenderSetClipRect: %s", SDL_GetError());
            }
        #endif

        if(SDL_SetRenderDrawColor(a__sdlRenderer,
                                  (uint8_t)g_clearR,
                                  (uint8_t)g_clearG,
                                  (uint8_t)g_clearB,
                                  SDL_ALPHA_OPAQUE) < 0) {

            a_out__error("SDL_SetRenderDrawColor: %s", SDL_GetError());
        }

        a_platform__renderClear();

        #if A_CONFIG_LIB_RENDER_SOFTWARE
            if(SDL_UpdateTexture(g_sdlTexture,
                                 NULL,
                                 a__screen.pixels,
                                 a__screen.width * (int)sizeof(APixel)) < 0) {

                A__FATAL("SDL_UpdateTexture: %s", SDL_GetError());
            }

            if(SDL_RenderCopy(a__sdlRenderer, g_sdlTexture, NULL, NULL) < 0) {
                A__FATAL("SDL_RenderCopy: %s", SDL_GetError());
            }
        #else
            a_pixel_push();
            a_pixel_blendSet(A_PIXEL_BLEND_PLAIN);

            a_platform__textureBlit(a__screen.texture, 0, 0, false);

            a_platform__renderTargetSet(a__screen.texture);
            a_platform__renderTargetClipSet(a__screen.clipX,
                                      a__screen.clipY,
                                      a__screen.clipWidth,
                                      a__screen.clipHeight);

            a_pixel_pop();
        #endif

        SDL_RenderPresent(a__sdlRenderer);
    #endif
}

#if A_CONFIG_LIB_SDL == 2
void a_platform__renderClear(void)
{
    if(SDL_RenderClear(a__sdlRenderer) < 0) {
        a_out__error("SDL_RenderClear: %s", SDL_GetError());
    }
}
#endif

#if A_CONFIG_SCREEN_HARDWARE_WIDTH > 0 && A_CONFIG_SCREEN_HARDWARE_HEIGHT > 0
void a_platform__screenResolutionGetNative(int* Width, int* Height)
{
    *Width = A_CONFIG_SCREEN_HARDWARE_WIDTH;
    *Height = A_CONFIG_SCREEN_HARDWARE_HEIGHT;
}
#elif A_CONFIG_LIB_SDL == 2
void a_platform__screenResolutionGetNative(int* Width, int* Height)
{
    SDL_DisplayMode mode;

    if(SDL_GetCurrentDisplayMode(0, &mode) < 0) {
        a_out__error("SDL_GetCurrentDisplayMode: %s", SDL_GetError());
        return;
    }

    a_out__message("Display info: %dx%d %dbpp",
                   mode.w,
                   mode.h,
                   SDL_BITSPERPIXEL(mode.format));

    *Width = mode.w;
    *Height = mode.h;
}
#elif A_CONFIG_LIB_SDL == 1
void a_platform__screenResolutionGetNative(int* Width, int* Height)
{
    const SDL_VideoInfo* info = SDL_GetVideoInfo();

    *Width = info->current_w;
    *Height = info->current_h;
}
#endif

bool a_platform__screenVsyncGet(void)
{
    return g_vsync;
}

void a_platform__screenZoomSet(int Zoom)
{
    #if A_CONFIG_LIB_SDL == 1
        #if A_CONFIG_SCREEN_ALLOCATE
            g_sdlScreen = SDL_SetVideoMode(a__screen.width * Zoom,
                                           a__screen.height * Zoom,
                                           0,
                                           g_sdlScreen->flags);

            if(g_sdlScreen == NULL) {
                A__FATAL("SDL_SetVideoMode: %s", SDL_GetError());
            }

            SDL_SetClipRect(g_sdlScreen, NULL);
        #else
            A_UNUSED(Zoom);

            a_out__warning(
                "SDL 1.2 Zoom needs A_CONFIG_SCREEN_ALLOCATE=1");
        #endif
    #elif A_CONFIG_LIB_SDL == 2
        SDL_SetWindowSize(
            g_sdlWindow, a__screen.width * Zoom, a__screen.height * Zoom);
    #endif
}

void a_platform__screenFullscreenSet(bool Fullscreen)
{
    #if A_CONFIG_LIB_SDL == 1
        #if A_CONFIG_SCREEN_ALLOCATE
            uint32_t videoFlags = g_sdlScreen->flags;

            if(Fullscreen) {
                videoFlags |= SDL_FULLSCREEN;
            } else {
                videoFlags &= ~(uint32_t)SDL_FULLSCREEN;
            }

            g_sdlScreen = SDL_SetVideoMode(0, 0, 0, videoFlags);

            if(g_sdlScreen == NULL) {
                A__FATAL("SDL_SetVideoMode: %s", SDL_GetError());
            }

            SDL_SetClipRect(g_sdlScreen, NULL);
        #else
            a_out__warning(
                "SDL 1.2 fullscreen needs A_CONFIG_SCREEN_ALLOCATE=1");
        #endif
    #elif A_CONFIG_LIB_SDL == 2
        if(SDL_SetWindowFullscreen(
            g_sdlWindow, Fullscreen ? SDL_WINDOW_FULLSCREEN : 0) < 0) {

            a_out__error("SDL_SetWindowFullscreen: %s", SDL_GetError());
        }
    #endif

    a_platform__screenMouseCursorSet(!Fullscreen);
}

void a_platform__screenMouseCursorSet(bool Show)
{
    #if A_CONFIG_INPUT_MOUSE_CURSOR
        int setting = Show ? SDL_ENABLE : SDL_DISABLE;
    #else
        A_UNUSED(Show);
        int setting = SDL_DISABLE;
    #endif

    if(SDL_ShowCursor(setting) < 0) {
        a_out__error("SDL_ShowCursor: %s", SDL_GetError());
    }
}
#endif // A_CONFIG_LIB_SDL
