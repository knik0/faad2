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
** $Id: is.h,v 1.4 2002/06/13 11:03:27 menno Exp $
**/

#ifndef __IS_H__
#define __IS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "syntax.h"

void is_decode(ic_stream *ics, ic_stream *icsr, real_t *l_spec, real_t *r_spec,
               uint16_t frame_len);

static INLINE int8_t is_intensity(ic_stream *ics, uint8_t group, uint8_t sfb)
{
    switch (ics->sfb_cb[group][sfb])
    {
    case INTENSITY_HCB:
        return 1;
    case INTENSITY_HCB2:
        return -1;
    default:
        return 0;
    }
}

static INLINE int8_t invert_intensity(ic_stream *ics, uint8_t group, uint8_t sfb)
{
    if (ics->ms_mask_present == 1)
        return (1-2*ics->ms_used[group][sfb]);
    return 1;
}


#ifdef __cplusplus
}
#endif
#endif
