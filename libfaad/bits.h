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
** $Id: bits.h,v 1.4 2002/02/15 20:52:09 menno Exp $
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
#define bit2byte(a) ((a)/BYTE_NUMBIT)

typedef struct _bitfile
{
    /* bit input */
	unsigned long bufa;
	unsigned long bufb;
	unsigned long pos;
	unsigned long *tail;
	unsigned long *start;
} bitfile;


#if defined _WIN32
#define bits_inline __inline
#define BSWAP(a) __asm mov eax,a __asm bswap eax __asm mov a, eax
#elif defined(LINUX) || defined(DJGPP)
#define bits_inline inline
#define BSWAP(a) __asm__ ( "bswapl %0\n" : "=r" (a) : "0" (a) )
#else
#define bits_inline
#define BSWAP(a) \
	 ((a) = ( ((a)&0xff)<<24) | (((a)&0xff00)<<8) | (((a)>>8)&0xff00) | (((a)>>24)&0xff))
#endif


void faad_initbits(bitfile *ld, void *buffer);
unsigned int faad_byte_align(bitfile *ld);
int faad_get_processed_bits(bitfile *ld);


static bits_inline unsigned int faad_showbits(bitfile *ld, int bits)
{
    int nbit = (bits + ld->pos) - 32;
    if (nbit > 0) 
    {
        return ((ld->bufa & (0xffffffff >> ld->pos)) << nbit) |
            (ld->bufb >> (32 - nbit));
    } else {
        return (ld->bufa & (0xffffffff >> ld->pos)) >> (32 - ld->pos - bits);
    }
}

static bits_inline void faad_flushbits(bitfile *ld, int bits)
{
	ld->pos += bits;

	if (ld->pos >= 32) 
	{
		unsigned long tmp;

		ld->bufa = ld->bufb;
		tmp = *(unsigned long*)ld->tail;
#ifndef ARCH_IS_BIG_ENDIAN
		BSWAP(tmp);
#endif
		ld->bufb = tmp;
		ld->tail++;
		ld->pos -= 32;
	}
}

/* return next n bits (right adjusted) */
static bits_inline unsigned int faad_getbits(bitfile *ld, int n DEBUGDEC)
{
	unsigned long ret = faad_showbits(ld, n);
	faad_flushbits(ld, n);

#ifdef ANALYSIS
    if (print)
        fprintf(stdout, "%4d %2d bits, val: %4d, variable: %d %s\n", dbg_count++, n, ret, var, dbg);
#endif

	return ret;
}

static bits_inline unsigned int faad_get1bit(bitfile *ld DEBUGDEC)
{
    return faad_getbits(ld, 1 DEBUGVAR(print,var,dbg));
}

#ifdef __cplusplus
}
#endif
#endif
