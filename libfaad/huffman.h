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
** $Id: huffman.h,v 1.2 2002/01/19 09:39:41 menno Exp $
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


typedef struct
{
    short len;
    unsigned long cw;
    char x, y, v, w;
} codebook;

typedef struct
{
    short len;
    unsigned long cw;
    int scl;
} codebook_scl;

extern codebook book1[];
extern codebook book2[];
extern codebook book3[];
extern codebook book4[];
extern codebook book5[];
extern codebook book6[];
extern codebook book7[];
extern codebook book8[];
extern codebook book9[];
extern codebook book10[];
extern codebook book11[];
extern codebook_scl bookscl[];

static codebook *book_table[] = {
    0,
    book1,
    book2,
    book3,
    book4,
    book5,
    book6,
    book7,
    book8,
    book9,
    book10,
    book11
};

#if defined(LINUX)
#define huff_inline inline
#elif defined(WIN32)
#define huff_inline __inline
#else
#define huff_inline
#endif

static huff_inline int huffman_scale_factor(bitfile *ld)
{
    int i, j;
    long cw;
    codebook_scl *h = bookscl;

    i = h->len;
    cw = faad_getbits(ld, i);

    while ((unsigned long)cw != h->cw)
    {
        h++;
        j = h->len-i;
        i = h->len;
        if (j!=0) {
            while (j--)
                cw = (cw<<1) | faad_get1bit(ld);
        }
    }

#ifdef ANALYSIS
    fprintf(stdout, "%4d %2d bits, huffman_scale_factor(): huffman codeword (scalefactor)\n",
        dbg_count++, h->len);
#endif

    return h->scl;
}

static int codebookN[] = { 0, 7, 7, 9 };

static huff_inline void huffman_spectral_data(int cb, bitfile *ld, short *sp)
{
    int i, j;
    unsigned long cw;
    codebook *h;

    h = book_table[cb];
    i = h->len;
    cw = faad_getbits(ld, i DEBUGVAR(0,0,""));

    while (cw != h->cw)
    {
        h++;
        j = h->len-i;
        i = h->len;
        if (j!=0) {
            while (j--)
                cw = (cw<<1) | faad_get1bit(ld DEBUGVAR(0,0,""));
        }
    }

#ifdef ANALYSIS
    fprintf(stdout, "%4d %2d bits, huffman_spectral_data(): huffman codeword\n",
        dbg_count++, h->len);
#endif

    if(cb < FIRST_PAIR_HCB)
    {
        sp[0] = h->x;
        sp[1] = h->y;
        sp[2] = h->v;
        sp[3] = h->w;
    } else {
        sp[0] = h->x;
        sp[1] = h->y;
    }
}

static huff_inline void huffman_sign_bits(bitfile *ld, short *sp, int len)
{
    int i;

    for(i = 0; i < len; i++)
    {
        if(sp[i])
        {
            if(faad_get1bit(ld
                DEBUGVAR(1,0,"huffman_sign_bits(): sign bit")) & 1)
            {
                sp[i] = -sp[i];
            }
        }
    }
}

static huff_inline short huffman_getescape(bitfile *ld, short sp)
{
    int i, off, neg;

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
        if (faad_get1bit(ld DEBUGVAR(1,0,"huffman_getescape(): escape size")) == 0)
            break;
    }

    if (i > 16) {
        off = faad_getbits(ld, i-16
            DEBUGVAR(1,0,"huffman_getescape(): escape, first part")) << 16;
        off |= faad_getbits(ld, 16
            DEBUGVAR(1,0,"huffman_getescape(): escape, second part"));
    } else {
        off = faad_getbits(ld, i
            DEBUGVAR(1,0,"huffman_getescape(): escape"));
    }

    i = off + (1<<i);
    if (neg)
        i = -i;
    return i;
}

#ifdef __cplusplus
}
#endif
#endif
