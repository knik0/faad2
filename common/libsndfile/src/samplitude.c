/*
** Copyright (C) 2000-2001 Erik de Castro Lopo <erikd@zip.com.au>
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


#include	<stdio.h>
#include	<unistd.h>
#include	<string.h>
#include	<ctype.h>
#include	<stdarg.h>

#include	"sndfile.h"
#include	"config.h"
#include	"sfendian.h"
#include	"common.h"


/*------------------------------------------------------------------------------
 * Macros to handle big/little endian issues.
*/

#if (CPU_IS_LITTLE_ENDIAN == 1)
#	define	MAKE_MARKER(a,b,c,d)		((a)|((b)<<8)|((c)<<16)|((d)<<24))
#elif (CPU_IS_BIG_ENDIAN == 1)
#	define	MAKE_MARKER(a,b,c,d)		(((a)<<24)|((b)<<16)|((c)<<8)|(d))
#else
#	error "Cannot determine endian-ness of processor."
#endif

#define SEKD_MARKER	(MAKE_MARKER ('S', 'E', 'K', 'D'))
#define SAMR_MARKER	(MAKE_MARKER ('S', 'A', 'M', 'R'))

/*------------------------------------------------------------------------------
 * Typedefs for file chunks.
*/

typedef struct
{	unsigned int	sekd, samr ;
	int				unknown0 ;
	int				bitspersample ;
	int				unknown1, unknown2 ;
	int				poss_len ;
	int				samplerate ;
	int				channels ;
} RAP_CHUNK ;


/*------------------------------------------------------------------------------
 * Private static functions.
*/

static	int		smpltd_close	(SF_PRIVATE  *psf) ;


/*------------------------------------------------------------------------------
** Public functions.
*/

int 	smpltd_open_read	(SF_PRIVATE *psf)
{	RAP_CHUNK rap ;
	
	psf_binheader_readf (psf, "mm", &(rap.sekd), &(rap.samr)) ;
	if (rap.sekd != SEKD_MARKER)
		return SFE_SMTD_NO_SEKD ;
	if (rap.samr != SAMR_MARKER)
		return SFE_SMTD_NO_SAMR ;
	
/*-
	printf ("Here we are!\n") ;	

	printf ("unknown0        : %d\n", rap.unknown0) ;
	printf ("bits per sample : %d\n", rap.bitspersample) ;
	printf ("unknown1        : %d\n", rap.unknown1) ;
	printf ("unknown2        : %d\n", rap.unknown2) ;
	printf ("poss_len        : %d\n", rap.poss_len) ;
	printf ("sample rate     : %d\n", rap.samplerate) ;
	printf ("channels        : %d\n", rap.channels) ;
		
-*/
	psf->close = (func_close) smpltd_close ;

	return 0 ;
} /* smpltd_open_read */

int 	
smpltd_open_write	(SF_PRIVATE *psf)
{	

	return 0 ;
} /* smpltd_open_write */

/*------------------------------------------------------------------------------
*/

static int	
smpltd_close	(SF_PRIVATE  *psf)
{	
	if (psf->mode == SF_MODE_WRITE)
	{	/*  Now we know for certain the length of the file we can re-write 
		**	correct values for the FORM, 8SVX and BODY chunks.
		*/
                
		} ;

	if (psf->fdata)
		free (psf->fdata) ;
	psf->fdata = NULL ;
	
	return 0 ;
} /* smpltd_close */

