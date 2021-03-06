/*
    Copyright 2016 Alex Margarit <alex@alxm.org>
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

#include "a2x_pack_bitfield.v.h"

#include "a2x_pack_main.v.h"
#include "a2x_pack_mem.v.h"

typedef unsigned long AChunk;
#define A__BITS_PER_CHUNK (unsigned)(sizeof(AChunk) * 8)
#define a__BITS_PER_CHUNK_MASK (A__BITS_PER_CHUNK - 1)

struct ABitfield {
    unsigned numChunks;
    AChunk bits[];
};

ABitfield* a_bitfield_new(unsigned NumBits)
{
    if(NumBits < 1) {
        A__FATAL("a_bitfield_new(0): Invalid size");
    }

    unsigned numChunks = (NumBits + A__BITS_PER_CHUNK - 1) / A__BITS_PER_CHUNK;
    ABitfield* b = a_mem_zalloc(sizeof(ABitfield) + numChunks * sizeof(AChunk));

    b->numChunks = numChunks;

    return b;
}

void a_bitfield_free(ABitfield* Bitfield)
{
    free(Bitfield);
}

void a_bitfield_set(ABitfield* Bitfield, unsigned Bit)
{
    AChunk bit = (AChunk)1 << (Bit & a__BITS_PER_CHUNK_MASK);
    Bitfield->bits[Bit / A__BITS_PER_CHUNK] |= bit;
}

void a_bitfield_clear(ABitfield* Bitfield, unsigned Bit)
{
    AChunk bit = (AChunk)1 << (Bit & a__BITS_PER_CHUNK_MASK);
    Bitfield->bits[Bit / A__BITS_PER_CHUNK] &= ~bit;
}

void a_bitfield_reset(ABitfield* Bitfield)
{
    memset(Bitfield->bits, 0, Bitfield->numChunks * sizeof(AChunk));
}

bool a_bitfield_test(const ABitfield* Bitfield, unsigned Bit)
{
    AChunk value = Bitfield->bits[Bit / A__BITS_PER_CHUNK];
    AChunk bit = (AChunk)1 << (Bit & a__BITS_PER_CHUNK_MASK);

    return (value & bit) != 0;
}

bool a_bitfield_testMask(const ABitfield* Bitfield, const ABitfield* Mask)
{
    for(unsigned i = Mask->numChunks; i--; ) {
        if((Bitfield->bits[i] & Mask->bits[i]) != Mask->bits[i]) {
            return false;
        }
    }

    return true;
}
