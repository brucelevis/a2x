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

#pragma once

#include "a2x_pack_screen.p.h"

#include "a2x_pack_platform.v.h"

struct AScreen {
    APixel* pixels;
    size_t pixelsSize;
    ASprite* sprite;
    APlatformTexture* texture;
    int width;
    int height;
    int clipX;
    int clipY;
    int clipX2;
    int clipY2;
    int clipWidth;
    int clipHeight;
    bool ownsBuffer;
};

extern AScreen a__screen;

extern void a_screen__init(void);
extern void a_screen__uninit(void);

extern void a_screen__tick(void);
extern void a_screen__draw(void);

extern int a_screen__zoomGet(void);
extern void a_screen__zoomSet(int Zoom);

extern void a_screen_fullscreenFlip(void);

extern bool a_screen__sameSize(const AScreen* Screen1, const AScreen* Screen2);
