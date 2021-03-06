/*
    Copyright 2017 Alex Margarit <alex@alxm.org>
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

#include "a2x_pack_embed.p.h"

typedef struct {
    const char* path;
    size_t size;
    const char* entries[];
} AEmbeddedDir;

typedef struct {
    const char* path;
    size_t size;
    uint8_t buffer[];
} AEmbeddedFile;

extern void a_embed__init(void);
extern void a_embed__uninit(void);

extern const AEmbeddedDir* a_embed__getDir(const char* Path);
extern const AEmbeddedFile* a_embed__getFile(const char* Path);
