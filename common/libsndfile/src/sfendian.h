/*
** Copyright (C) 1999-2001 Erik de Castro Lopo <erikd@zip.com.au>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 2.1 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
** 
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include	"config.h"

#if HAVE_ENDIAN_H
	/* This is the best way to do it. Unfortunately Sparc Solaris (and
	** possibly others) don't have <endian.h>
	*/
	#include	<endian.h>
	#if (__BYTE_ORDER == __LITTLE_ENDIAN)
		#define	CPU_IS_LITTLE_ENDIAN		1
		#define	CPU_IS_BIG_ENDIAN			0
	#elif (__BYTE_ORDER == __BIG_ENDIAN)
		#define	CPU_IS_LITTLE_ENDIAN		0
		#define	CPU_IS_BIG_ENDIAN			1
	#else
		#error "A bit confused about endian-ness! Have <endian.h> but not __BYTEORDER."
	#endif
#else
	/* If we don't have <endian.h> use the guess based on target processor
	** from the autoconf process.
	*/
	#if GUESS_LITTLE_ENDIAN
		#define	CPU_IS_LITTLE_ENDIAN		1
		#define	CPU_IS_BIG_ENDIAN			0
	#elif GUESS_BIG_ENDIAN
		#define	CPU_IS_LITTLE_ENDIAN		0
		#define	CPU_IS_BIG_ENDIAN			1
	#else
		#error "Endian guess seems incorrect."
	#endif
#endif	

#define		ENDSWAP_SHORT(x)			((((x)>>8)&0xFF)|(((x)&0xFF)<<8))
#define		ENDSWAP_INT(x)				((((x)>>24)&0xFF)|(((x)>>8)&0xFF00)|(((x)&0xFF00)<<8)|(((x)&0xFF)<<24))

#if (CPU_IS_LITTLE_ENDIAN == 1)
	#define	H2LE_SHORT(x)				(x)
	#define	H2LE_INT(x)					(x)
	#define	LE2H_SHORT(x)				(x)
	#define	LE2H_INT(x)					(x)

	#define	BE2H_INT(x)					ENDSWAP_INT(x)
	#define	BE2H_SHORT(x)				ENDSWAP_SHORT(x)
	#define	H2BE_INT(x)					ENDSWAP_INT(x)
	#define	H2BE_SHORT(x)				ENDSWAP_SHORT(x)

#elif (CPU_IS_BIG_ENDIAN == 1)
	#define	H2LE_SHORT(x)				ENDSWAP_SHORT(x)
	#define	H2LE_INT(x)					ENDSWAP_INT(x)
	#define	LE2H_SHORT(x)				ENDSWAP_SHORT(x)
	#define	LE2H_INT(x)					ENDSWAP_INT(x)

	#define	BE2H_INT(x)					(x)
	#define	BE2H_SHORT(x)				(x)
	#define	H2BE_INT(x)					(x)
	#define	H2BE_SHORT(x)				(x)

#else
	#error "Cannot determine endian-ness of processor."
#endif

#if (CPU_IS_LITTLE_ENDIAN == 1)
#	define	MAKE_MARKER(a,b,c,d)		((a)|((b)<<8)|((c)<<16)|((d)<<24))
#elif (CPU_IS_BIG_ENDIAN == 1)
#	define	MAKE_MARKER(a,b,c,d)		(((a)<<24)|((b)<<16)|((c)<<8)|(d))
#else
#	error "Cannot determine endian-ness of processor."
#endif

