/*
** Copyright (C) 2001 Erik de Castro Lopo <erikd@zip.com.au>
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

/*------------------------------------------------------------------------------
** Macros to handle big/little endian issues.
*/

/* The IRCAM magic number is weird in that one byte in the number can have
** values of 0x1, 0x2, 0x03 or 0x04. Hence the need for a marker and a mask.
*/

#define IRCAM_02_MARKER	(MAKE_MARKER (0x00, 0x02, 0xA3, 0x64)) 
#define IRCAM_03_MARKER	(MAKE_MARKER (0x64, 0xA3, 0x03, 0x00)) 
#define IRCAM_04_MARKER	(MAKE_MARKER (0x64, 0xA3, 0x04, 0x00)) 

#define IRCAM_DATA_OFFSET	(1024)

/*------------------------------------------------------------------------------
** Typedefs.
*/

enum
{	IRCAM_PCM_16	= 0x00002,
	IRCAM_FLOAT		= 0x00004,
	IRCAM_ALAW		= 0x10001,
	IRCAM_ULAW		= 0x20001,
	IRCAM_PCM_32	= 0x40004
} ;


/*------------------------------------------------------------------------------
** Private static functions.
*/

static	int		ircam_close		(SF_PRIVATE *psf) ;
static	int		ircam_write_header (SF_PRIVATE *psf) ;

static	int		get_encoding (SF_PRIVATE *psf) ;

static	char*	get_encoding_str (int encoding) ;

/*------------------------------------------------------------------------------
** Public functions.
*/

int
ircam_open_read	(SF_PRIVATE *psf)
{	unsigned int	marker, encoding ;
	float			samplerate ;
	int				error = SFE_NO_ERROR ;
	
	psf_binheader_readf (psf, "pm", 0, &marker) ;
	
	if (marker == IRCAM_03_MARKER)
	{	psf->endian = SF_ENDIAN_LITTLE ;
		
		if (CPU_IS_LITTLE_ENDIAN)
			marker = ENDSWAP_INT (marker) ;
		psf_log_printf (psf, "marker: 0x%X => little endian\n", marker) ;

		psf_binheader_readf (psf, "fll", &samplerate, &(psf->sf.channels), &encoding) ;
		
		psf->sf.samplerate = (int) samplerate ;
		psf_log_printf (psf, "  Sample Rate : %d\n", psf->sf.samplerate) ;
		psf_log_printf (psf, "  Channels    : %d\n", psf->sf.channels) ;
		psf_log_printf (psf, "  Encoding    : %X => %s\n", encoding, get_encoding_str (encoding)) ;
		}
	else if (marker == IRCAM_02_MARKER || marker == IRCAM_04_MARKER)
	{	psf->endian = SF_ENDIAN_BIG ;
		
		if (CPU_IS_BIG_ENDIAN)
			marker = ENDSWAP_INT (marker) ;
		psf_log_printf (psf, "marker: 0x%X => big endian\n", marker) ;

		psf_binheader_readf (psf, "FLL", &samplerate, &(psf->sf.channels), &encoding) ;

		psf->sf.samplerate = (int) samplerate ;
		psf_log_printf (psf, "  Sample Rate : %d\n", psf->sf.samplerate) ;
		psf_log_printf (psf, "  Channels    : %d\n", psf->sf.channels) ;
		psf_log_printf (psf, "  Encoding    : %X => %s\n", encoding, get_encoding_str (encoding)) ;
		}
	else	
		return SFE_IRCAM_NO_MARKER ;
		
	/* Sanit checking for endian-ness detection. */
	if (psf->sf.channels > 256)
		return SFE_IRCAM_BAD_CHANNELS ;

	psf->sf.sections = 1 ;
	psf->sf.seekable = SF_TRUE ;

	switch (encoding)
	{	case  IRCAM_PCM_16 :	
				psf->sf.pcmbitwidth = 16 ;
				psf->bytewidth      = BITWIDTH2BYTES (psf->sf.pcmbitwidth) ;
				psf->blockwidth = psf->sf.channels * psf->bytewidth ;

				if (psf->endian == SF_ENDIAN_BIG)
					psf->sf.format = SF_FORMAT_IRCAM | SF_FORMAT_PCM_BE ;
				else
					psf->sf.format = SF_FORMAT_IRCAM | SF_FORMAT_PCM_LE ;
					
				error = pcm_read_init (psf) ;
				break ;

		case IRCAM_PCM_32 :
				psf->sf.pcmbitwidth = 32 ;
				psf->bytewidth      = BITWIDTH2BYTES (psf->sf.pcmbitwidth) ;
				psf->blockwidth = psf->sf.channels * psf->bytewidth ;

				if (psf->endian == SF_ENDIAN_BIG)
					psf->sf.format = SF_FORMAT_IRCAM | SF_FORMAT_PCM_BE ;
				else
					psf->sf.format = SF_FORMAT_IRCAM | SF_FORMAT_PCM_LE ;
					
				error = pcm_read_init (psf) ;
				break ;
	
		case  IRCAM_FLOAT :
				psf->sf.pcmbitwidth = 32 ;
				psf->bytewidth      = BITWIDTH2BYTES (psf->sf.pcmbitwidth) ;
				psf->blockwidth = psf->sf.channels * psf->bytewidth ;
					
				if (psf->endian == SF_ENDIAN_BIG)
					psf->sf.format = SF_FORMAT_IRCAM | SF_FORMAT_FLOAT_BE ;
				else
					psf->sf.format = SF_FORMAT_IRCAM | SF_FORMAT_FLOAT_LE ;
					
				error = float32_read_init (psf) ;
				break ;

		default : 
				error = SFE_IRCAM_UNKNOWN_FORMAT ;
				break ;
		} ;

	if (error)
		return error ;
		
	psf->dataoffset = IRCAM_DATA_OFFSET ;
	psf->datalength = psf->filelength - psf->dataoffset ;

	if (! psf->sf.samples && psf->blockwidth)
		psf->sf.samples = psf->datalength / psf->blockwidth ;

	psf_log_printf (psf, "  Samples     : %d\n", psf->sf.samples) ;

	psf_binheader_readf (psf, "p", IRCAM_DATA_OFFSET) ;

	return 0 ;
} /* ircam_open_read */

/*------------------------------------------------------------------------------
*/

int
ircam_open_write	(SF_PRIVATE *psf)
{	unsigned int	encoding, subformat ;
	int				error = SFE_NO_ERROR ;

	if ((psf->sf.format & SF_FORMAT_TYPEMASK) != SF_FORMAT_IRCAM)
		return	SFE_BAD_OPEN_FORMAT ;
		
	subformat = psf->sf.format & SF_FORMAT_SUBMASK ;

	if (subformat == SF_FORMAT_ULAW || subformat == SF_FORMAT_ALAW)
		psf->bytewidth = 1 ;
	else
		psf->bytewidth = BITWIDTH2BYTES (psf->sf.pcmbitwidth) ;
		
	psf->sf.seekable = SF_TRUE ;
	psf->error       = 0 ;

	psf->blockwidth  = psf->bytewidth * psf->sf.channels ;
 	psf->dataoffset  = IRCAM_DATA_OFFSET ;
	psf->datalength  = psf->blockwidth * psf->sf.samples ;
	psf->filelength  = psf->datalength + psf->dataoffset ;

	if (! (encoding = ircam_write_header (psf)))
		return psf->error ;

	psf->close        = (func_close)  ircam_close ;
	psf->write_header = (func_wr_hdr) ircam_write_header ;
	
	switch (encoding)
	{	case  IRCAM_PCM_16 :	/* 16-bit linear PCM. */
		case  IRCAM_PCM_32 :	/* 32-bit linear PCM. */
				error = pcm_write_init (psf) ;
				break ;
				
		case  IRCAM_FLOAT :	/* 32-bit linear PCM. */
				error = float32_write_init (psf) ;
				break ;
				
		default :   break ;
		} ;
		
	return error ;
} /* ircam_open_write */

/*------------------------------------------------------------------------------
*/

static int
ircam_close	(SF_PRIVATE  *psf)
{
	return 0 ;
} /* ircam_close */

static int
ircam_write_header (SF_PRIVATE *psf)
{	int		encoding ;
	float	samplerate ;

	/* This also sets psf->endian. */
	encoding = get_encoding (psf) ;
	
	if (! encoding)
	{	psf->error = SFE_BAD_OPEN_FORMAT ;
		return	encoding ;
		} ;

	/* Reset the current header length to zero. */
	psf->header [0] = 0 ;
	psf->headindex = 0 ;
	fseek (psf->file, 0, SEEK_SET) ;
	
	samplerate = psf->sf.samplerate ;

	if (psf->endian == SF_ENDIAN_BIG)
	{	psf_binheader_writef (psf, "mF", IRCAM_02_MARKER, samplerate) ;
		psf_binheader_writef (psf, "LL", psf->sf.channels, encoding) ;
		}
	else if  (psf->endian == SF_ENDIAN_LITTLE)
	{	psf_binheader_writef (psf, "mf", IRCAM_03_MARKER, samplerate) ;
		psf_binheader_writef (psf, "ll", psf->sf.channels, encoding) ;
		}
	else
	{	psf->error = SFE_BAD_OPEN_FORMAT ;
		return	encoding ;
		} ;

	psf_binheader_writef (psf, "z", IRCAM_DATA_OFFSET - psf->headindex) ;
	
	/* Header construction complete so write it out. */
	fwrite (psf->header, psf->headindex, 1, psf->file) ;

	return encoding ;
} /* ircam_write_header */ 

static int
get_encoding (SF_PRIVATE *psf)
{	unsigned int format, bitwidth ;

	format = psf->sf.format & SF_FORMAT_SUBMASK ;
	bitwidth = psf->bytewidth * 8 ;

	/* Default endian-ness is the same as host processor unless overridden. */
	if (format == SF_FORMAT_PCM_BE || format == SF_FORMAT_FLOAT_BE)
		psf->endian = SF_ENDIAN_BIG ;
	else if (format == SF_FORMAT_PCM_LE || format == SF_FORMAT_FLOAT_LE)
		psf->endian = SF_ENDIAN_LITTLE ;
	else if (CPU_IS_BIG_ENDIAN)
		psf->endian = SF_ENDIAN_BIG ;
	else	
		psf->endian = SF_ENDIAN_LITTLE ;

	switch (format)
	{	case SF_FORMAT_ULAW :	return IRCAM_ULAW ;
		case SF_FORMAT_ALAW :	return IRCAM_ALAW ;

		case SF_FORMAT_PCM :
		case SF_FORMAT_PCM_BE :
		case SF_FORMAT_PCM_LE :
				/* For PCM encoding, the header encoding field depends on the bitwidth. */
				switch (bitwidth)
				{	case	16 : return IRCAM_PCM_16 ;
					case	32 : return	IRCAM_PCM_32 ;
					default : break ;
					} ;
				break ;

		case SF_FORMAT_FLOAT :
		case SF_FORMAT_FLOAT_BE :	
		case SF_FORMAT_FLOAT_LE :	
				return IRCAM_FLOAT ;
					
		default : break ;
		} ;

	return 0 ;
} /* get_encoding */

static	char*	
get_encoding_str (int encoding)
{	switch (encoding)
	{	case IRCAM_PCM_16	: return "16 bit PCM" ;
		case IRCAM_FLOAT	: return "32 bit float" ;
		case IRCAM_ALAW		: return "A law" ;
		case IRCAM_ULAW		: return "u law" ;
		case IRCAM_PCM_32	: return "32 bit PCM" ;
		} ;
	return "Unknown encoding" ;
} /* get_encoding_str */

