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
** $Id: bits.h,v 1.1 2002/01/14 19:15:55 menno Exp $
**/

#ifndef __BITS_H__
#define __BITS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define BYTE_NUMBIT 8
#define bit2byte(a) ((a)/BYTE_NUMBIT)

/* to mask the n least significant bits of an integer */
static unsigned int msk[33] =
{
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
    0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
    0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
    0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
    0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
    0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
    0xffffffff
};

typedef struct _bitfile
{
    /* bit input */
    unsigned char *rdptr;
    int incnt;
    int bitcnt;
    int framebits;
} bitfile;


#if defined(LINUX) && defined(ARM__)
#define bits_inline
#define _SWAP(a,b) { register unsigned int temp; \
	b = *(int*)(a); \
	__asm__ ( " EOR %0, %1, %2, ROR #16" : "=r" (temp) : "r" (b), "r" (b)); \
	__asm__ ( " BIC %0, %1, #0x00FF0000" : "=r" (temp) : "r" (temp)); \
	__asm__ ( " MOV %0, %1, ROR #8"      : "=r" (b) : "r" (b)); \
	__asm__ ( " EOR %0, %1, %2, LSR #8"  : "=r" (b) : "r" (b), "r" (temp)); \
	}
#elif defined(LINUX)
#define bits_inline inline
#define _SWAP(a,b)	\
	b=*(int*)a; \
	__asm__ ( "bswapl %0\n" : "=r" (b) : "0" (b) )
#elif defined(WIN32)
#define bits_inline __inline
#define _SWAP(a,b) \
	{	\
	register unsigned int * c = &b;	\
	b=*(int*)a; __asm mov ecx, c __asm mov eax, [ecx] __asm bswap eax __asm mov [ecx], eax	\
	}
#else
#define bits_inline
#define _SWAP(a,b) (b=((a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3]))
#endif


void faad_initbits(bitfile *ld, unsigned char *buffer);
unsigned int faad_byte_align(bitfile *ld);
int faad_get_processed_bits(bitfile *ld);


static bits_inline unsigned int faad_showbits(bitfile *ld, int n)
{
    unsigned char *v = ld->rdptr;
    int rbit = 32 - ld->bitcnt;
    unsigned int b;

    _SWAP(v, b);
    return ((b & msk[rbit]) >> (rbit-n));
}

static bits_inline void faad_flushbits(bitfile *ld, int n)
{
    ld->bitcnt += n;

    if (ld->bitcnt >= 8)
    {
        ld->rdptr += (ld->bitcnt>>3);
        ld->bitcnt &= 7;
    }

    ld->framebits += n;
}

/* return next n bits (right adjusted) */
static bits_inline unsigned int faad_getbits(bitfile *ld, int n)
{
    long l;

    l = faad_showbits(ld, n);
    faad_flushbits(ld, n);

    return l;
}

static bits_inline unsigned int faad_get1bit(bitfile *ld)
{
    unsigned char l;

    l = *ld->rdptr << ld->bitcnt;

    ld->bitcnt++;
    ld->framebits++;
    ld->rdptr += (ld->bitcnt>>3);
    ld->bitcnt &= 7;

    return l>>7;
}

#ifdef __cplusplus
}
#endif
#endif
