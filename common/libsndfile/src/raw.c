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


#include	<stdio.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<string.h>
#include	<ctype.h>

#include	"sndfile.h"
#include	"config.h"
#include	"sfendian.h"
#include	"common.h"

static long	raw_seek   (SF_PRIVATE *psf, long offset, int whence) ;

/*------------------------------------------------------------------------------
** Public functions.
*/

int 	raw_open_read	(SF_PRIVATE *psf)
{	unsigned int subformat ;
	int			error ;
	
	if(! psf->sf.channels || ! psf->sf.pcmbitwidth)
		return SFE_RAW_READ_BAD_SPEC ;
		
	subformat = psf->sf.format & SF_FORMAT_SUBMASK ;

	psf->endian = 0 ;

	if (subformat == SF_FORMAT_PCM_BE)
		psf->endian = SF_ENDIAN_BIG ;
	else if (subformat == SF_FORMAT_PCM_LE)
		psf->endian = SF_ENDIAN_LITTLE ;
	else if (subformat == SF_FORMAT_PCM_S8)
		psf->chars = SF_CHARS_SIGNED ;
	else if (subformat == SF_FORMAT_PCM_U8)
		psf->chars = SF_CHARS_UNSIGNED ;
	else
		return SFE_RAW_READ_BAD_SPEC ;
		
	psf->seek_func = (func_seek) raw_seek ;

	psf->sf.seekable = SF_TRUE ;
	psf->sf.sections = 1 ;

	psf->dataoffset = 0 ;
	psf->bytewidth  = BITWIDTH2BYTES (psf->sf.pcmbitwidth) ;
	psf->blockwidth = psf->sf.channels * psf->bytewidth ;

	if ((error = pcm_read_init (psf)))
		return error ;
		
	if (psf->blockwidth)
		psf->sf.samples = psf->filelength / psf->blockwidth ;

 	psf->datalength = psf->filelength - psf->dataoffset ;

 	psf->current  = 0 ;
	
	return 0 ;
} /* raw_open_read */

/*------------------------------------------------------------------------------
*/

int 	raw_open_write	(SF_PRIVATE *psf)
{	unsigned int	subformat, big_endian_file ;
	int error ;
		
	subformat = psf->sf.format & SF_FORMAT_SUBMASK ;

	if (subformat == SF_FORMAT_PCM_BE)
		big_endian_file = 1 ;
	else if (subformat == SF_FORMAT_PCM_LE)
		big_endian_file = 0 ;
	else if (subformat == SF_FORMAT_PCM_S8 || subformat == SF_FORMAT_PCM_U8)
		big_endian_file = 0 ;
	else
		return	SFE_BAD_OPEN_FORMAT ;
		
	psf->bytewidth = BITWIDTH2BYTES (psf->sf.pcmbitwidth) ;
		
	psf->endian      = big_endian_file ? SF_ENDIAN_BIG : SF_ENDIAN_LITTLE ;
	psf->sf.seekable = SF_TRUE ;
	psf->blockwidth  = psf->bytewidth * psf->sf.channels ;
 	psf->dataoffset  = 0 ;
	psf->datalength  = psf->blockwidth * psf->sf.samples ;
	psf->filelength  = psf->datalength ;
	psf->error       = 0 ;

	if (subformat == SF_FORMAT_PCM_S8)
		psf->chars = SF_CHARS_SIGNED ;
	else if (subformat == SF_FORMAT_PCM_U8)
		psf->chars = SF_CHARS_UNSIGNED ;
	
	if ((error = pcm_write_init (psf)))
		return error ;
		
	return 0 ;
} /* raw_open_write */

/*------------------------------------------------------------------------------
*/

static long
raw_seek   (SF_PRIVATE *psf, long offset, int whence)
{	long position ;

	if (! (psf->blockwidth && psf->datalength))
	{	psf->error = SFE_BAD_SEEK ;
		return	((long) -1) ;
		} ;

	position = ftell (psf->file) ;
	offset = offset * psf->blockwidth ;
		
	switch (whence)
	{	case SEEK_SET :
				if (offset < 0 || offset > psf->datalength)
				{	psf->error = SFE_BAD_SEEK ;
					return	((long) -1) ;
					} ;
				break ;
				
		case SEEK_CUR :
				if (psf->current + offset < 0 || psf->current + offset > psf->datalength)
				{	psf->error = SFE_BAD_SEEK ;
					return	((long) -1) ;
					} ;
				offset = position + offset ;
				break ;
				
		case SEEK_END :
				if (offset > 0 || psf->datalength + offset < 0)
				{	psf->error = SFE_BAD_SEEK ;
					return	((long) -1) ;
					} ;
				offset = psf->datalength + offset ;
				break ;
				
		default : 
				psf->error = SFE_BAD_SEEK ;
				return	((long) -1) ;
		} ;
		
	if (psf->mode == SF_MODE_READ)
		fseek (psf->file, offset, SEEK_SET) ;
	else
	{	/* What to do about write??? */ 
		psf->error = SFE_BAD_SEEK ;
		return	((long) -1) ;
		} ;

	psf->current = offset / psf->blockwidth ;

	return psf->current ;
} /* raw_seek */

