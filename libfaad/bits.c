/*
** FAAD - Freeware Advanced Audio Decoder
** Copyright (C) 2002 M. Bakker
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** $Id: bits.c,v 1.5 2002/02/18 10:01:05 menno Exp $
**/

#include "common.h"
#include "bits.h"

/* initialize buffer, call once before first getbits or showbits */
void faad_initbits(bitfile *ld, void *buffer)
{
    uint32_t tmp;

    ld->start = (uint32_t*)buffer;

    tmp = *(uint32_t*)buffer;
#ifndef ARCH_IS_BIG_ENDIAN
    BSWAP(tmp);
#endif
    ld->bufa = tmp;

    tmp = *((uint32_t*)buffer + 1);
#ifndef ARCH_IS_BIG_ENDIAN
    BSWAP(tmp);
#endif
    ld->bufb = tmp;

    ld->pos = 0;
    ld->tail = ((uint32_t*)buffer + 2);
}

uint32_t faad_get_processed_bits(bitfile *ld)
{
    return 8 * (4*(ld->tail - ld->start) - 4) - (32 - ld->pos);
}

uint8_t faad_byte_align(bitfile *ld)
{
    uint8_t remainder = (uint8_t)(ld->pos % 8);

    if (remainder)
    {
        faad_flushbits(ld, 8 - remainder);
        return (8 - remainder);
    }
    return 0;
}
