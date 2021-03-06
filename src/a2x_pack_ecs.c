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

#include "a2x_pack_ecs.v.h"

#include "a2x_pack_ecs_collection.v.h"
#include "a2x_pack_ecs_component.v.h"
#include "a2x_pack_ecs_system.v.h"
#include "a2x_pack_listit.v.h"
#include "a2x_pack_mem.v.h"
#include "a2x_pack_out.v.h"

static AList* g_lists[A_ECS__NUM]; // Each entity is in exactly one of these
static bool g_deleting; // Set at uninit time to prevent using freed entities
static ACollection* g_collection; // New entities are added to this collection

void a_ecs__init(void)
{
    for(int i = A_ECS__NUM; i--; ) {
        g_lists[i] = a_list_new();
    }
}

void a_ecs__uninit(void)
{
    g_deleting = true;

    for(int i = A_ECS__NUM; i--; ) {
        a_list_freeEx(g_lists[i], (AFree*)a_entity__free);
    }

    a_template__uninit();
    a_system__uninit();
    a_component__uninit();
}

void a_ecs_init(unsigned NumComponents, unsigned NumSystems)
{
    a_component__init(NumComponents);
    a_system__init(NumSystems);
    a_template__init();
}

ACollection* a_ecs_collectionGet(void)
{
    return g_collection;
}

void a_ecs_collectionSet(ACollection* Collection)
{
    g_collection = Collection;
}

bool a_ecs__isDeleting(void)
{
    return g_deleting;
}

void a_ecs__tick(void)
{
    a_ecs__flushEntitiesFromSystems();

    // Check what systems the new entities match
    A_LIST_ITERATE(g_lists[A_ECS__NEW], AEntity*, e) {
        for(unsigned id = a_system__tableLen; id--; ) {
            ASystem* s = a_system__get((int)id, __func__);

            if(a_bitfield_testMask(e->componentBits, s->componentBits)) {
                if(s->onlyActiveEntities) {
                    a_list_addLast(e->matchingSystemsActive, s);
                } else {
                    a_list_addLast(e->matchingSystemsRest, s);
                }
            }
        }

        a_ecs__entityAddToList(e, A_ECS__RESTORE);
    }

    // Add entities to the systems they match
    A_LIST_ITERATE(g_lists[A_ECS__RESTORE], AEntity*, e) {
        if(!A_FLAG_TEST_ANY(e->flags, A_ENTITY__ACTIVE_REMOVED)) {
            A_LIST_ITERATE(e->matchingSystemsActive, ASystem*, system) {
                a_list_addLast(
                    e->systemNodesActive, a_list_addLast(system->entities, e));
            }
        }

        A_LIST_ITERATE(e->matchingSystemsRest, ASystem*, system) {
            a_list_addLast(
                e->systemNodesEither, a_list_addLast(system->entities, e));
        }

        a_ecs__entityAddToList(e, A_ECS__DEFAULT);
    }

    a_list_clear(g_lists[A_ECS__NEW]);
    a_list_clear(g_lists[A_ECS__RESTORE]);
    a_list_clearEx(g_lists[A_ECS__REMOVED_FREE], (AFree*)a_entity__free);
}

bool a_ecs__entityIsInList(const AEntity* Entity, AEcsListId List)
{
    return a_list__nodeGetList(Entity->node) == g_lists[List];
}

void a_ecs__entityAddToList(AEntity* Entity, AEcsListId List)
{
    Entity->node = a_list_addLast(g_lists[List], Entity);
}

void a_ecs__entityMoveToList(AEntity* Entity, AEcsListId List)
{
    a_list_removeNode(Entity->node);
    Entity->node = a_list_addLast(g_lists[List], Entity);
}

void a_ecs__flushEntitiesFromSystems(void)
{
    A_LIST_ITERATE(g_lists[A_ECS__MUTED_QUEUE], AEntity*, e) {
        a_entity__removeFromAllSystems(e);

        a_ecs__entityAddToList(e, A_ECS__DEFAULT);
    }

    A_LIST_ITERATE(g_lists[A_ECS__REMOVED_QUEUE], AEntity*, e) {
        a_entity__removeFromAllSystems(e);

        if(e->references == 0) {
            a_ecs__entityAddToList(e, A_ECS__REMOVED_FREE);
        } else {
            a_ecs__entityAddToList(e, A_ECS__REMOVED_LIMBO);
        }
    }

    a_list_clear(g_lists[A_ECS__MUTED_QUEUE]);
    a_list_clear(g_lists[A_ECS__REMOVED_QUEUE]);
}
