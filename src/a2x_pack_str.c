/*
    Copyright 2010, 2016, 2017 Alex Margarit

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

#include "a2x_system_includes.h"
#include "a2x_pack_str.v.h"

#include "a2x_pack_mem.v.h"

char* a_str_merge(const char* String1, ...)
{
    va_list args;
    size_t size = 0;

    va_start(args, String1);

    for(const char* s = String1; s != NULL; s = va_arg(args, const char*)) {
        size += strlen(s);
    }

    va_end(args);

    char* string = a_mem_malloc(size + 1);
    char* buffer = string;

    va_start(args, String1);

    for(const char* s = String1; s != NULL; s = va_arg(args, const char*)) {
        while(*s != '\0') {
            *buffer++ = *s++;
        }
    }

    *buffer = '\0';

    va_end(args);

    return string;
}

char* a_str_dup(const char* String)
{
    if(String == NULL) {
        return NULL;
    }

    char* buffer = a_mem_malloc(strlen(String) + 1);

    return strcpy(buffer, String);
}

char* a_str_trim(const char* String)
{
    int start = 0;
    int end = (int)strlen(String) - 1;

    while(isspace(String[start])) {
        start++;
    }

    while(end > start && isspace(String[end])) {
        end--;
    }

    return a_str_getSub(String, start, end + 1);
}

char* a_str_getSub(const char* String, int Start, int End)
{
    size_t len = (size_t)(End - Start);
    char* str = a_mem_malloc(len + 1);

    memcpy(str, String + Start, len);
    str[len] = '\0';

    return str;
}

char* a_str_getPrefix(const char* String, int Length)
{
    return a_str_getSub(String, 0, Length);
}

char* a_str_getSuffix(const char* String, int Length)
{
    int sLen = (int)strlen(String);
    return a_str_getSub(String, sLen - Length, sLen);
}

int a_str_getFirstIndex(const char* String, char Character)
{
    for(int i = 0; String[i] != '\0'; i++) {
        if(String[i] == Character) {
            return i;
        }
    }

    return -1;
}

int a_str_getLastIndex(const char* String, char Character)
{
    for(int i = (int)strlen(String); i--; ) {
        if(String[i] == Character) {
            return i;
        }
    }

    return -1;
}

bool a_str_startsWith(const char* String, const char* Prefix)
{
    while(*String != '\0' && *Prefix != '\0') {
        if(*String++ != *Prefix++) {
            return false;
        }
    }

    return *Prefix == '\0';
}

bool a_str_endsWith(const char* String, const char* Suffix)
{
    const size_t str_len = strlen(String);
    const size_t suf_len = strlen(Suffix);

    if(suf_len > str_len) {
        return false;
    }

    return strcmp(String + str_len - suf_len, Suffix) == 0;
}

char* a_str_getPrefixFirstFind(const char* String, char Marker)
{
    const int index = a_str_getFirstIndex(String, Marker);

    if(index == -1) {
        return NULL;
    }

    return a_str_getSub(String, 0, index);
}

char* a_str_getPrefixLastFind(const char* String, char Marker)
{
    const int index = a_str_getLastIndex(String, Marker);

    if(index == -1) {
        return NULL;
    }

    return a_str_getSub(String, 0, index);
}

char* a_str_getSuffixFirstFind(const char* String, char Marker)
{
    const int start = a_str_getFirstIndex(String, Marker) + 1;
    int end = start;

    if(start == 0) {
        return NULL;
    }

    while(String[end] != '\0') {
        end++;
    }

    return a_str_getSub(String, start, end);
}

char* a_str_getSuffixLastFind(const char* String, char Marker)
{
    const int index = a_str_getLastIndex(String, Marker);

    if(index == -1) {
        return NULL;
    }

    return a_str_getSub(String, index + 1, (int)strlen(String));
}

char* a_str_extractPath(const char* String)
{
    char* path = a_str_getPrefixLastFind(String, '/');

    if(path) {
        return path;
    } else {
        return a_str_dup(".");
    }
}

char* a_str_extractFile(const char* String)
{
    char* c = a_str_getSuffixLastFind(String, '/');

    if(c) {
        return c;
    } else {
        return a_str_dup(String);
    }
}

char* a_str_extractName(const char* String)
{
    char* file = a_str_extractFile(String);
    char* name = a_str_getPrefixLastFind(file, '.');

    if(name) {
        free(file);
        return name;
    } else {
        return file;
    }
}

AList* a_str_split(const char* String, const char* Delimiters)
{
    AList* strings = a_list_new();

    const char* str = String;
    const char* start = String;

    while(*str != '\0') {
        for(const char* d = Delimiters; *d != '\0'; d++) {
            if(*str == *d) {
                if(str > start) {
                    size_t len = (size_t)(str - start);
                    char* split = a_mem_malloc(len + 1);

                    memcpy(split, start, len);
                    split[len] = '\0';

                    a_list_addLast(strings, split);
                }

                start = str + 1;
                break;
            }
        }

        str++;
    }

    if(str > start) {
        size_t len = (size_t)(str - start);
        char* split = a_mem_malloc(len + 1);

        memcpy(split, start, len);
        split[len] = '\0';

        a_list_addLast(strings, split);
    }

    return strings;
}
