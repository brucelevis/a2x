/*
    Copyright 2010 Alex Margarit

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

#ifndef A2X_APP_MAIN_VH
#define A2X_APP_MAIN_VH

#define A__VERSION "rolling"

#include "a2x_app_main.p.h"

#include "a2x_app_settings.v.h"

#include "a2x_pack_blit.v.h"
#include "a2x_pack_conf.v.h"
#include "a2x_pack_draw.v.h"
#include "a2x_pack_fix.v.h"
#include "a2x_pack_font.v.h"
#include "a2x_pack_fps.v.h"
#include "a2x_pack_hw.v.h"
#include "a2x_pack_input.v.h"
#include "a2x_pack_math.v.h"
#include "a2x_pack_mem.v.h"
#include "a2x_pack_pixel.v.h"
#include "a2x_pack_screen.v.h"
#include "a2x_pack_screenshot.v.h"
#include "a2x_pack_sdl.v.h"
#include "a2x_pack_sound.v.h"
#include "a2x_pack_sprite.v.h"
#include "a2x_pack_state.v.h"
#include "a2x_pack_time.v.h"

#if A_PLATFORM_LINUXPC
    #define a_out(...)                                                   \
    ({                                                                   \
        if(!a2x_bool("app.quiet")) {                                     \
            printf("%c[%d;%dm[ a2x Msg ]%c[%dm ", 0x1b, 0, 32, 0x1b, 0); \
            printf(__VA_ARGS__);                                         \
            printf("\n");                                                \
        }                                                                \
    })

    #define a_error(...)                                                      \
    ({                                                                        \
        fprintf(stderr, "%c[%d;%dm[ a2x Err ]%c[%dm ", 0x1b, 0, 31, 0x1b, 0); \
        fprintf(stderr, __VA_ARGS__);                                         \
        fprintf(stderr, "\n");                                                \
    })
#else
    #define a_out(...)               \
    ({                               \
        if(!a2x_bool("app.quiet")) { \
            printf("[ a2x Msg ] ");  \
            printf(__VA_ARGS__);     \
            printf("\n");            \
        }                            \
    })

    #define a_error(...)                 \
    ({                                   \
        fprintf(stderr, "[ a2x Err ] "); \
        fprintf(stderr, __VA_ARGS__);    \
        fprintf(stderr, "\n");           \
    })
#endif

#endif // A2X_APP_MAIN_VH
