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
** $Id: huffman.h,v 1.5 2002/02/18 10:01:05 menno Exp $
**/

#ifndef __HUFFMAN_H__
#define __HUFFMAN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#ifdef ANALYSIS
#include <stdio.h>
#endif
#include "bits.h"
#include "codebook/hcb.h"


static INLINE uint8_t huffman_scale_factor(bitfile *ld)
{
    uint16_t offset = 0;

    while (hcb_sf[offset][1])
    {
        uint8_t b = faad_get1bit(ld);
        offset += hcb_sf[offset][b];
    }
    return hcb_sf[offset][0];
}


static hcb *hcb_table[] = {
    0, hcb1_1, hcb2_1, 0, hcb4_1, 0, hcb6_1, 0, hcb8_1, 0, hcb10_1, hcb11_1
};

static hcb_2_quad *hcb_2_quad_table[] = {
    0, hcb1_2, hcb2_2, 0, hcb4_2, 0, 0, 0, 0, 0, 0, 0
};

static hcb_2_pair *hcb_2_pair_table[] = {
    0, 0, 0, 0, 0, 0, hcb6_2, 0, hcb8_2, 0, hcb10_2, hcb11_2
};

static hcb_bin_pair *hcb_bin_table[] = {
    0, 0, 0, 0, 0, hcb5, 0, hcb7, 0, hcb9, 0, 0
};

static uint8_t hcbN[] = { 0, 5, 5, 0, 5, 0, 5, 0, 5, 0, 6, 5 };


static INLINE void huffman_spectral_data(uint8_t cb, bitfile *ld, int16_t *sp)
{
    uint32_t cw;
    uint16_t offset = 0;
    uint8_t extra_bits;

    switch (cb)
    {
    case 1: /* 2-step method for data quadruples */
    case 2:
    case 4:

        cw = faad_showbits(ld, hcbN[cb]);
        offset = hcb_table[cb][cw].offset;
        extra_bits = hcb_table[cb][cw].extra_bits;

        if (extra_bits)
        {
            /* we know for sure it's more than hcbN[cb] bits long */
            faad_flushbits(ld, hcbN[cb]);
            offset += (uint16_t)faad_showbits(ld, extra_bits);
            faad_flushbits(ld, hcb_2_quad_table[cb][offset].bits - hcbN[cb]);
        } else {
            faad_flushbits(ld, hcb_2_quad_table[cb][offset].bits);
        }

        sp[0] = hcb_2_quad_table[cb][offset].x;
        sp[1] = hcb_2_quad_table[cb][offset].y;
        sp[2] = hcb_2_quad_table[cb][offset].v;
        sp[3] = hcb_2_quad_table[cb][offset].w;
        break;

    case 6: /* 2-step method for data pairs */
    case 8:
    case 10:
    case 11:

        cw = faad_showbits(ld, hcbN[cb]);
        offset = hcb_table[cb][cw].offset;
        extra_bits = hcb_table[cb][cw].extra_bits;

        if (extra_bits)
        {
            /* we know for sure it's more than hcbN[cb] bits long */
            faad_flushbits(ld, hcbN[cb]);
            offset += (uint16_t)faad_showbits(ld, extra_bits);
            faad_flushbits(ld, hcb_2_pair_table[cb][offset].bits - hcbN[cb]);
        } else {
            faad_flushbits(ld, hcb_2_pair_table[cb][offset].bits);
        }

        sp[0] = hcb_2_pair_table[cb][offset].x;
        sp[1] = hcb_2_pair_table[cb][offset].y;
        break;

    case 3: /* binary search for data quadruples */

        while (!hcb3[offset].is_leaf)
        {
            uint8_t b = faad_get1bit(ld);
            offset += hcb3[offset].data[b];
        }

        sp[0] = hcb3[offset].data[0];
        sp[1] = hcb3[offset].data[1];
        sp[2] = hcb3[offset].data[2];
        sp[3] = hcb3[offset].data[3];

        break;

    case 5: /* binary search for data pairs */
    case 7:
    case 9:

        while (!hcb_bin_table[cb][offset].is_leaf)
        {
            uint8_t b = faad_get1bit(ld);
            offset += hcb_bin_table[cb][offset].data[b];
        }

        sp[0] = hcb_bin_table[cb][offset].data[0];
        sp[1] = hcb_bin_table[cb][offset].data[1];

        break;
    }
}

static INLINE void huffman_sign_bits(bitfile *ld, int16_t *sp, uint8_t len)
{
    uint8_t i;

    for(i = 0; i < len; i++)
    {
        if(sp[i])
        {
            if(faad_get1bit(ld
                DEBUGVAR(1,5,"huffman_sign_bits(): sign bit")) & 1)
            {
                sp[i] = -sp[i];
            }
        }
    }
}

static INLINE int32_t huffman_getescape(bitfile *ld, int16_t sp)
{
    uint8_t neg, i;
    int32_t j, off;

    if (sp < 0) {
        if(sp != -16)
            return sp;
        neg = 1;
    } else {
        if(sp != +16)
            return sp;
        neg = 0;
    }

    for (i = 4; ; i++){
        if (faad_get1bit(ld
            DEBUGVAR(1,6,"huffman_getescape(): escape size")) == 0)
        {
            break;
        }
    }

    if (i > 16) {
        off = faad_getbits(ld, i-16
            DEBUGVAR(1,7,"huffman_getescape(): escape, first part")) << 16;
        off |= faad_getbits(ld, 16
            DEBUGVAR(1,8,"huffman_getescape(): escape, second part"));
    } else {
        off = faad_getbits(ld, i
            DEBUGVAR(1,9,"huffman_getescape(): escape"));
    }

    j = off + (1<<i);
    if (neg)
        j = -j;
    return j;
}

#ifdef __cplusplus
}
#endif
#endif
