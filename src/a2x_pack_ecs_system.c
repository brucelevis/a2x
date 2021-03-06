/*
    Copyright 2016-2018 Alex Margarit <alex@alxm.org>
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

#include "a2x_pack_ecs_system.v.h"

#include "a2x_pack_ecs.v.h"
#include "a2x_pack_ecs_entity.v.h"
#include "a2x_pack_listit.v.h"
#include "a2x_pack_main.v.h"
#include "a2x_pack_mem.v.h"

unsigned a_system__tableLen;
static ASystem* g_systemsTable;

void a_system__init(unsigned NumSystems)
{
    a_system__tableLen = NumSystems;
    g_systemsTable = a_mem_malloc(NumSystems * sizeof(ASystem));

    while(NumSystems--) {
        g_systemsTable[NumSystems].entities = NULL;
    }
}

void a_system__uninit(void)
{
    for(unsigned s = a_system__tableLen; s--; ) {
        a_list_free(g_systemsTable[s].entities);
        a_bitfield_free(g_systemsTable[s].componentBits);
    }

    free(g_systemsTable);
}

ASystem* a_system__get(int System, const char* CallerFunction)
{
    #if A_CONFIG_BUILD_DEBUG
        if(g_systemsTable == NULL) {
            A__FATAL("%s: Call a_ecs_init first", CallerFunction);
        }

        if(System < 0 || System >= (int)a_system__tableLen) {
            A__FATAL("%s: Unknown system %d", CallerFunction, System);
        }

        if(g_systemsTable[System].entities == NULL) {
            A__FATAL("%s: Uninitialized system %d", CallerFunction, System);
        }
    #else
        A_UNUSED(CallerFunction);
    #endif

    return &g_systemsTable[System];
}

void a_system_new(int Index, ASystemHandler* Handler, ASystemSort* Compare, bool OnlyActiveEntities)
{
    if(g_systemsTable == NULL) {
        A__FATAL("a_system_new(%d): Call a_ecs_init first", Index);
    }

    if(g_systemsTable[Index].entities != NULL) {
        A__FATAL("a_system_new(%d): Already declared", Index);
    }

    ASystem* s = &g_systemsTable[Index];

    s->handler = Handler;
    s->compare = Compare;
    s->entities = a_list_new();
    s->componentBits = a_bitfield_new(a_component__tableLen);
    s->onlyActiveEntities = OnlyActiveEntities;
}

void a_system_add(int System, int Component)
{
    ASystem* s = a_system__get(System, __func__);
    const AComponent* c = a_component__get(Component, __func__);

    a_bitfield_set(s->componentBits, c->bit);
}

void a_system_run(int System)
{
    ASystem* system = a_system__get(System, __func__);

    if(system->compare) {
        a_list_sort(system->entities, (AListCompare*)system->compare);
    }

    if(system->onlyActiveEntities) {
        A_LIST_ITERATE(system->entities, AEntity*, entity) {
            if(a_entity_activeGet(entity)) {
                system->handler(entity);
            } else {
                a_entity__removeFromActiveSystems(entity);
            }
        }
    } else {
        A_LIST_ITERATE(system->entities, AEntity*, entity) {
            system->handler(entity);
        }
    }

    a_ecs__flushEntitiesFromSystems();
}
