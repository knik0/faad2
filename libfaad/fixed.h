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
** $Id: fixed.h,v 1.4 2002/11/07 18:24:53 menno Exp $
**/

#ifndef __FIXED_H__
#define __FIXED_H__

#ifdef __cplusplus
extern "C" {
#endif


#define COEF_BITS 28
#define COEF_PRECISION (1 << COEF_BITS)
#define REAL_BITS 7
#define REAL_PRECISION (1 << REAL_BITS)


typedef int32_t real_t;


#define REAL_CONST(A) ((real_t)(A*(REAL_PRECISION)))
#define COEF_CONST(A) ((real_t)(A*(COEF_PRECISION)))

#if defined(_WIN32) && !defined(_WIN32_WCE)

/* multiply real with real */
static INLINE MUL(real_t A, real_t B)
{
    _asm {
        mov eax,A
        imul B
        shrd eax,edx,REAL_BITS
    }
}

/* multiply coef with coef */
static INLINE MUL_C_C(real_t A, real_t B)
{
    _asm {
        mov eax,A
        imul B
        shrd eax,edx,COEF_BITS
    }
}

/* multiply real with coef */
static INLINE MUL_R_C(real_t A, real_t B)
{
    _asm {
        mov eax,A
        imul B
        shrd eax,edx,COEF_BITS
    }
}

#else

  /* multiply real with real */
  #define MUL(A,B) (real_t)(((int64_t)(A)*(int64_t)(B)+(1 << (REAL_BITS-1))) >> REAL_BITS)
  /* multiply coef with coef */
  #define MUL_C_C(A,B) (real_t)(((int64_t)(A)*(int64_t)(B)+(1 << (COEF_BITS-1))) >> COEF_BITS)
  /* multiply real with coef */
  #define MUL_R_C(A,B) (real_t)(((int64_t)(A)*(int64_t)(B)+(1 << (COEF_BITS-1))) >> COEF_BITS)

#endif


#ifdef __cplusplus
}
#endif
#endif
