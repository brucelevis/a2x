/*
    Copyright 2011, 2016-2019 Alex Margarit <alex@alxm.org>
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

#include "a2x_pack_dir.v.h"

#include <dirent.h>
#include <sys/stat.h>

#include "a2x_pack_embed.v.h"
#include "a2x_pack_listit.v.h"
#include "a2x_pack_mem.v.h"
#include "a2x_pack_out.v.h"
#include "a2x_pack_path.v.h"
#include "a2x_pack_str.v.h"

struct ADir {
    APath* path;
    AList* files; // list of APath
};

static int dirSort(const APath* A, const APath* B)
{
    const char* nameA = a_path_getName(A);
    const char* nameB = a_path_getName(B);
    int a = *nameA;
    int b = *nameB;
    int lowerCaseUpperCaseCmp = 0;

    while(a != '\0' && b != '\0') {
        if(a != b) {
            int lowerA = tolower(a);
            int lowerB = tolower(b);

            if(lowerA == lowerB) {
                if(lowerCaseUpperCaseCmp == 0) {
                    lowerCaseUpperCaseCmp = b - a;
                }
            } else {
                return lowerA - lowerB;
            }
        }

        a = *++nameA;
        b = *++nameB;
    }

    if(a == b) {
        return lowerCaseUpperCaseCmp;
    }

    return a - b;
}

static AList* dirReal(APath* Path)
{
    const char* path = a_path_getFull(Path);
    DIR* dir = opendir(path);

    if(dir == NULL) {
        #if A_CONFIG_SYSTEM_LINUX
            int result = mkdir(path, S_IRWXU);
        #else
            int result = mkdir(path);
        #endif

        if(result == -1) {
            a_out__error("a_dir_new: mkdir(%s) failed", path);
            return NULL;
        }

        dir = opendir(path);

        if(dir != NULL) {
            a_path__flagsSet(Path, A_PATH_DIR | A_PATH_REAL);
        }
    }

    if(dir == NULL) {
        a_out__error("a_dir_new: opendir(%s) failed", path);
        return NULL;
    }

    AList* files = a_list_new();

    for(struct dirent* ent = readdir(dir); ent; ent = readdir(dir)) {
        if(ent->d_name[0] != '.') {
            a_list_addLast(files, a_path_newf("%s/%s", path, ent->d_name));
        }
    }

    a_list_sort(files, (AListCompare*)dirSort);

    closedir(dir);

    return files;
}

static AList* dirEmbedded(APath* Path)
{
    const char* path = a_path_getFull(Path);
    const AEmbeddedDir* data = a_embed__getDir(path);

    if(data == NULL) {
        return NULL;
    }

    AList* files = a_list_new();

    for(size_t e = data->size; e--; ) {
        a_list_addLast(files, a_path_newf("%s/%s", path, data->entries[e]));
    }

    return files;
}

ADir* a_dir_new(const char* Path)
{
    AList* files = NULL;
    APath* path = a_path_new(Path);

    if(a_path_test(path, A_PATH_DIR | A_PATH_REAL)) {
        files = dirReal(path);
    } else if(a_path_test(path, A_PATH_DIR | A_PATH_EMBEDDED)) {
        files = dirEmbedded(path);
    } else {
        files = dirReal(path);
    }

    if(files == NULL) {
        a_path_free(path);

        return NULL;
    }

    ADir* d = a_mem_malloc(sizeof(ADir));

    d->path = path;
    d->files = files;

    return d;
}

void a_dir_free(ADir* Dir)
{
    if(Dir == NULL) {
        return;
    }

    a_list_freeEx(Dir->files, (AFree*)a_path_free);
    a_path_free(Dir->path);

    free(Dir);
}

const APath* a_dir_pathGet(const ADir* Dir)
{
    return Dir->path;
}

AList* a_dir_entriesListGet(const ADir* Dir)
{
    return Dir->files;
}

unsigned a_dir_entriesNumGet(const ADir* Dir)
{
    return a_list_sizeGet(Dir->files);
}
