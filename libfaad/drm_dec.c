/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR and PS decoding
** Copyright (C) 2003-2004 M. Bakker, Ahead Software AG, http://www.nero.com
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
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: drm_dec.c,v 1.1 2004/01/19 21:49:53 menno Exp $
**/

#include "common.h"

#ifdef DRM

#include "drm_dec.h"
#include "bits.h"

/* constants */
#define NUM_SA_BANDS 8
#define NUM_PAN_BANDS 20


/* type definitaions */
typedef const int8_t (*drm_ps_huff_tab)[2];


/* binary search huffman tables */
static const int8_t f_huffman_sa[][2] = {
    { /*0*/ -15, 1 },             /* index 0: 1 bits: x */
    { 2, 3 },                     /* index 1: 2 bits: 1x */
    { /*7*/ -8, 4 },              /* index 2: 3 bits: 10x */
    { 5, 6 },                     /* index 3: 3 bits: 11x */
    { /*1*/ -14, /*-1*/ -16 },    /* index 4: 4 bits: 101x */
    { /*-2*/ -17, 7 },            /* index 5: 4 bits: 110x */
    { 8, 9 },                     /* index 6: 4 bits: 111x */
    { /*2*/ -13, /*-3*/ -18 },    /* index 7: 5 bits: 1101x */
    { /*3*/ -12, 10 },            /* index 8: 5 bits: 1110x */
    { 11, 12 },                   /* index 9: 5 bits: 1111x */
    { /*4*/ -11, /*5*/ -10 },     /* index 10: 6 bits: 11101x */
    { /*-4*/ -19, /*-5*/ -20 },   /* index 11: 6 bits: 11110x */
    { /*6*/ -9, 13 },             /* index 12: 6 bits: 11111x */
    { /*-7*/ -22, /*-6*/ -21 }    /* index 13: 7 bits: 111111x */
};

static const int8_t t_huffman_sa[][2] = {
    { /*0*/ -15, 1 },             /* index 0: 1 bits: x */
    { 2, 3 },                     /* index 1: 2 bits: 1x */
    { /*-1*/ -16, /*1*/ -14 },    /* index 2: 3 bits: 10x */
    { 4, 5 },                     /* index 3: 3 bits: 11x */
    { /*-2*/ -17, /*2*/ -13 },    /* index 4: 4 bits: 110x */
    { 6, 7 },                     /* index 5: 4 bits: 111x */
    { /*-3*/ -18, /*3*/ -12 },    /* index 6: 5 bits: 1110x */
    { 8, 9 },                     /* index 7: 5 bits: 1111x */
    { /*-4*/ -19, /*4*/ -11 },    /* index 8: 6 bits: 11110x */
    { 10, 11 },                   /* index 9: 6 bits: 11111x */
    { /*-5*/ -20, /*5*/ -10 },    /* index 10: 7 bits: 111110x */
    { /*-6*/ -21, 12 },           /* index 11: 7 bits: 111111x */
    { /*-7*/ -22, 13 },           /* index 12: 8 bits: 1111111x */
    { /*6*/ -9, /*7*/ -8 }        /* index 13: 9 bits: 11111111x */
};

static const int8_t f_huffman_pan[][2] = {
    { /*0*/ -15, 1 },             /* index 0: 1 bits: x */
    { /*-1*/ -16, 2 },            /* index 1: 2 bits: 1x */
    { /*1*/ -14, 3 },             /* index 2: 3 bits: 11x */
    { 4, 5 },                     /* index 3: 4 bits: 111x */
    { /*-2*/ -17, /*2*/ -13 },    /* index 4: 5 bits: 1110x */
    { 6, 7 },                     /* index 5: 5 bits: 1111x */
    { /*-3*/ -18, /*3*/ -12 },    /* index 6: 6 bits: 11110x */
    { 8, 9 },                     /* index 7: 6 bits: 11111x */
    { /*-4*/ -19, /*4*/ -11 },    /* index 8: 7 bits: 111110x */
    { 10, 11 },                   /* index 9: 7 bits: 111111x */
    { /*-5*/ -20, /*5*/ -10 },    /* index 10: 8 bits: 1111110x */
    { 12, 13 },                   /* index 11: 8 bits: 1111111x */
    { /*-6*/ -21, /*6*/ -9 },     /* index 12: 9 bits: 11111110x */
    { /*-7*/ -22, 14 },           /* index 13: 9 bits: 11111111x */
    { /*7*/ -8, 15 },             /* index 14: 10 bits: 111111111x */
    { 16, 17 },                   /* index 15: 11 bits: 1111111111x */
    { /*-8*/ -23, /*8*/ -7 },     /* index 16: 12 bits: 11111111110x */
    { 18, 19 },                   /* index 17: 12 bits: 11111111111x */
    { /*-10*/ -25, 20 },          /* index 18: 13 bits: 111111111110x */
    { 21, 22 },                   /* index 19: 13 bits: 111111111111x */
    { /*-9*/ -24, /*9*/ -6 },     /* index 20: 14 bits: 1111111111101x */
    { /*10*/ -5, 23 },            /* index 21: 14 bits: 1111111111110x */
    { 24, 25 },                   /* index 22: 14 bits: 1111111111111x */
    { /*-13*/ -28, /*-11*/ -26 }, /* index 23: 15 bits: 11111111111101x */
    { /*11*/ -4, /*13*/ -2 },     /* index 24: 15 bits: 11111111111110x */
    { 26, 27 },                   /* index 25: 15 bits: 11111111111111x */
    { /*-14*/ -29, /*-12*/ -27 }, /* index 26: 16 bits: 111111111111110x */
    { /*12*/ -3, /*14*/ -1 }      /* index 27: 16 bits: 111111111111111x */
};

static const int8_t t_huffman_pan[][2] = {
    { /*0*/ -15, 1 },             /* index 0: 1 bits: x */
    { /*-1*/ -16, 2 },            /* index 1: 2 bits: 1x */
    { /*1*/ -14, 3 },             /* index 2: 3 bits: 11x */
    { /*-2*/ -17, 4 },            /* index 3: 4 bits: 111x */
    { /*2*/ -13, 5 },             /* index 4: 5 bits: 1111x */
    { /*-3*/ -18, 6 },            /* index 5: 6 bits: 11111x */
    { /*3*/ -12, 7 },             /* index 6: 7 bits: 111111x */
    { /*-4*/ -19, 8 },            /* index 7: 8 bits: 1111111x */
    { /*4*/ -11, 9 },             /* index 8: 9 bits: 11111111x */
    { 10, 11 },                   /* index 9: 10 bits: 111111111x */
    { /*-5*/ -20, /*5*/ -10 },    /* index 10: 11 bits: 1111111110x */
    { 12, 13 },                   /* index 11: 11 bits: 1111111111x */
    { /*-6*/ -21, /*6*/ -9 },     /* index 12: 12 bits: 11111111110x */
    { 14, 15 },                   /* index 13: 12 bits: 11111111111x */
    { /*-7*/ -22, /*7*/ -8 },     /* index 14: 13 bits: 111111111110x */
    { 16, 17 },                   /* index 15: 13 bits: 111111111111x */
    { /*-8*/ -23, /*8*/ -7 },     /* index 16: 14 bits: 1111111111110x */
    { 18, 19 },                   /* index 17: 14 bits: 1111111111111x */
    { /*-10*/ -25, /*10*/ -5 },   /* index 18: 15 bits: 11111111111110x */
    { 20, 21 },                   /* index 19: 15 bits: 11111111111111x */
    { /*-9*/ -24, /*9*/ -6 },     /* index 20: 16 bits: 111111111111110x */
    { 22, 23 },                   /* index 21: 16 bits: 111111111111111x */
    { 24, 25 },                   /* index 22: 17 bits: 1111111111111110x */
    { 26, 27 },                   /* index 23: 17 bits: 1111111111111111x */
    { /*-14*/ -29, /*-13*/ -28 }, /* index 24: 18 bits: 11111111111111100x */
    { /*-12*/ -27, /*-11*/ -26 }, /* index 25: 18 bits: 11111111111111101x */
    { /*11*/ -4, /*12*/ -3 },     /* index 26: 18 bits: 11111111111111110x */
    { /*13*/ -2, /*14*/ -1 }      /* index 27: 18 bits: 11111111111111111x */
};


/* static function declarations */
static void drm_ps_sa_element(drm_ps_info *ps, bitfile *ld);
static void drm_ps_pan_element(drm_ps_info *ps, bitfile *ld);
static int8_t huff_dec(bitfile *ld, drm_ps_huff_tab huff);


uint16_t drm_ps_data(drm_ps_info *ps, bitfile *ld)
{
    uint16_t bits = (uint16_t)faad_get_processed_bits(ld);

    ps->bs_enable_sa = faad_get1bit(ld);
    if (ps->bs_enable_sa)
    {
        drm_ps_sa_element(ps, ld);
    }

    ps->bs_enable_pan = faad_get1bit(ld);
    if (ps->bs_enable_pan)
    {
        drm_ps_pan_element(ps, ld);
    }

    bits = (uint16_t)faad_get_processed_bits(ld) - bits;

    return bits;
}

static void drm_ps_sa_element(drm_ps_info *ps, bitfile *ld)
{
    drm_ps_huff_tab huff;
    uint8_t band;

    ps->bs_sa_dt_flag = faad_get1bit(ld);
    if (ps->bs_sa_dt_flag)
    {
        huff = t_huffman_sa;
    } else {
        huff = f_huffman_sa;
    }

    for (band = 0; band < NUM_SA_BANDS; band++)
    {
        ps->bs_sa_data[band] = huff_dec(ld, huff);
    }
}

static void drm_ps_pan_element(drm_ps_info *ps, bitfile *ld)
{
    drm_ps_huff_tab huff;
    uint8_t band;

    ps->bs_pan_dt_flag = faad_get1bit(ld);
    if (ps->bs_pan_dt_flag)
    {
        huff = t_huffman_pan;
    } else {
        huff = f_huffman_pan;
    }

    for (band = 0; band < NUM_PAN_BANDS; band++)
    {
        ps->bs_pan_data[band] = huff_dec(ld, huff);
    }
}

/* binary search huffman decoding */
static int8_t huff_dec(bitfile *ld, drm_ps_huff_tab huff)
{
    uint8_t bit;
    int16_t index = 0;

    while (index >= 0)
    {
        bit = (uint8_t)faad_get1bit(ld);
        index = huff[index][bit];
    }

    return index + 15;
}

#endif
