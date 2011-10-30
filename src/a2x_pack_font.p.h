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

#ifndef A2X_PACK_FONT_PH
#define A2X_PACK_FONT_PH

#include "a2x_app_includes.h"

#include "a2x_pack_sheet.p.h"

typedef enum FontLoad  {
    A_LOAD_ALL = 1, A_LOAD_AN = 2, A_LOAD_A = 4, A_LOAD_N = 8, A_LOAD_CAPS = 16
} FontLoad;

typedef enum FontAlign {
    A_LEFT = 1, A_MIDDLE = 2, A_RIGHT = 4, A_SPACED = 8, A_SAFE = 16
} FontAlign;

extern int a_font_load(Sheet* sheet, int sx, int sy, int zoom, FontLoad loader);
extern int a_font_copy(int font, uint8_t r, uint8_t g, uint8_t b);

extern int a_font_text(FontAlign align, int x, int y, int index, const char* text);
extern int a_font_safe(FontAlign align, int x, int y, int index, const char* text);
extern int a_font_int(FontAlign align, int x, int y, int f, int number);
extern int a_font_float(FontAlign align, int x, int y, int f, float number);
extern int a_font_char(FontAlign align, int x, int y, int f, char ch);

extern int a_font_fixed(FontAlign align, int x, int y, int f, int width, const char* text);

extern int a_font_width(int index, const char* text);

#define a_font_textf(align, x, y, f, ...) \
({                                        \
    char a__s[256];                       \
    sprintf(a__s, __VA_ARGS__);           \
    a_font_text(align, x, y, f, a__s);    \
})

#define a_font_widthf(f, ...)   \
({                              \
    char a__s[256];             \
    sprintf(a__s, __VA_ARGS__); \
    a_font_width(f, a__s);      \
})

#endif // A2X_PACK_FONT_PH
