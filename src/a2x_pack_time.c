/*
    Copyright 2010, 2016, 2018 Alex Margarit

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
#include "a2x_pack_time.v.h"

#include "a2x_pack_platform.v.h"

uint32_t a_time_getMs(void)
{
    return a_platform__getMs();
}

void a_time_waitMs(uint32_t Ms)
{
    a_platform__waitMs(Ms);
}

void a_time_waitSec(uint32_t Sec)
{
    a_platform__waitMs(Sec * 1000);
}

void a_time_spinMs(uint32_t Ms)
{
    const uint32_t start = a_platform__getMs();

    while(a_platform__getMs() - start < Ms) {
        continue;
    }
}

void a_time_spinSec(uint32_t Sec)
{
    a_time_spinMs(Sec * 1000);
}
