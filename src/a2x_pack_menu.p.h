/*
    Copyright 2010, 2016, 2018 Alex Margarit <alex@alxm.org>
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

typedef struct AMenu AMenu;

typedef enum {
    A_MENU_STATE_INVALID = -1,
    A_MENU_STATE_RUNNING,
    A_MENU_STATE_SELECTED,
    A_MENU_STATE_CANCELED
} AMenuState;

#include "a2x_pack_input_button.p.h"
#include "a2x_pack_list.p.h"
#include "a2x_pack_sound.p.h"

extern AMenu* a_menu_new(AButton* Next, AButton* Back, AButton* Select, AButton* Cancel);
extern void a_menu_free(AMenu* Menu);
extern void a_menu_freeEx(AMenu* Menu, AFree* ItemFree);

extern void a_menu_soundSet(AMenu* Menu, ASample* Accept, ASample* Cancel, ASample* Browse);
extern void a_menu_itemAdd(AMenu* Menu, void* Item);

extern void a_menu_tick(AMenu* Menu);
extern AMenuState a_menu_stateGet(const AMenu* Menu);

extern AList* a_menu_itemsListGet(const AMenu* Menu);
extern bool a_menu_itemIsSelected(const AMenu* Menu, const void* Item);
extern unsigned a_menu_selectedIndexGet(const AMenu* Menu);
extern void* a_menu_itemGetSelected(const AMenu* Menu);

extern void a_menu_keepRunning(AMenu* Menu);
extern void a_menu_reset(AMenu* Menu);
