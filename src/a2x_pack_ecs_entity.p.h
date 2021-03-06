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

#pragma once

#include "a2x_system_includes.h"

typedef struct AEntity AEntity;

extern AEntity* a_entity_new(const char* Id, void* Context);
extern AEntity* a_entity_newEx(const char* Template, const void* ComponentInitContext, void* Context);

extern void a_entity_debugSet(AEntity* Entity, bool DebugOn);

extern const char* a_entity_idGet(const AEntity* Entity);
extern void* a_entity_contextGet(const AEntity* Entity);

extern AEntity* a_entity_parentGet(const AEntity* Entity);
extern void a_entity_parentSet(AEntity* Entity, AEntity* Parent);
extern bool a_entity_parentHas(const AEntity* Child, const AEntity* PotentialParent);

extern void a_entity_refInc(AEntity* Entity);
extern void a_entity_refDec(AEntity* Entity);

extern bool a_entity_removeGet(const AEntity* Entity);
extern void a_entity_removeSet(AEntity* Entity);

extern bool a_entity_activeGet(const AEntity* Entity);
extern void a_entity_activeSet(AEntity* Entity);
extern void a_entity_activeSetPermanent(AEntity* Entity);

extern void* a_entity_componentAdd(AEntity* Entity, int Component);
extern bool a_entity_componentHas(const AEntity* Entity, int Component);
extern void* a_entity_componentGet(const AEntity* Entity, int Component);
extern void* a_entity_componentReq(const AEntity* Entity, int Component);

extern bool a_entity_muteGet(const AEntity* Entity);
extern void a_entity_muteInc(AEntity* Entity);
extern void a_entity_muteDec(AEntity* Entity);
