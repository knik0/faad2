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
** $Id: common.h,v 1.21 2002/09/13 13:08:45 menno Exp $
**/

#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif


#ifdef LINUX
#define INLINE inline
#else
#ifdef _WIN32
#define INLINE __inline
#else
#define INLINE
#endif
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif


/* COMPILE TIME DEFINITIONS */

/* use double precision */
/* #define USE_DOUBLE_PRECISION */
/* use fixed point reals */
//#define FIXED_POINT

//#define SBR
#define ERROR_RESILIENCE


/* Allow decoding of MAIN profile AAC */
#define MAIN_DEC
/* Allow decoding of LTP profile AAC */
#define LTP_DEC
/* Allow decoding of LD profile AAC */
#define LD_DEC

/* LD can't do without LTP */
#ifdef LD_DEC
#ifndef ERROR_RESILIENCE
#define ERROR_RESILIENCE
#endif
#ifndef LTP_DEC
#define LTP_DEC
#endif
#endif


/* END COMPILE TIME DEFINITIONS */


#if defined(_WIN32)


typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;
typedef __int64 int64_t;
typedef __int32 int32_t;
typedef __int16 int16_t;
typedef __int8  int8_t;
#ifndef FIXED_POINT
typedef float float32_t;
#endif


#elif defined(LINUX) || defined(DJGPP)


#if defined(LINUX)
#include <stdint.h>
#else
typedef unsigned long long uint64_t;
typedef unsigned long uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef long long int64_t;
typedef long int32_t;
typedef short int16_t;
typedef char int8_t;
#ifndef FIXED_POINT
typedef float float32_t;
#endif
#endif


#else /* Some other OS */

#include <inttypes.h>

#endif

/* FIXED_POINT doesn't work with FFTW */
#ifdef FIXED_POINT
  #undef MAIN_DEC
#endif


#if defined(FIXED_POINT)

  #include <math.h>

  #include "fixed.h"

#elif defined(USE_DOUBLE_PRECISION)

  typedef double real_t;

  #include <math.h>

  #define MUL(A,B) ((A)*(B))
  #define MUL_C_C(A,B) ((A)*(B))
  #define MUL_R_C(A,B) ((A)*(B))

  #define REAL_CONST(A) ((real_t)A)
  #define COEF_CONST(A) ((real_t)A)

#else /* Normal floating point operation */

  typedef float real_t;

  #define MUL(A,B) ((A)*(B))
  #define MUL_C_C(A,B) ((A)*(B))
  #define MUL_R_C(A,B) ((A)*(B))

  #define REAL_CONST(A) ((real_t)A)
  #define COEF_CONST(A) ((real_t)A)

  #ifdef __ICL /* only Intel C compiler has fmath ??? */

    #include <mathf.h>

    #define sin sinf
    #define cos cosf
    #define log logf
    #define floor floorf
    #define ceil ceilf
    #define sqrt sqrtf

  #else

    #include <math.h>

#ifdef HAVE_SINF
#  define sin sinf
#endif
#ifdef HAVE_COSF
#  define cos cosf
#endif
#ifdef HAVE_LOGF
#  define log logf
#endif
#ifdef HAVE_POWF
#  define pow powf
#endif
#ifdef HAVE_FLOORF
#  define floor floorf
#endif
#ifdef HAVE_FLOORF
#  define ceil ceilf
#endif
#ifdef HAVE_SQRTF
#  define sqrt sqrtf
#endif

  #endif

#endif

typedef struct {
    real_t re;
    real_t im;
} complex_t;


/* common functions */
uint32_t int_log2(uint32_t val);

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
#ifndef M_PI_2 /* PI/2 */
#define M_PI_2 1.57079632679489661923
#endif

#ifdef __cplusplus
}
#endif
#endif
