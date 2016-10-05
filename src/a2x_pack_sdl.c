/*
    Copyright 2010, 2016 Alex Margarit

    This file is part of a2x-framework.

    a2x-framework is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    a2x-framework is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with a2x-framework.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "a2x_pack_sdl.v.h"

#include <SDL.h>
#include <SDL_mixer.h>

typedef struct ASdlInputHeader {
    char* name;
    int device_index;
} ASdlInputHeader;

typedef struct ASdlInputButton {
    ASdlInputHeader header;
    AInputButton* input;
    int code; // SDL button/key code
    bool lastStatePressed;
} ASdlInputButton;

typedef struct ASdlInputAnalog {
    ASdlInputHeader header;
    AInputAnalog* input;
    int xaxis_index;
    int yaxis_index;
} ASdlInputAnalog;

typedef struct ASdlInputTouch {
    ASdlInputHeader header;
    AInputTouch* input;
} ASdlInputTouch;

typedef struct ASdlInputController {
    SDL_Joystick* joystick;
    #if A_USE_LIB_SDL
        uint8_t id;
    #elif A_USE_LIB_SDL2
        SDL_JoystickID id;
    #endif
    int numButtons;
    int numHats;
    AStrHash* buttons;
} ASdlInputController;

static uint32_t g_sdlFlags;

#if A_USE_LIB_SDL
    static SDL_Surface* g_sdlScreen = NULL;
#elif A_USE_LIB_SDL2
    static SDL_Window* g_sdlWindow = NULL;
    static SDL_Renderer* g_sdlRenderer = NULL;
    static SDL_Texture* g_sdlTexture = NULL;
#endif

static AStrHash* g_buttons;
static AStrHash* g_analogs;
static AStrHash* g_touchScreens;
static AList* g_controllers;

static void freeHeader(ASdlInputHeader* Header)
{
    free(Header->name);
}

static void addButton(const char* Name, int Code)
{
    ASdlInputButton* b = a_strhash_get(g_buttons, Name);

    if(b) {
        a_out__error("Button '%s' already defined", Name);
        return;
    }

    b = a_mem_malloc(sizeof(ASdlInputButton));

    b->header.name = a_str_dup(Name);
    b->code = Code;
    b->lastStatePressed = false;

    a_strhash_add(g_buttons, Name, b);
}

#if A_PLATFORM_CAANOO || A_PLATFORM_PANDORA
    static void addAnalog(const char* Name, int DeviceIndex, char* DeviceName, int XAxisIndex, int YAxisIndex)
    {
        if(DeviceIndex == -1 && DeviceName == NULL) {
            a_out__error("Inputs must specify device index or name");
            return;
        }

        ASdlInputAnalog* a = a_strhash_get(g_analogs, Name);

        if(a) {
            a_out__error("Analog '%s' is already defined", Name);
            return;
        }

        a = a_mem_malloc(sizeof(ASdlInputAnalog));

        a->header.name = a_str_dup(Name);
        a->header.device_index = DeviceIndex;
        a->xaxis_index = XAxisIndex;
        a->yaxis_index = YAxisIndex;

        // check if we requested a specific device by Name
        if(DeviceName) {
            A_LIST_ITERATE(g_controllers, ASdlInputController*, c) {
                if(a_str_equal(DeviceName, SDL_JoystickName(c->id))) {
                    a->header.device_index = c->id;
                    break;
                }
            }
        }

        a_strhash_add(g_analogs, Name, a);
    }
#endif

static void addTouch(const char* Name)
{
    ASdlInputTouch* t = a_strhash_get(g_touchScreens, Name);

    if(t) {
        a_out__error("Touchscreen '%s' is already defined", Name);
        return;
    }

    t = a_mem_malloc(sizeof(ASdlInputTouch));

    t->header.name = a_str_dup(Name);

    a_strhash_add(g_touchScreens, Name, t);
}

void a_sdl__init(void)
{
    int ret = 0;
    g_sdlFlags = 0;

    if(a_settings_getBool("video.window")) {
        g_sdlFlags |= SDL_INIT_VIDEO;
    }

    if(a_settings_getBool("sound.on")) {
        g_sdlFlags |= SDL_INIT_AUDIO;
    }

    g_sdlFlags |= SDL_INIT_JOYSTICK;

    #if !(A_PLATFORM_WIZ || A_PLATFORM_CAANOO)
        g_sdlFlags |= SDL_INIT_TIMER;
    #endif

    ret = SDL_Init(g_sdlFlags);

    if(ret != 0) {
        a_out__fatal("SDL: %s", SDL_GetError());
    }

    if(a_settings_getBool("sound.on")) {
        #if A_PLATFORM_LINUXPC || A_PLATFORM_MINGW
            if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) != 0) {
                a_settings__set("sound.on", "0");
            }
        #elif A_PLATFORM_GP2X
            if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 256) != 0) {
                a_settings__set("sound.on", "0");
            }
        #else
            if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 512) != 0) {
                a_settings__set("sound.on", "0");
            }
        #endif
    }

    g_buttons = a_strhash_new();
    g_analogs = a_strhash_new();
    g_touchScreens = a_strhash_new();
    g_controllers = a_list_new();

    const int joysticksNum = SDL_NumJoysticks();
    a_out__message("Found %d controllers", joysticksNum);

    for(int i = 0; i < joysticksNum; i++) {
        SDL_Joystick* joystick = SDL_JoystickOpen(i);

        if(joystick == NULL) {
            a_out__error("SDL_JoystickOpen(%d) failed: %s",
                         i,
                         SDL_GetError());
            continue;
        }

        #if A_USE_LIB_SDL2
            SDL_JoystickID id = SDL_JoystickInstanceID(joystick);

            if(id < 0) {
                a_out__error("SDL_JoystickInstanceID(%d) failed: %s",
                             i,
                             SDL_GetError());
                SDL_JoystickClose(joystick);
                continue;
            }
        #endif

        ASdlInputController* c = a_mem_malloc(sizeof(ASdlInputController));

        c->joystick = joystick;
        #if A_USE_LIB_SDL
            c->id = i;
        #elif A_USE_LIB_SDL2
            c->id = id;
        #endif
        c->numButtons = SDL_JoystickNumButtons(c->joystick);
        c->numHats = SDL_JoystickNumHats(c->joystick);
        c->buttons = a_strhash_new();

        a_list_addLast(g_controllers, c);

        #if A_PLATFORM_GP2X || A_PLATFORM_WIZ || A_PLATFORM_CAANOO
            if(i == 0) {
                // Joystick 0 is the built-in controls on these platforms
                continue;
            }
        #endif

        AStrHash* savedButtons = g_buttons;
        g_buttons = c->buttons;

        for(int j = 0; j < c->numButtons; j++) {
            char name[32];
            snprintf(name, sizeof(name), "controller.b%d", j);
            addButton(name, j);
        }

        if(c->numHats > 0) {
            addButton("controller.up", -1);
            addButton("controller.down", -1);
            addButton("controller.left", -1);
            addButton("controller.right", -1);
        }

        g_buttons = savedButtons;
    }

    #if A_PLATFORM_GP2X
        addButton("gp2x.up", 0);
        addButton("gp2x.down", 4);
        addButton("gp2x.left", 2);
        addButton("gp2x.right", 6);
        addButton("gp2x.upleft", 1);
        addButton("gp2x.upright", 7);
        addButton("gp2x.downleft", 3);
        addButton("gp2x.downright", 5);
        addButton("gp2x.l", 10);
        addButton("gp2x.r", 11);
        addButton("gp2x.a", 12);
        addButton("gp2x.b", 13);
        addButton("gp2x.x", 14);
        addButton("gp2x.y", 15);
        addButton("gp2x.start", 8);
        addButton("gp2x.select", 9);
        addButton("gp2x.volup", 16);
        addButton("gp2x.voldown", 17);
        addButton("gp2x.stickclick", 18);
        addTouch("gp2x.touch");
    #elif A_PLATFORM_WIZ
        addButton("wiz.up", 0);
        addButton("wiz.down", 4);
        addButton("wiz.left", 2);
        addButton("wiz.right", 6);
        addButton("wiz.upleft", 1);
        addButton("wiz.upright", 7);
        addButton("wiz.downleft", 3);
        addButton("wiz.downright", 5);
        addButton("wiz.l", 10);
        addButton("wiz.r", 11);
        addButton("wiz.a", 12);
        addButton("wiz.b", 13);
        addButton("wiz.x", 14);
        addButton("wiz.y", 15);
        addButton("wiz.menu", 8);
        addButton("wiz.select", 9);
        addButton("wiz.volup", 16);
        addButton("wiz.voldown", 17);
        addTouch("wiz.touch");
    #elif A_PLATFORM_CAANOO
        addButton("caanoo.up", -1);
        addButton("caanoo.down", -1);
        addButton("caanoo.left", -1);
        addButton("caanoo.right", -1);
        addButton("caanoo.l", 4);
        addButton("caanoo.r", 5);
        addButton("caanoo.a", 0);
        addButton("caanoo.b", 2);
        addButton("caanoo.x", 1);
        addButton("caanoo.y", 3);
        addButton("caanoo.home", 6);
        addButton("caanoo.hold", 7);
        addButton("caanoo.1", 8);
        addButton("caanoo.2", 9);
        addAnalog("caanoo.stick", 0, NULL, 0, 1);
        addTouch("caanoo.touch");
    #elif A_PLATFORM_PANDORA
        addButton("pandora.up", SDLK_UP);
        addButton("pandora.down", SDLK_DOWN);
        addButton("pandora.left", SDLK_LEFT);
        addButton("pandora.right", SDLK_RIGHT);
        addButton("pandora.l", SDLK_RSHIFT);
        addButton("pandora.r", SDLK_RCTRL);
        addButton("pandora.a", SDLK_HOME);
        addButton("pandora.b", SDLK_END);
        addButton("pandora.x", SDLK_PAGEDOWN);
        addButton("pandora.y", SDLK_PAGEUP);
        addButton("pandora.start", SDLK_LALT);
        addButton("pandora.select", SDLK_LCTRL);
        addTouch("pandora.touch");
        addAnalog("pandora.nub1", -1, "nub0", 0, 1);
        addAnalog("pandora.nub2", -1, "nub1", 0, 1);
        addButton("pandora.m", SDLK_m);
        addButton("pandora.s", SDLK_s);
    #elif A_PLATFORM_LINUXPC || A_PLATFORM_MINGW
        addButton("pc.up", SDLK_UP);
        addButton("pc.down", SDLK_DOWN);
        addButton("pc.left", SDLK_LEFT);
        addButton("pc.right", SDLK_RIGHT);
        addButton("pc.z", SDLK_z);
        addButton("pc.x", SDLK_x);
        addButton("pc.c", SDLK_c);
        addButton("pc.v", SDLK_v);
        addButton("pc.m", SDLK_m);
        addButton("pc.enter", SDLK_RETURN);
        addButton("pc.space", SDLK_SPACE);
        addButton("pc.f1", SDLK_F1);
        addButton("pc.f2", SDLK_F2);
        addButton("pc.f3", SDLK_F3);
        addButton("pc.f4", SDLK_F4);
        addButton("pc.f5", SDLK_F5);
        addButton("pc.f6", SDLK_F6);
        addButton("pc.f7", SDLK_F7);
        addButton("pc.f8", SDLK_F8);
        addButton("pc.f9", SDLK_F9);
        addButton("pc.f10", SDLK_F10);
        addButton("pc.f11", SDLK_F11);
        addButton("pc.f12", SDLK_F12);
        addButton("pc.1", SDLK_1);
        addButton("pc.0", SDLK_0);
        addTouch("pc.mouse");
    #endif
}

void a_sdl__uninit(void)
{
    A_STRHASH_ITERATE(g_buttons, ASdlInputButton*, b) {
        freeHeader(&b->header);
        free(b);
    }

    A_STRHASH_ITERATE(g_analogs, ASdlInputAnalog*, a) {
        freeHeader(&a->header);
        free(a);
    }

    A_STRHASH_ITERATE(g_touchScreens, ASdlInputTouch*, t) {
        freeHeader(&t->header);
        free(t);
    }

    A_LIST_ITERATE(g_controllers, ASdlInputController*, c) {
        A_STRHASH_ITERATE(c->buttons, ASdlInputButton*, b) {
            freeHeader(&b->header);
            free(b);
        }

        SDL_JoystickClose(c->joystick);
        a_strhash_free(c->buttons);
        free(c);
    }

    a_strhash_free(g_buttons);
    a_strhash_free(g_analogs);
    a_strhash_free(g_touchScreens);
    a_list_free(g_controllers);

    if(a_settings_getBool("sound.on")) {
        Mix_CloseAudio();
    }

    if(a_settings_getBool("video.window")) {
        #if A_USE_LIB_SDL
            if(!a_settings_getBool("video.doubleBuffer")) {
                if(SDL_MUSTLOCK(g_sdlScreen)) {
                    SDL_UnlockSurface(g_sdlScreen);
                }
            }
        #elif A_USE_LIB_SDL2
            SDL_DestroyTexture(g_sdlTexture);
            SDL_DestroyRenderer(g_sdlRenderer);
            SDL_DestroyWindow(g_sdlWindow);
        #endif
    }

    SDL_QuitSubSystem(g_sdlFlags);
    SDL_Quit();
}

void a_sdl__screen_set(void)
{
    #if A_USE_LIB_SDL
        int bpp = 0;
        uint32_t videoFlags = SDL_SWSURFACE;

        if(a_settings_getBool("video.fullscreen")) {
            videoFlags |= SDL_FULLSCREEN;
        }

        bpp = SDL_VideoModeOK(a_screen__width,
                              a_screen__height,
                              A_PIXEL_BPP,
                              videoFlags);
        if(bpp == 0) {
            a_out__fatal("SDL: %dx%d:%d video not available",
                         a_screen__width,
                         a_screen__height,
                         A_PIXEL_BPP);
        }

        g_sdlScreen = SDL_SetVideoMode(a_screen__width,
                                       a_screen__height,
                                       A_PIXEL_BPP,
                                       videoFlags);
        if(g_sdlScreen == NULL) {
            a_out__fatal("SDL: %s", SDL_GetError());
        }

        SDL_SetClipRect(g_sdlScreen, NULL);

        if(!a_settings_getBool("video.doubleBuffer")) {
            if(SDL_MUSTLOCK(g_sdlScreen)) {
                if(SDL_LockSurface(g_sdlScreen) < 0) {
                    a_out__fatal("SDL_LockSurface failed: %s",
                                 SDL_GetError());
                }
            }
        }

        a_screen__pixels = g_sdlScreen->pixels;
    #elif A_USE_LIB_SDL2
        int ret;

        g_sdlWindow = SDL_CreateWindow("",
                                       SDL_WINDOWPOS_UNDEFINED,
                                       SDL_WINDOWPOS_UNDEFINED,
                                       a_screen__width,
                                       a_screen__height,
                                       SDL_WINDOW_RESIZABLE);
        if(g_sdlWindow == NULL) {
            a_out__fatal("SDL_CreateWindow failed: %s", SDL_GetError());
        }

        g_sdlRenderer = SDL_CreateRenderer(g_sdlWindow,
                                           -1,
                                           0);
        if(g_sdlRenderer == NULL) {
            a_out__fatal("SDL_CreateRenderer failed: %s", SDL_GetError());
        }

        ret = SDL_RenderSetLogicalSize(g_sdlRenderer,
                                       a_screen__width,
                                       a_screen__height);
        if(ret < 0) {
            a_out__fatal("SDL_RenderSetLogicalSize failed: %s", SDL_GetError());
        }

        g_sdlTexture = SDL_CreateTexture(g_sdlRenderer,
                                         #if A_PIXEL_BPP == 16
                                             SDL_PIXELFORMAT_RGB565,
                                         #elif A_PIXEL_BPP == 32
                                             SDL_PIXELFORMAT_RGBX8888,
                                         #else
                                             #error Invalid A_PIXEL_BPP value
                                         #endif
                                         SDL_TEXTUREACCESS_STREAMING,
                                         a_screen__width,
                                         a_screen__height);
        if(g_sdlTexture == NULL) {
            a_out__fatal("SDL_CreateTexture failed: %s", SDL_GetError());
        }

        SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY,
                                "nearest",
                                SDL_HINT_OVERRIDE);

        char* end;
        const char* color = a_settings_getString("video.borderColor");
        uint8_t r = strtol(color, &end, 0);
        uint8_t g = strtol(end, &end, 0);
        uint8_t b = strtol(end, NULL, 0);

        ret = SDL_SetRenderDrawColor(g_sdlRenderer, r, g, b, 255);
        if(ret < 0) {
            a_out__fatal("SDL_SetRenderDrawColor failed: %s", SDL_GetError());
        }
    #endif

    #if A_PLATFORM_LINUXPC
        char caption[64];
        snprintf(caption, 64, "%s %s",
            a_settings_getString("app.title"),
            a_settings_getString("app.version"));

        #if A_USE_LIB_SDL
            SDL_WM_SetCaption(caption, NULL);
        #elif A_USE_LIB_SDL2
            SDL_SetWindowTitle(g_sdlWindow, caption);
        #endif
    #else
        SDL_ShowCursor(SDL_DISABLE);
    #endif
}

void a_sdl__screen_show(void)
{
    #if A_USE_LIB_SDL
        #if A_PLATFORM_WIZ
            if(a_settings_getBool("video.fixWizTearing")) { // also video.doubleBuffer
                #define A_WIDTH 320
                #define A_HEIGHT 240

                if(SDL_MUSTLOCK(g_sdlScreen)) {
                    if(SDL_LockSurface(g_sdlScreen) < 0) {
                        a_out__fatal("SDL_LockSurface failed: %s",
                                     SDL_GetError());
                    }
                }

                APixel* dst = g_sdlScreen->pixels + A_WIDTH * A_HEIGHT;
                const APixel* src = a_screen__pixels;

                for(int i = A_HEIGHT; i--; dst += A_WIDTH * A_HEIGHT + 1) {
                    for(int j = A_WIDTH; j--; ) {
                        dst -= A_HEIGHT;
                        *dst = *src++;
                    }
                }

                if(SDL_MUSTLOCK(g_sdlScreen)) {
                    SDL_UnlockSurface(g_sdlScreen);
                }

                SDL_Flip(g_sdlScreen);
                return;
            }
        #endif

        if(a_settings_getBool("video.doubleBuffer")) {
            if(SDL_MUSTLOCK(g_sdlScreen)) {
                if(SDL_LockSurface(g_sdlScreen) < 0) {
                    a_out__fatal("SDL_LockSurface failed: %s",
                                 SDL_GetError());
                }
            }

            const APixel* src = a_screen__pixels;
            APixel* dst = g_sdlScreen->pixels;

            memcpy(dst, src, A_SCREEN_SIZE);

            if(SDL_MUSTLOCK(g_sdlScreen)) {
                SDL_UnlockSurface(g_sdlScreen);
            }

            SDL_Flip(g_sdlScreen);
        } else {
            if(SDL_MUSTLOCK(g_sdlScreen)) {
                SDL_UnlockSurface(g_sdlScreen);
            }

            SDL_Flip(g_sdlScreen);

            if(SDL_MUSTLOCK(g_sdlScreen)) {
                if(SDL_LockSurface(g_sdlScreen) < 0) {
                    a_out__fatal("SDL_LockSurface failed: %s",
                                 SDL_GetError());
                }
            }

            a_screen__pixels = g_sdlScreen->pixels;
            a_screen__savedPixels = a_screen__pixels;
        }
    #elif A_USE_LIB_SDL2
        int ret;

        ret = SDL_RenderClear(g_sdlRenderer);
        if(ret < 0) {
            a_out__fatal("SDL_RenderClear failed: %s", SDL_GetError());
        }

        ret = SDL_UpdateTexture(g_sdlTexture,
                                NULL,
                                a_screen__pixels,
                                a_screen__width * sizeof(APixel));
        if(ret < 0) {
            a_out__fatal("SDL_UpdateTexture failed: %s", SDL_GetError());
        }

        ret = SDL_RenderCopy(g_sdlRenderer, g_sdlTexture, NULL, NULL);
        if(ret < 0) {
            a_out__fatal("SDL_RenderCopy failed: %s", SDL_GetError());
        }

        SDL_RenderPresent(g_sdlRenderer);
    #endif
}

int a_sdl__sound_volumeMax(void)
{
    return MIX_MAX_VOLUME;
}

void* a_sdl__music_load(const char* Path)
{
    Mix_Music* m = Mix_LoadMUS(Path);

    if(!m) {
        a_out__error("Mix_LoadMUS failed: %s", Mix_GetError());
    }

    return m;
}

void a_sdl__music_free(void* Music)
{
    Mix_FreeMusic(Music);
}

void a_sdl__music_setVolume(int Volume)
{
    Mix_VolumeMusic(Volume);
}

void a_sdl__music_play(void* Music)
{
    Mix_PlayMusic(Music, -1);
}

void a_sdl__music_stop(void)
{
    Mix_HaltMusic();
}

void a_sdl__music_toggle(void)
{
    if(Mix_PausedMusic()) {
        Mix_ResumeMusic();
    } else {
        Mix_PauseMusic();
    }
}

void* a_sdl__sfx_loadFromFile(const char* Path)
{
    Mix_Chunk* sfx = Mix_LoadWAV(Path);

    if(sfx == NULL) {
        a_out__error("Mix_LoadWAV(%s) failed: %s", Path, SDL_GetError());
    }

    return sfx;
}

void* a_sdl__sfx_loadFromData(const uint8_t* Data, int Size)
{
    SDL_RWops* rw;
    Mix_Chunk* sfx = NULL;

    rw = SDL_RWFromMem((void*)Data, Size);
    if(rw == NULL) {
        a_out__error("SDL_RWFromMem failed: %s", SDL_GetError());
        goto Done;
    }

    sfx = Mix_LoadWAV_RW(rw, 0);
    if(sfx == NULL) {
        a_out__error("Mix_LoadWAV_RW failed: %s", SDL_GetError());
        goto Done;
    }

Done:
    if(rw) {
        SDL_FreeRW(rw);
    }

    return sfx;
}

void a_sdl__sfx_free(void* Sfx)
{
    Mix_FreeChunk(Sfx);
}

void a_sdl__sfx_setVolume(void* Sfx, int Volume)
{
    Mix_VolumeChunk(Sfx, Volume);
}

void a_sdl__sfx_play(void* Sfx)
{
    Mix_PlayChannel(-1, Sfx, 0);
}

uint32_t a_sdl__getTicks(void)
{
    return SDL_GetTicks();
}

void a_sdl__delay(uint32_t Ms)
{
    SDL_Delay(Ms);
}

void a_sdl__input_bind(void)
{
    A_STRHASH_ITERATE(g_buttons, ASdlInputButton*, b) {
        b->input = a_input__newButton(b->header.name);
    }

    A_STRHASH_ITERATE(g_analogs, ASdlInputAnalog*, a) {
        a->input = a_input__newAnalog(a->header.name);
    }

    A_STRHASH_ITERATE(g_touchScreens, ASdlInputTouch*, t) {
        t->input = a_input__newTouch(t->header.name);
    }

    A_LIST_ITERATE(g_controllers, ASdlInputController*, c) {
        a_input__newController();

        A_STRHASH_ITERATE(c->buttons, ASdlInputButton*, b) {
            b->input = a_input__newButton(b->header.name);
        }
    }
}

void a_sdl__input_get(void)
{
    for(SDL_Event event; SDL_PollEvent(&event); ) {
        switch(event.type) {
            case SDL_QUIT: {
                a_state_exit();
            } break;

            case SDL_KEYDOWN: {
                if(event.key.keysym.sym == SDLK_ESCAPE) {
                    a_state_exit();
                    break;
                }

                A_STRHASH_ITERATE(g_buttons, ASdlInputButton*, b) {
                    if(b->code == event.key.keysym.sym) {
                        a_input__button_setState(b->input, true);
                    }
                }
            } break;

            case SDL_KEYUP: {
                A_STRHASH_ITERATE(g_buttons, ASdlInputButton*, b) {
                    if(b->code == event.key.keysym.sym) {
                        a_input__button_setState(b->input, false);
                    }
                }
            } break;

            case SDL_JOYBUTTONDOWN: {
                A_STRHASH_ITERATE(g_buttons, ASdlInputButton*, b) {
                    if(b->code == event.jbutton.button) {
                        a_input__button_setState(b->input, true);
                    }
                }

                A_LIST_ITERATE(g_controllers, ASdlInputController*, c) {
                    if(c->id == event.jbutton.which) {
                        A_STRHASH_ITERATE(c->buttons, ASdlInputButton*, b) {
                            if(b->code == event.jbutton.button) {
                                a_input__button_setState(b->input, true);
                            }
                        }

                        break;
                    }
                }
            } break;

            case SDL_JOYBUTTONUP: {
                A_STRHASH_ITERATE(g_buttons, ASdlInputButton*, b) {
                    if(b->code == event.jbutton.button) {
                        a_input__button_setState(b->input, false);
                    }
                }

                A_LIST_ITERATE(g_controllers, ASdlInputController*, c) {
                    if(c->id == event.jbutton.which) {
                        A_STRHASH_ITERATE(c->buttons, ASdlInputButton*, b) {
                            if(b->code == event.jbutton.button) {
                                a_input__button_setState(b->input, false);
                            }
                        }

                        break;
                    }
                }
            } break;

            case SDL_JOYHATMOTION: {
                unsigned int state = 0;
                #define UP_PRESSED    (1 << 0)
                #define DOWN_PRESSED  (1 << 1)
                #define LEFT_PRESSED  (1 << 2)
                #define RIGHT_PRESSED (1 << 3)

                switch(event.jhat.value) {
                    case SDL_HAT_UP: {
                        state = UP_PRESSED;
                    } break;

                    case SDL_HAT_DOWN: {
                        state = DOWN_PRESSED;
                    } break;

                    case SDL_HAT_LEFT: {
                        state = LEFT_PRESSED;
                    } break;

                    case SDL_HAT_RIGHT: {
                        state = RIGHT_PRESSED;
                    } break;

                    case SDL_HAT_LEFTUP: {
                        state = LEFT_PRESSED | UP_PRESSED;
                    } break;

                    case SDL_HAT_RIGHTUP: {
                        state = RIGHT_PRESSED | UP_PRESSED;
                    } break;

                    case SDL_HAT_LEFTDOWN: {
                        state = LEFT_PRESSED | DOWN_PRESSED;
                    } break;

                    case SDL_HAT_RIGHTDOWN: {
                        state = RIGHT_PRESSED | DOWN_PRESSED;
                    } break;
                }

                A_LIST_ITERATE(g_controllers, ASdlInputController*, c) {
                    if(c->id == event.jhat.which) {
                        ASdlInputButton* buttons[4] = {
                            a_strhash_get(c->buttons, "controller.up"),
                            a_strhash_get(c->buttons, "controller.down"),
                            a_strhash_get(c->buttons, "controller.left"),
                            a_strhash_get(c->buttons, "controller.right")
                        };

                        for(int i = 0; i < 4; i++, state >>= 1) {
                            ASdlInputButton* b = buttons[i];

                            if(state & 1) {
                                if(!b->lastStatePressed) {
                                    b->lastStatePressed = true;
                                    a_input__button_setState(b->input, true);
                                }
                            } else {
                                if(b->lastStatePressed) {
                                    b->lastStatePressed = false;
                                    a_input__button_setState(b->input, false);
                                }
                            }
                        }

                        break;
                    }
                }
            } break;

            case SDL_JOYAXISMOTION: {
                A_STRHASH_ITERATE(g_analogs, ASdlInputAnalog*, a) {
                    if(a->header.device_index == event.jaxis.which) {
                        if(event.jaxis.axis == a->xaxis_index) {
                            a_input__analog_setXAxis(a->input,
                                                     event.jaxis.value);
                        } else if(event.jaxis.axis == a->yaxis_index) {
                            a_input__analog_setYAxis(a->input,
                                                     event.jaxis.value);
                        }
                    }
                }
            } break;

            case SDL_MOUSEMOTION: {
                A_STRHASH_ITERATE(g_touchScreens, ASdlInputTouch*, t) {
                    a_input__touch_addMotion(t->input,
                                             event.button.x,
                                             event.button.y);
                }
            } break;

            case SDL_MOUSEBUTTONDOWN: {
                switch(event.button.button) {
                    case SDL_BUTTON_LEFT: {
                        A_STRHASH_ITERATE(g_touchScreens, ASdlInputTouch*, t) {
                            a_input__touch_setCoords(t->input,
                                                     event.button.x,
                                                     event.button.y,
                                                     true);
                        }
                    } break;
                }
            } break;

            case SDL_MOUSEBUTTONUP: {
                switch(event.button.button) {
                    case SDL_BUTTON_LEFT: {
                        A_STRHASH_ITERATE(g_touchScreens, ASdlInputTouch*, t) {
                            a_input__touch_setCoords(t->input,
                                                     event.button.x,
                                                     event.button.y,
                                                     false);
                        }
                    } break;
                }
            } break;

            default:break;
        }
    }
}
