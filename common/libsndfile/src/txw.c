/*
** Copyright (C) 2002 Erik de Castro Lopo <erikd@zip.com.au>
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

/*===========================================================================
** Yamaha TX16 Sampler Files.
**
** This header parser was written using information from the SoX source code
** and trial and error experimentation. The code here however is all original.
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
#include	"au.h"

/*------------------------------------------------------------------------------
** Markers.
*/

#define LM89_MARKER		(MAKE_MARKER ('L', 'M', '8', '9'))
#define FIVE3_MARKER	(MAKE_MARKER ('5', '3', 0, 0))

#define AU_DATA_OFFSET	24

/*------------------------------------------------------------------------------
** Known AU file encoding types.
*/

/*------------------------------------------------------------------------------
** Private static functions.
*/

static int	txw_close	(SF_PRIVATE *psf) ;
static int  txw_read_header	(SF_PRIVATE *psf) ;
static int	txw_write_header (SF_PRIVATE *psf) ;

/*------------------------------------------------------------------------------
** Public functions.
*/

struct WaveHeader_ {
  unsigned char
    filetype [6],    /* = "LM8953", */
    nulls [10],
    dummy_aeg [6],   /* space for the AEG (never mind this) */
    format,          /* 0x49 = looped, 0xC9 = non-looped */
    sample_rate,     /* 1 = 33 kHz, 2 = 50 kHz, 3 = 16 kHz */
    atc_length [3],  /* I'll get to this... */
    rpt_length [3],
    unused [2] ;     /* set these to null, to be on the safe side */
} ;

#define	ERROR_666	666

int
txw_open	(SF_PRIVATE *psf)
{	int error ;

	if (psf->mode == SFM_READ || (psf->mode == SFM_RDWR && psf->filelength > 0))
	{	if ((error = txw_read_header (psf)))
			return error ;
		} ;

	if (psf->mode == SFM_WRITE || psf->mode == SFM_RDWR)
	{ 	psf->dataoffset = psf_ftell (psf->filedes) ;
		psf->datalength = psf->filelength - psf->dataoffset ;

	 	if (psf_fseek (psf->filedes, psf->dataoffset, SEEK_SET))
			return SFE_BAD_SEEK ;

		psf->close = txw_close ;

		/* Only fill in type major. */
		if (psf->endian == SF_ENDIAN_BIG)
			psf->sf.format = SF_FORMAT_TXW ;
		else if (psf->endian == SF_ENDIAN_LITTLE)
			psf->sf.format = SF_ENDIAN_LITTLE | SF_FORMAT_TXW ;

		psf->sf.format |= SF_FORMAT_PCM_16 ;

		if ((error = txw_write_header (psf)))
			return error ;

		psf->write_header = txw_write_header ;
		} ;

	psf->close = txw_close ;

	error = pcm_init (psf) ;

	return error ;
} /* txw_open */

/*------------------------------------------------------------------------------
*/

static int
txw_read_header	(SF_PRIVATE *psf)
{	unsigned char byte ;
	int		marker, dword ;
	char	*strptr ;

	psf_binheader_readf (psf, "pmm", 0, &marker, &dword) ;

	if (marker != LM89_MARKER || dword != FIVE3_MARKER)
		return ERROR_666 ;
	psf_log_printf (psf, "LM8953\n") ;

	/* Jump 8 bytes (nulls). */
	psf_binheader_readf (psf, "j", 8) ;

	/* Jump 6 bytes. (dummp_aeg) */
	psf_binheader_readf (psf, "j", 6) ;

	/* Read format. */
	psf_binheader_readf (psf, "1", &byte) ;
	switch (byte)
	{	case 0x49 :
				strptr = "looped" ;
				break ;

		case 0xC9 :
				strptr = "non-looped" ;
				break ;

		default :
				psf_log_printf (psf, " Format : 0x%02x => ?????\n", byte & 0xFF) ;
				return ERROR_666 ;
		} ;

	psf_log_printf (psf, " Format : 0x%X => looped\n", byte & 0xFF, strptr) ;

	/* Read format. */
	psf_binheader_readf (psf, "1", &byte) ;
	switch (byte)
	{	case 1 :
				strptr = "33kHz" ;
				psf->sf.samplerate = 33000 ;
				break ;

		case 2 :
				strptr = "50kHz" ;
				psf->sf.samplerate = 50000 ;
				break ;

		case 3 :
				strptr = "16kHz" ;
				psf->sf.samplerate = 16000 ;
				break ;

		default :
				psf_log_printf (psf, " Sample Rate : %d => Unknown\n", byte & 0xFF) ;
				return ERROR_666 ;
				break ;
		} ;

	psf_log_printf (psf, " Sample Rate : %d => %s\n", byte & 0xFF, strptr) ;

	/* Jump 8 bytes (atc_length[3], rpt_length[3], unused[2]). */
	psf_binheader_readf (psf, "j", 8) ;

	psf->sf.channels = 1 ;

	return 0 ;
} /* txw_read_header */

/*-
int
txw_open_write	(SF_PRIVATE *psf)
{	int		subformat ;

	if ((psf->sf.format & SF_FORMAT_TYPEMASK) != SF_FORMAT_AU)
		return	SFE_BAD_OPEN_FORMAT ;

	subformat = psf->sf.format & SF_FORMAT_SUBMASK ;

	psf->endian = psf->sf.format & SF_FORMAT_ENDMASK ;
	if (CPU_IS_LITTLE_ENDIAN && psf->endian == SF_ENDIAN_CPU)
		psf->endian = SF_ENDIAN_LITTLE ;
	else if (psf->endian != SF_ENDIAN_LITTLE)
		psf->endian = SF_ENDIAN_BIG ;

	psf->error       = 0 ;

	return 0 ;
} /+* txw_open_write *+/
-*/

static int
txw_write_header (SF_PRIVATE *psf)
{
	return (psf ? 0 : 1) ;
} /* txw_write_header */

static int
txw_close	(SF_PRIVATE  *psf)
{
	if (psf->mode == SFM_WRITE || psf->mode == SFM_RDWR)
	{	/*  
		**	Now we know for certain the length of the file we can
		**  re-write correct values for the datasize header element.
		*/

		psf_fseek (psf->filedes, 0, SEEK_END) ;
		psf->filelength = psf_ftell (psf->filedes) ;

		psf->datalength = psf->filelength - AU_DATA_OFFSET ;
		psf_fseek (psf->filedes, 0, SEEK_SET) ;

		psf->sf.samples = psf->datalength / psf->blockwidth ;
		txw_write_header (psf) ;
		} ;

	if (psf->fdata)
		free (psf->fdata) ;
	psf->fdata = NULL ;

	return 0 ;
} /* txw_close */

