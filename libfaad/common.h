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
** $Id: common.h,v 1.2 2002/02/20 13:05:57 menno Exp $
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

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
#ifndef M_PI_2 /* PI/2 */
#define M_PI_2 1.57079632679489661923
#endif


/* COMPILE TIME DEFINITIONS */

/* use double precision */
/* #define USE_DOUBLE_PRECISION */

/* use table lookup twiddle factors in MDCT [more memory, higher speed],
   otherwise recurrence relations are used [no memory usage, lower speed] */
#define USE_TWIDDLE_TABLE

/* END COMPILE TIME DEFINITIONS */


#if defined(_WIN32)


typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;
typedef __int32 int32_t;
typedef __int16 int16_t;
typedef __int8  int8_t;
typedef float float32_t;


#elif defined(LINUX) || defined(DJGPP)


#if defined(LINUX)
#include <stdint.h>
#else
typedef unsigned long uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef long int32_t;
typedef short int16_t;
typedef char int8_t;
typedef float float32_t;
#endif


#else /* Some other OS */


#include <inttypes.h>

#endif


#ifndef USE_DOUBLE_PRECISION

  typedef float real_t;

  #ifdef __ICL /* only Intel C compiler has fmath ??? */

    #include <mathf.h>

    #define sin sinf
    #define cos cosf
    #define pow powf
    #define floor floorf
    #define sqrt sqrtf

  #else

    #include <math.h>

#ifdef HAVE_SINF
#  define sin sinf
#endif
#ifdef HAVE_COSF
#  define cos cosf
#endif
#ifdef HAVE_POWF
#  define pow powf
#endif
#ifdef HAVE_FLOORF
#  define floor floorf
#endif
#ifdef HAVE_SQRTF
#  define sqrt sqrtf
#endif

  #endif

#else

  typedef double real_t;
  #include <math.h>

#endif

#ifdef __cplusplus
}
#endif
#endif
