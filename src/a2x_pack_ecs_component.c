/*
    Copyright 2016-2019 Alex Margarit <alex@alxm.org>
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

#include "a2x_pack_ecs_component.v.h"

#include "a2x_pack_ecs_entity.v.h"
#include "a2x_pack_main.v.h"
#include "a2x_pack_mem.v.h"
#include "a2x_pack_strhash.v.h"

unsigned a_component__tableLen;
static AComponent* g_componentsTable;

static AStrHash* g_components; // table of AComponent

static inline AComponentInstance* getHeader(const void* Component)
{
    return (AComponentInstance*)Component - 1;
}

void a_component__init(unsigned NumComponents)
{
    g_components = a_strhash_new();

    a_component__tableLen = NumComponents;
    g_componentsTable = a_mem_zalloc(NumComponents * sizeof(AComponent));

    while(NumComponents--) {
        g_componentsTable[NumComponents].stringId = "???";
        g_componentsTable[NumComponents].bit = UINT_MAX;
    }
}

void a_component__uninit(void)
{
    a_strhash_free(g_components);
    free(g_componentsTable);
}

int a_component__stringToIndex(const char* StringId)
{
    const AComponent* c = a_strhash_get(g_components, StringId);

    return c ? (int)c->bit : -1;
}

const AComponent* a_component__get(int Component, const char* CallerFunction)
{
    #if A_CONFIG_BUILD_DEBUG
        if(g_componentsTable == NULL) {
            A__FATAL("%s: Call a_ecs_init first", CallerFunction);
        }

        if(Component < 0 || Component >= (int)a_component__tableLen) {
            A__FATAL("%s: Unknown component %d", CallerFunction, Component);
        }

        if(g_componentsTable[Component].bit == UINT_MAX) {
            A__FATAL(
                "%s: Uninitialized component %d", CallerFunction, Component);
        }
    #else
        A_UNUSED(CallerFunction);
    #endif

    return &g_componentsTable[Component];
}

void a_component_new(int Index, const char* StringId, size_t Size, AInit* Init, AFree* Free)
{
    if(g_componentsTable == NULL) {
        A__FATAL(
            "a_component_new(%d, %s): Call a_ecs_init first", Index, StringId);
    }

    AComponent* c = &g_componentsTable[Index];

    if(c->bit != UINT_MAX || a_strhash_contains(g_components, StringId)) {
        A__FATAL("a_component_new(%d, %s): Already declared", Index, StringId);
    }

    c->size = sizeof(AComponentInstance) + Size;
    c->init = Init;
    c->free = Free;
    c->stringId = StringId;
    c->bit = (unsigned)Index;

    a_strhash_add(g_components, StringId, c);
}

void a_component_newEx(int Index, const char* StringId, size_t Size, AInitWithData* InitWithData, AFree* Free, size_t DataSize, AComponentDataInit* DataInit, AFree* DataFree)
{
    a_component_new(Index, StringId, Size, NULL, Free);

    AComponent* c = &g_componentsTable[Index];

    c->initWithData = InitWithData;
    c->dataSize = DataSize;
    c->dataInit = DataInit;
    c->dataFree = DataFree;
}

const void* a_component_dataGet(const void* Component)
{
    AComponentInstance* h = getHeader(Component);

    return a_template__dataGet(
            h->entity->template, (int)(h->component - g_componentsTable));
}

AEntity* a_component_entityGet(const void* Component)
{
    return getHeader(Component)->entity;
}
