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
** $Id: bits.c,v 1.2 2002/01/19 09:39:41 menno Exp $
**/

#include "bits.h"


/* initialize buffer, call once before first getbits or showbits */
void faad_initbits(bitfile *ld, unsigned char *buffer)
{
    ld->incnt = 0;
    ld->framebits = 0;
    ld->bitcnt = 0;
    ld->rdptr = buffer;
}

int faad_get_processed_bits(bitfile *ld)
{
    return (ld->framebits);
}

unsigned int faad_byte_align(bitfile *ld)
{
    int i = 0;

    while (ld->framebits%8 != 0)
    {
        faad_get1bit(ld DEBUGVAR(1,0,"faad_byte_align(): get bit until aligned"));
        i++;
    }

    return(i);
}
