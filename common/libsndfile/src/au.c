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
#include	"au.h"

/*------------------------------------------------------------------------------
** Macros to handle big/little endian issues.
*/

#define DOTSND_MARKER	(MAKE_MARKER ('.', 's', 'n', 'd')) 
#define DNSDOT_MARKER	(MAKE_MARKER ('d', 'n', 's', '.')) 

#define AU_DATA_OFFSET	24

/*------------------------------------------------------------------------------
** Known AU file encoding types.
*/

enum
{	AU_ENCODING_ULAW_8					= 1,	/* 8-bit u-law samples */
	AU_ENCODING_PCM_8					= 2,	/* 8-bit linear samples */
	AU_ENCODING_PCM_16					= 3,	/* 16-bit linear samples */
	AU_ENCODING_PCM_24					= 4,	/* 24-bit linear samples */
	AU_ENCODING_PCM_32					= 5,	/* 32-bit linear samples */
	
	AU_ENCODING_FLOAT					= 6,	/* floating-point samples */
	AU_ENCODING_DOUBLE					= 7,	/* double-precision float samples */
	AU_ENCODING_INDIRECT				= 8,	/* fragmented sampled data */
	AU_ENCODING_NESTED					= 9,	/* ? */
	AU_ENCODING_DSP_CORE				= 10,	/* DSP program */
	AU_ENCODING_DSP_DATA_8				= 11,	/* 8-bit fixed-point samples */
	AU_ENCODING_DSP_DATA_16				= 12,	/* 16-bit fixed-point samples */
	AU_ENCODING_DSP_DATA_24				= 13,	/* 24-bit fixed-point samples */
	AU_ENCODING_DSP_DATA_32				= 14,	/* 32-bit fixed-point samples */

	AU_ENCODING_DISPLAY					= 16,	/* non-audio display data */
	AU_ENCODING_MULAW_SQUELCH			= 17,	/* ? */
	AU_ENCODING_EMPHASIZED				= 18,	/* 16-bit linear with emphasis */
	AU_ENCODING_NEXT					= 19,	/* 16-bit linear with compression (NEXT) */
	AU_ENCODING_COMPRESSED_EMPHASIZED	= 20,	/* A combination of the two above */
	AU_ENCODING_DSP_COMMANDS			= 21,	/* Music Kit DSP commands */
	AU_ENCODING_DSP_COMMANDS_SAMPLES	= 22,	/* ? */

	AU_ENCODING_ADPCM_G721_32			= 23,	/* G721 32 kbs ADPCM - 4 bits per sample. */
	AU_ENCODING_ADPCM_G722				= 24,
	AU_ENCODING_ADPCM_G723_24			= 25,	/* G723 24 kbs ADPCM - 3 bits per sample. */
	AU_ENCODING_ADPCM_G723_5			= 26,
	
	AU_ENCODING_ALAW_8					= 27
} ;

/*------------------------------------------------------------------------------
** Typedefs.
*/
 
typedef	struct
{	int		dataoffset ;
	int		datasize ;
	int		encoding ;
    int		samplerate ;
    int		channels ;
} AU_FMT ;


/*------------------------------------------------------------------------------
** Private static functions.
*/

static	int				au_close		(SF_PRIVATE *psf) ;

static	int 			get_encoding	(unsigned int format, unsigned int	bitwidth) ;
static	char const* 	get_encoding_str(int format) ;

static int				au_write_header (SF_PRIVATE *psf) ;

/*
static	void			endswap_au_fmt	(AU_FMT *pau_fmt) ;
*/

/*------------------------------------------------------------------------------
** Public functions.
*/

int
au_open_read	(SF_PRIVATE *psf)
{	AU_FMT			au_fmt ;
	unsigned int	marker, dword ;
	int				error = SFE_NO_ERROR ;
	
	psf_binheader_readf (psf, "pm", 0, &marker) ;
	psf_log_printf (psf, "%D\n", marker) ;

	if (marker == DOTSND_MARKER)
	{	psf->endian = SF_ENDIAN_BIG ;

		psf_binheader_readf (psf, "LLLLL", &(au_fmt.dataoffset), &(au_fmt.datasize),
					&(au_fmt.encoding), &(au_fmt.samplerate), &(au_fmt.channels)) ;
		}
	else if (marker == DNSDOT_MARKER)
	{	psf->endian = SF_ENDIAN_LITTLE ;
		psf_binheader_readf (psf, "lllll", &(au_fmt.dataoffset), &(au_fmt.datasize),
					&(au_fmt.encoding), &(au_fmt.samplerate), &(au_fmt.channels)) ;
		}
	else
		return SFE_AU_NO_DOTSND ;
		
	
	psf_log_printf (psf, "  Data Offset : %d\n", au_fmt.dataoffset) ;
	
	if (au_fmt.datasize == 0xFFFFFFFF || au_fmt.dataoffset + au_fmt.datasize == psf->filelength)
		psf_log_printf (psf, "  Data Size   : %d\n", au_fmt.datasize) ;
	else if (au_fmt.dataoffset + au_fmt.datasize < psf->filelength)
	{	psf->filelength = au_fmt.dataoffset + au_fmt.dataoffset ;
		psf_log_printf (psf, "  Data Size   : %d\n", au_fmt.datasize) ;
		}
	else
	{	dword = psf->filelength - au_fmt.dataoffset ;
		psf_log_printf (psf, "  Data Size   : %d (should be %d)\n", au_fmt.datasize, dword) ;
		au_fmt.datasize = dword ;
		} ;
		
 	psf->dataoffset = au_fmt.dataoffset ;
	psf->datalength = psf->filelength - psf->dataoffset ;

 	psf->current  = 0 ;
 	
 	if (fseek (psf->file, psf->dataoffset, SEEK_SET))
		return SFE_BAD_SEEK ;

	psf->close = (func_close) au_close ;
	
	psf->sf.samplerate	= au_fmt.samplerate ;
	psf->sf.channels 	= au_fmt.channels ;
					
	/* Only fill in type major. */
	if (psf->endian == SF_ENDIAN_BIG)
		psf->sf.format = SF_FORMAT_AU ;
	else if (psf->endian == SF_ENDIAN_LITTLE)
		psf->sf.format = SF_FORMAT_AULE ;

	psf->sf.sections 	= 1 ;

	psf_log_printf (psf, "  Encoding    : %d => %s\n", au_fmt.encoding, get_encoding_str (au_fmt.encoding)) ;

	psf_log_printf (psf, "  Sample Rate : %d\n", au_fmt.samplerate) ;
	psf_log_printf (psf, "  Channels    : %d\n", au_fmt.channels) ;
	
	switch (au_fmt.encoding)
	{	case  AU_ENCODING_PCM_8 :	
				psf->sf.pcmbitwidth = 8 ;
				psf->bytewidth      = BITWIDTH2BYTES (psf->sf.pcmbitwidth) ;
				psf->blockwidth = psf->sf.channels * psf->bytewidth ;

				psf->sf.format |= SF_FORMAT_PCM ;

				psf->chars = SF_CHARS_SIGNED ;
									
				if ((error = pcm_read_init (psf)))
					return error ;
				break ;

		case  AU_ENCODING_PCM_16 :	
				psf->sf.pcmbitwidth = 16 ;
				psf->bytewidth      = BITWIDTH2BYTES (psf->sf.pcmbitwidth) ;
				psf->blockwidth = psf->sf.channels * psf->bytewidth ;

				psf->sf.format |= SF_FORMAT_PCM ;
					
				if ((error = pcm_read_init (psf)))
					return error ;
				break ;

		case  AU_ENCODING_PCM_24 :	
				psf->sf.pcmbitwidth = 24 ;
				psf->bytewidth      = BITWIDTH2BYTES (psf->sf.pcmbitwidth) ;
				psf->blockwidth = psf->sf.channels * psf->bytewidth ;
					
				psf->sf.format |= SF_FORMAT_PCM ;

				if ((error = pcm_read_init (psf)))
					return error ;
				break ;

		case  AU_ENCODING_PCM_32 :	
				psf->sf.pcmbitwidth = 32 ;
				psf->bytewidth      = BITWIDTH2BYTES (psf->sf.pcmbitwidth) ;
				psf->blockwidth = psf->sf.channels * psf->bytewidth ;
					
				psf->sf.format |= SF_FORMAT_PCM ;
					
				if ((error = pcm_read_init (psf)))
					return error ;
				break ;
					
		case  AU_ENCODING_FLOAT :	
				psf->sf.pcmbitwidth = 32 ;
				psf->bytewidth      = BITWIDTH2BYTES (psf->sf.pcmbitwidth) ;
				psf->blockwidth = psf->sf.channels * psf->bytewidth ;
					
				psf->sf.format |= SF_FORMAT_FLOAT ;

				float32_read_init (psf) ;
				break ;

		default : 
				error = SFE_AU_UNKNOWN_FORMAT ;
				break ;
		} ;

	if (error)
		return error ;

	if (! psf->sf.samples && psf->blockwidth)
		psf->sf.samples = au_fmt.datasize / psf->blockwidth ;


	return 0 ;
} /* au_open_read */


/*------------------------------------------------------------------------------
*/

int
au_open_write	(SF_PRIVATE *psf)
{	unsigned int	encoding, format, subformat ;
	int				error = 0 ;

	format = psf->sf.format & SF_FORMAT_TYPEMASK ;
	subformat = psf->sf.format & SF_FORMAT_SUBMASK ;
	if (format == SF_FORMAT_AU)
		psf->endian = SF_ENDIAN_BIG ;
	else if (format == SF_FORMAT_AULE)
		psf->endian = SF_ENDIAN_LITTLE ;
	else
		return	SFE_BAD_OPEN_FORMAT ;
		
	if (subformat == SF_FORMAT_ULAW || subformat == SF_FORMAT_ALAW || 
		subformat == SF_FORMAT_G721_32 || subformat == SF_FORMAT_G723_24)
		psf->bytewidth = 1 ;
	else
		psf->bytewidth = BITWIDTH2BYTES (psf->sf.pcmbitwidth) ;
		
	psf->sf.seekable = SF_TRUE ;
	psf->error       = 0 ;

	psf->blockwidth  = psf->bytewidth * psf->sf.channels ;
 	psf->dataoffset  = AU_DATA_OFFSET ;
	psf->datalength  = psf->blockwidth * psf->sf.samples ;
	psf->filelength  = psf->datalength + psf->dataoffset ;

	if (! (encoding = au_write_header (psf)))
		return psf->error ;

	psf->close        = (func_close)  au_close ;
	psf->write_header = (func_wr_hdr) au_write_header ;
	
	switch (encoding)
	{	case  AU_ENCODING_PCM_8 :	/* 8-bit linear PCM. */
				psf->chars = SF_CHARS_SIGNED ;

				error = pcm_write_init (psf) ;				
				break ;

		case  AU_ENCODING_PCM_16 :	/* 16-bit linear PCM. */
		case  AU_ENCODING_PCM_24 :	/* 24-bit linear PCM */
		case  AU_ENCODING_PCM_32 :	/* 32-bit linear PCM. */
				error = pcm_write_init (psf) ;
				break ;

		case  AU_ENCODING_FLOAT :	/* 32-bit linear PCM. */
				error = float32_write_init (psf) ;
				break ;
				
		default :   break ;
		} ;
		
	return error ;
} /* au_open_write */

/*------------------------------------------------------------------------------
*/

static int
au_close	(SF_PRIVATE  *psf)
{
	if (psf->mode == SF_MODE_WRITE)
	{	/*  Now we know for certain the length of the file we can
		 *  re-write correct values for the datasize header element.
		 */

		fseek (psf->file, 0, SEEK_END) ;
		psf->filelength = ftell (psf->file) ;

		psf->datalength = psf->filelength - AU_DATA_OFFSET ;
		fseek (psf->file, 0, SEEK_SET) ;
		
		psf->sf.samples = psf->datalength / psf->blockwidth ;
		au_write_header (psf) ;
		} ;

	if (psf->fdata)
		free (psf->fdata) ;
	psf->fdata = NULL ;
	
	return 0 ;
} /* au_close */

static int
au_write_header (SF_PRIVATE *psf)
{	int		encoding ;

	encoding = get_encoding (psf->sf.format & SF_FORMAT_SUBMASK, psf->bytewidth * 8) ;
	if (! encoding)
	{	psf->error = SFE_BAD_OPEN_FORMAT ;
		return	encoding ;
		} ;

	/* Reset the current header length to zero. */
	psf->header [0] = 0 ;
	psf->headindex = 0 ;
	fseek (psf->file, 0, SEEK_SET) ;

	if (psf->endian == SF_ENDIAN_BIG)
	{	psf_binheader_writef (psf, "mL", DOTSND_MARKER, AU_DATA_OFFSET) ;
		psf_binheader_writef (psf, "LLLL", psf->datalength, encoding, psf->sf.samplerate, psf->sf.channels) ;
		}
	else if  (psf->endian == SF_ENDIAN_LITTLE)
	{	psf_binheader_writef (psf, "ml", DNSDOT_MARKER, AU_DATA_OFFSET) ;
		psf_binheader_writef (psf, "llll", psf->datalength, encoding, psf->sf.samplerate, psf->sf.channels) ;
		}
	else
	{	psf->error = SFE_BAD_OPEN_FORMAT ;
		return	encoding ;
		} ;

	/* Header construction complete so write it out. */
	fwrite (psf->header, psf->headindex, 1, psf->file) ;

	psf->dataoffset = psf->headindex ;
	
	return encoding ;
} /* au_write_header */ 

static int
get_encoding (unsigned int format, unsigned int	bitwidth)
{	if (format == SF_FORMAT_ULAW)
		return AU_ENCODING_ULAW_8 ;
		
	if (format == SF_FORMAT_ALAW)
		return AU_ENCODING_ALAW_8 ;

	if (format == SF_FORMAT_G721_32)
		return AU_ENCODING_ADPCM_G721_32 ;

	if (format == SF_FORMAT_G723_24)
		return AU_ENCODING_ADPCM_G723_24 ;

	if (format == SF_FORMAT_FLOAT)
		return AU_ENCODING_FLOAT ;

	if (format != SF_FORMAT_PCM)
		return 0 ;

	/* For PCM encoding, the header encoding field depends on the bitwidth. */
	switch (bitwidth)
	{	case	8  : return AU_ENCODING_PCM_8 ;
		case	16 : return AU_ENCODING_PCM_16 ;
		case	24 : return AU_ENCODING_PCM_24 ;
		case	32 : return	AU_ENCODING_PCM_32 ;
		default : break ;
		} ;
	return 0 ;
} /* get_encoding */

/*-
static void
endswap_au_fmt (AU_FMT *pau_fmt)
{	pau_fmt->dataoffset = ENDSWAP_INT (pau_fmt->dataoffset) ;
	pau_fmt->datasize   = ENDSWAP_INT (pau_fmt->datasize) ;
	pau_fmt->encoding   = ENDSWAP_INT (pau_fmt->encoding) ;
    pau_fmt->samplerate = ENDSWAP_INT (pau_fmt->samplerate) ;
    pau_fmt->channels   = ENDSWAP_INT (pau_fmt->channels) ;
} /+* endswap_au_fmt *+/
-*/

static	char const* 
get_encoding_str (int format)
{	switch (format)
	{	case  AU_ENCODING_ULAW_8 :	
				return "8-bit ISDN u-law" ;
										
		case  AU_ENCODING_PCM_8 :	
				return "8-bit linear PCM" ;

		case  AU_ENCODING_PCM_16 :	
				return "16-bit linear PCM" ;

		case  AU_ENCODING_PCM_24 :	
				return "24-bit linear PCM" ;

		case  AU_ENCODING_PCM_32 :	
				return "32-bit linear PCM" ;
					
		case  AU_ENCODING_FLOAT :	
				return "32-bit float" ;
					
		case  AU_ENCODING_ALAW_8 :
				return "8-bit ISDN A-law" ;
					
		case  AU_ENCODING_ADPCM_G721_32 :  
				return "G721 32kbs ADPCM" ;
										
		case  AU_ENCODING_ADPCM_G723_24 :  
				return "G723 24kbs ADPCM" ;
										
		case  AU_ENCODING_NEXT :	
				return "Weird NeXT encoding format (unsupported)" ;
		} ;
	return "Unknown!!" ;
} /* get_encoding_str */	


