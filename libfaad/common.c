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
** $Id: common.c,v 1.3 2002/09/27 08:37:22 menno Exp $
**/

/* just some common functions that could be used anywhere */

#include "common.h"

#include "syntax.h"

/* Returns the sample rate index based on the samplerate */
uint8_t get_sr_index(uint32_t samplerate)
{
    if (92017 <= samplerate) return 0;
    if (75132 <= samplerate) return 1;
    if (55426 <= samplerate) return 2;
    if (46009 <= samplerate) return 3;
    if (37566 <= samplerate) return 4;
    if (27713 <= samplerate) return 5;
    if (23004 <= samplerate) return 6;
    if (18783 <= samplerate) return 7;
    if (13856 <= samplerate) return 8;
    if (11502 <= samplerate) return 9;
    if (9391 <= samplerate) return 10;

    return 11;
}

/* Returns 0 if an object type is decodable, otherwise returns -1 */
int8_t can_decode_ot(uint8_t object_type)
{
    switch (object_type)
    {
    case LC:
        return 0;
    case MAIN:
#ifdef MAIN_DEC
        return 0;
#else
        return -1;
#endif
    case SSR:
        return -1;
    case LTP:
#ifdef LTP_DEC
        return 0;
#else
        return -1;
#endif

    /* ER object types */
#ifdef ERROR_RESILIENCE
    case ER_LC:
#ifdef DRM
    case DRM_ER_LC:
#endif
        return 0;
    case ER_LTP:
#ifdef LTP_DEC
        return 0;
#else
        return -1;
#endif
    case LD:
#ifdef LD_DEC
        return 0;
#else
        return -1;
#endif
#endif
    }

    return -1;
}

#if 0

#define LOG2 0.30102999566398

uint32_t int_log2(uint32_t val)
{
    return (uint32_t)(log((real_t)val)/LOG2 + 0.5);
}
#endif

