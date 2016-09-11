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

#include "a2x_pack_pixel.v.h"

APixelMode a_pixel__mode;
static AList* g_modeStack;

void a_pixel__init(void)
{
    a_pixel__mode.blend = A_PIXEL_BLEND_PLAIN;
    g_modeStack = a_list_new();
}

void a_pixel__uninit(void)
{
    A_LIST_ITERATE(g_modeStack, APixelMode*, mode) {
        free(mode);
    }

    a_list_free(g_modeStack);
}

void a_pixel_push(void)
{
    APixelMode* mode = a_mem_malloc(sizeof(APixelMode));

    *mode = a_pixel__mode;
    a_list_push(g_modeStack, mode);
}

void a_pixel_pop(void)
{
    APixelMode* mode = a_list_pop(g_modeStack);

    if(mode == NULL) {
        a_out__fatal("Cannot pop APixelMode: stack is empty");
    }

    a_pixel__mode = *mode;
    free(mode);

    a_pixel_setBlend(a_pixel__mode.blend);
    a_pixel_setRGBA(a_pixel__mode.red, a_pixel__mode.green, a_pixel__mode.blue, a_pixel__mode.alpha);
}

void a_pixel_setBlend(APixelBlend Blend)
{
    a_pixel__mode.blend = Blend;

    a_sprite__updateRoutines();
    a_draw__updateRoutines();
}

void a_pixel_setAlpha(unsigned int Alpha)
{
    a_pixel__mode.alpha = a_math_min(Alpha, A_PIXEL_ALPHA_MAX);
}

void a_pixel_setRGB(uint8_t Red, uint8_t Green, uint8_t Blue)
{
    a_pixel__mode.red = Red;
    a_pixel__mode.green = Green;
    a_pixel__mode.blue = Blue;

    a_pixel__mode.pixel = a_pixel_make(a_pixel__mode.red, a_pixel__mode.green, a_pixel__mode.blue);
}

void a_pixel_setRGBA(uint8_t Red, uint8_t Green, uint8_t Blue, unsigned int Alpha)
{
    a_pixel__mode.red = Red;
    a_pixel__mode.green = Green;
    a_pixel__mode.blue = Blue;
    a_pixel__mode.alpha = a_math_min(Alpha, A_PIXEL_ALPHA_MAX);

    a_pixel__mode.pixel = a_pixel_make(a_pixel__mode.red, a_pixel__mode.green, a_pixel__mode.blue);
}

void a_pixel_setPixel(APixel Pixel)
{
    a_pixel__mode.pixel = Pixel;

    a_pixel__mode.red = a_pixel_red(a_pixel__mode.pixel);
    a_pixel__mode.green = a_pixel_green(a_pixel__mode.pixel);
    a_pixel__mode.blue = a_pixel_blue(a_pixel__mode.pixel);
}
