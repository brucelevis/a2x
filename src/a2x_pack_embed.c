/*
    Copyright 2017-2018 Alex Margarit <alex@alxm.org>
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

#include "a2x_pack_embed.v.h"

#include "a2x_pack_strhash.v.h"

#include "media/console.png.h"
#include "media/fontgrid.png.h"
#include "media/fontgrid_keyed.png.h"

static AStrHash* g_dirs; // table of AEmbeddedDir
static AStrHash* g_files; // table of AEmbeddedFile

static inline void addDir(const char* Path, const void* Data)
{
    a_strhash_add(g_dirs, Path, (void*)Data);
}

static inline void addFile(const char* Path, const void* Data)
{
    a_strhash_add(g_files, Path, (void*)Data);
}

void a_embed__init(void)
{
    g_dirs = a_strhash_new();
    g_files = a_strhash_new();

    addFile("/a2x/consoleTitles", &a__bin__media_console_png);
    addFile("/a2x/font", &a__bin__media_fontgrid_png);
    addFile("/a2x/fontKeyed", &a__bin__media_fontgrid_keyed_png);

    a__embed_application();
}

void a_embed__uninit(void)
{
    a_strhash_free(g_dirs);
    a_strhash_free(g_files);
}

void a__embed_addDir(const void* Data)
{
    addDir(((const AEmbeddedDir*)Data)->path, Data);
}

void a__embed_addFile(const void* Data)
{
    addFile(((const AEmbeddedFile*)Data)->path, Data);
}

const AEmbeddedDir* a_embed__getDir(const char* Path)
{
    return a_strhash_get(g_dirs, Path);
}

const AEmbeddedFile* a_embed__getFile(const char* Path)
{
    return a_strhash_get(g_files, Path);
}
