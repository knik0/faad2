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
** $Id: bits.h,v 1.5 2002/02/18 10:01:05 menno Exp $
**/

#ifndef __BITS_H__
#define __BITS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "analysis.h"
#ifdef ANALYSIS
#include <stdio.h>
#endif

#define BYTE_NUMBIT 8
#define bit2byte(a) ((a+7)/BYTE_NUMBIT)

typedef struct _bitfile
{
    /* bit input */
	uint32_t bufa;
	uint32_t bufb;
	uint32_t pos;
	uint32_t *tail;
	uint32_t *start;
} bitfile;


#if defined(_WIN32)
#define BSWAP(a) __asm mov eax,a __asm bswap eax __asm mov a, eax
#elif defined(LINUX) || defined(DJGPP)
#define BSWAP(a) __asm__ ( "bswapl %0\n" : "=r" (a) : "0" (a) )
#else
#define BSWAP(a) \
	 ((a) = ( ((a)&0xff)<<24) | (((a)&0xff00)<<8) | (((a)>>8)&0xff00) | (((a)>>24)&0xff))
#endif


void faad_initbits(bitfile *ld, void *buffer);
uint8_t faad_byte_align(bitfile *ld);
uint32_t faad_get_processed_bits(bitfile *ld);


static INLINE uint32_t faad_showbits(bitfile *ld, uint8_t bits)
{
    int32_t nbit = (bits + ld->pos) - 32;
    if (nbit > 0) 
    {
        return ((ld->bufa & (0xffffffff >> ld->pos)) << nbit) |
            (ld->bufb >> (32 - nbit));
    } else {
        return (ld->bufa & (0xffffffff >> ld->pos)) >> (32 - ld->pos - bits);
    }
}

static INLINE void faad_flushbits(bitfile *ld, uint8_t bits)
{
	ld->pos += bits;

	if (ld->pos >= 32) 
	{
		uint32_t tmp;

		ld->bufa = ld->bufb;
		tmp = *(uint32_t*)ld->tail;
#ifndef ARCH_IS_BIG_ENDIAN
		BSWAP(tmp);
#endif
		ld->bufb = tmp;
		ld->tail++;
		ld->pos -= 32;
	}
}

/* return next n bits (right adjusted) */
static INLINE uint32_t faad_getbits(bitfile *ld, uint8_t n DEBUGDEC)
{
	uint32_t ret = faad_showbits(ld, n);
	faad_flushbits(ld, n);

#ifdef ANALYSIS
    if (print)
        fprintf(stdout, "%4d %2d bits, val: %4d, variable: %d %s\n", dbg_count++, n, ret, var, dbg);
#endif

	return ret;
}

static INLINE uint8_t faad_get1bit(bitfile *ld DEBUGDEC)
{
    return (uint8_t)faad_getbits(ld, 1 DEBUGVAR(print,var,dbg));
}

#ifdef __cplusplus
}
#endif
#endif
