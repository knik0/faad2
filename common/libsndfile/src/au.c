/*
** Copyright (C) 1999-2002 Erik de Castro Lopo <erikd@zip.com.au>
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

static	int 			au_format_to_encoding	(int format) ;

static int				au_write_header (SF_PRIVATE *psf) ;
static int				au_read_header (SF_PRIVATE *psf) ;

/*------------------------------------------------------------------------------
** Public function.
*/

int
au_open	(SF_PRIVATE *psf)
{	int		encoding, subformat ;
	int		error = 0 ;

	if (psf->mode == SFM_READ || (psf->mode == SFM_RDWR && psf->filelength > 0))
	{	if ((error = au_read_header (psf)))
			return error ; 
		} ;

	if ((psf->sf.format & SF_FORMAT_TYPEMASK) != SF_FORMAT_AU)
		return	SFE_BAD_OPEN_FORMAT ;

	subformat = psf->sf.format & SF_FORMAT_SUBMASK ;
	
	if (psf->mode == SFM_WRITE || psf->mode == SFM_RDWR)
	{	psf->endian = psf->sf.format & SF_FORMAT_ENDMASK ; 
		if (CPU_IS_LITTLE_ENDIAN && psf->endian == SF_ENDIAN_CPU)
			psf->endian = SF_ENDIAN_LITTLE ;
		else if (psf->endian != SF_ENDIAN_LITTLE)
			psf->endian = SF_ENDIAN_BIG ;
	
		if (! (encoding = au_write_header (psf)))
			return psf->error ;
	
		psf->write_header = au_write_header ;
		} ;

	psf->close = au_close ;
	
	psf->blockwidth  = psf->bytewidth * psf->sf.channels ;

	switch (subformat)
	{	case  SF_FORMAT_ULAW :	/* 8-bit Ulaw encoding. */
				ulaw_init (psf) ;
				break ;
	
		case  SF_FORMAT_PCM_S8 :	/* 8-bit linear PCM. */
				psf->chars = SF_CHARS_SIGNED ;
				error = pcm_init (psf) ;				
				break ;

		case  SF_FORMAT_PCM_16 :	/* 16-bit linear PCM. */
		case  SF_FORMAT_PCM_24 :	/* 24-bit linear PCM */
		case  SF_FORMAT_PCM_32 :	/* 32-bit linear PCM. */
				error = pcm_init (psf) ;
				break ;

		case  SF_FORMAT_FLOAT :	/* 32-bit floats. */
				error = float32_init (psf) ;
				break ;
				
		case  SF_FORMAT_DOUBLE :	/* 64-bit double precision floats. */
				error = double64_init (psf) ;
				break ;
				
		case  SF_FORMAT_ALAW :	/* 8-bit Alaw encoding. */
				alaw_init (psf) ;
				break ;
	
		case  SF_FORMAT_G721_32 :  
				if (psf->mode == SFM_READ)
					error = au_g72x_reader_init (psf, AU_H_G721_32) ;
				else if (psf->mode == SFM_WRITE)
					error = au_g72x_writer_init (psf, AU_H_G721_32) ;
				psf->sf.seekable = SF_FALSE ;
				break ;

		case  SF_FORMAT_G723_24 :  
				if (psf->mode == SFM_READ)
					error = au_g72x_reader_init (psf, AU_H_G723_24) ;
				else if (psf->mode == SFM_WRITE)
					error = au_g72x_writer_init (psf, AU_H_G723_24) ;
				psf->sf.seekable = SF_FALSE ;
				break ;

		default :   break ;
		} ;
		
	return error ;
} /* au_open */

int
au_nh_open	(SF_PRIVATE *psf)
{	
	if (psf->mode == SFM_RDWR)
		return SFE_BAD_OPEN_FORMAT ;

	if (psf_fseek (psf->filedes, psf->dataoffset, SEEK_SET))
		return SFE_BAD_SEEK ;

	psf_log_printf (psf, "Headers-less u-law encoded file.\n") ;
	psf_log_printf (psf, "Setting up for 8kHz, mono, u-law.\n") ;
	
	psf->sf.format = SF_FORMAT_AU | SF_FORMAT_ULAW ;

 	psf->dataoffset     = 0 ;
	psf->endian         = 0 ;  /* Irrelevant but it must be something. */
	psf->sf.samplerate	= 8000 ;
	psf->sf.channels 	= 1 ;
	psf->bytewidth   	= 1 ;	/* Before decoding */
					
	ulaw_init (psf) ;
	
	psf->close = au_close ;

	psf->blockwidth = 1 ;
	psf->sf.samples = psf->filelength ;
	psf->datalength = psf->filelength - AU_DATA_OFFSET ;

	return 0 ;
} /* au_nh_open */

/*------------------------------------------------------------------------------
*/

static int
au_close	(SF_PRIVATE  *psf)
{
	if (psf->mode == SFM_WRITE || psf->mode == SFM_RDWR)
	{	/*  Now we know for certain the length of the file we can
		 *  re-write correct values for the datasize header element.
		 */

		psf_fseek (psf->filedes, 0, SEEK_END) ;
		psf->filelength = psf_ftell (psf->filedes) ;

		psf->datalength = psf->filelength - AU_DATA_OFFSET ;
		psf_fseek (psf->filedes, 0, SEEK_SET) ;
		
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

	encoding = au_format_to_encoding (psf->sf.format & SF_FORMAT_SUBMASK) ;
	if (! encoding)
	{	psf->error = SFE_BAD_OPEN_FORMAT ;
		return	encoding ;
		} ;

	/* Reset the current header length to zero. */
	psf->header [0] = 0 ;
	psf->headindex = 0 ;
	psf_fseek (psf->filedes, 0, SEEK_SET) ;

	if (psf->endian == SF_ENDIAN_BIG)
	{	psf_binheader_writef (psf, "Em4", DOTSND_MARKER, AU_DATA_OFFSET) ;
		psf_binheader_writef (psf, "Et8444", psf->datalength, encoding, psf->sf.samplerate, psf->sf.channels) ;
		}
	else if  (psf->endian == SF_ENDIAN_LITTLE)
	{	psf_binheader_writef (psf, "em4", DNSDOT_MARKER, AU_DATA_OFFSET) ;
		psf_binheader_writef (psf, "et8444", psf->datalength, encoding, psf->sf.samplerate, psf->sf.channels) ;
		}
	else
	{	psf->error = SFE_BAD_OPEN_FORMAT ;
		return	encoding ;
		} ;

	/* Header construction complete so write it out. */
	psf_fwrite (psf->header, psf->headindex, 1, psf->filedes) ;

	psf->dataoffset = psf->headindex ;
	
	return encoding ;
} /* au_write_header */ 

static int
au_format_to_encoding (int format)
{	
	switch (format)
	{	case SF_FORMAT_PCM_S8 : 	return AU_ENCODING_PCM_8 ;		
		case SF_FORMAT_PCM_16 :		return AU_ENCODING_PCM_16 ;
		case SF_FORMAT_PCM_24 : 	return AU_ENCODING_PCM_24 ;
		case SF_FORMAT_PCM_32 : 	return AU_ENCODING_PCM_32 ;

		case SF_FORMAT_FLOAT :		return AU_ENCODING_FLOAT ;
		case SF_FORMAT_DOUBLE :		return AU_ENCODING_DOUBLE ;

		case SF_FORMAT_ULAW :		return AU_ENCODING_ULAW_8 ;
		case SF_FORMAT_ALAW :		return AU_ENCODING_ALAW_8 ;
		
		case SF_FORMAT_G721_32 : 	return AU_ENCODING_ADPCM_G721_32 ;
		case SF_FORMAT_G723_24 :	return AU_ENCODING_ADPCM_G723_24 ;
		
		default : break ;
		} ;
	return 0 ;
} /* au_format_to_encoding */

static int
au_read_header (SF_PRIVATE *psf)
{	AU_FMT	au_fmt ;
	int		marker, dword ;

	psf_binheader_readf (psf, "pm", 0, &marker) ;
	psf_log_printf (psf, "%D\n", marker) ;

	if (marker == DOTSND_MARKER)
	{	psf->endian = SF_ENDIAN_BIG ;

		psf_binheader_readf (psf, "E44444", &(au_fmt.dataoffset), &(au_fmt.datasize),
					&(au_fmt.encoding), &(au_fmt.samplerate), &(au_fmt.channels)) ;
		}
	else if (marker == DNSDOT_MARKER)
	{	psf->endian = SF_ENDIAN_LITTLE ;
		psf_binheader_readf (psf, "e44444", &(au_fmt.dataoffset), &(au_fmt.datasize),
					&(au_fmt.encoding), &(au_fmt.samplerate), &(au_fmt.channels)) ;
		}
	else
		return SFE_AU_NO_DOTSND ;
		
	
	psf_log_printf (psf, "  Data Offset : %d\n", au_fmt.dataoffset) ;
	
	if (au_fmt.datasize == -1 || au_fmt.dataoffset + au_fmt.datasize == psf->filelength)
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

 	if (psf_fseek (psf->filedes, psf->dataoffset, SEEK_SET) != psf->dataoffset)
		return SFE_BAD_SEEK ;

	psf->close = au_close ;
	
	psf->sf.samplerate	= au_fmt.samplerate ;
	psf->sf.channels 	= au_fmt.channels ;
					
	/* Only fill in type major. */
	if (psf->endian == SF_ENDIAN_BIG)
		psf->sf.format = SF_FORMAT_AU ;
	else if (psf->endian == SF_ENDIAN_LITTLE)
		psf->sf.format = SF_ENDIAN_LITTLE | SF_FORMAT_AU ;

	psf_log_printf (psf, "  Encoding    : %d => ", au_fmt.encoding) ;
	
	psf->sf.format = psf->sf.format & SF_FORMAT_ENDMASK ;
				
	switch (au_fmt.encoding)
	{	case  AU_ENCODING_ULAW_8 :	
				psf->sf.format |= SF_FORMAT_AU | SF_FORMAT_ULAW ;
				psf->bytewidth  = 1 ;	/* Before decoding */
				psf_log_printf (psf, "8-bit ISDN u-law\n") ;
				break  ;
														
		case  AU_ENCODING_PCM_8 :	
				psf->sf.format |= SF_FORMAT_AU | SF_FORMAT_PCM_S8 ;
				psf->bytewidth  = 1 ;
				psf_log_printf (psf, "8-bit linear PCM\n") ;
				break  ;

		case  AU_ENCODING_PCM_16 :	
				psf->sf.format |= SF_FORMAT_AU | SF_FORMAT_PCM_16 ;
				psf->bytewidth  = 2 ;
				psf_log_printf (psf, "16-bit linear PCM\n") ;
				break  ;

		case  AU_ENCODING_PCM_24 :	
				psf->sf.format |= SF_FORMAT_AU | SF_FORMAT_PCM_24 ;
				psf->bytewidth  = 3 ;
				psf_log_printf (psf, "24-bit linear PCM\n") ;
				break  ;

		case  AU_ENCODING_PCM_32 :	
				psf->sf.format |= SF_FORMAT_AU | SF_FORMAT_PCM_32 ;
				psf->bytewidth  = 4 ;
				psf_log_printf (psf, "32-bit linear PCM\n") ;
				break  ;
					
		case  AU_ENCODING_FLOAT :	
				psf->sf.format |= SF_FORMAT_AU | SF_FORMAT_FLOAT ;
				psf->bytewidth  = 4 ;
				psf_log_printf (psf, "32-bit float\n") ;
				break  ;
					
		case  AU_ENCODING_DOUBLE :	
				psf->sf.format |= SF_FORMAT_AU | SF_FORMAT_DOUBLE ;
				psf->bytewidth  = 8 ;
				psf_log_printf (psf, "64-bit double precision float\n") ;
				break  ;
					
		case  AU_ENCODING_ALAW_8 :
				psf->sf.format |= SF_FORMAT_AU | SF_FORMAT_ALAW ;
				psf->bytewidth  = 1 ;	/* Before decoding */
				psf_log_printf (psf, "8-bit ISDN A-law\n") ;
				break  ;
					
		case  AU_ENCODING_ADPCM_G721_32 :  
				psf->sf.format |= SF_FORMAT_AU | SF_FORMAT_G721_32 ;
				psf->bytewidth  = 0 ;
				psf_log_printf (psf, "G721 32kbs ADPCM\n") ;
				break  ;
										
		case  AU_ENCODING_ADPCM_G723_24 :  
				psf->sf.format |= SF_FORMAT_AU | SF_FORMAT_G723_24 ;
				psf->bytewidth  = 0 ;
				psf_log_printf (psf, "G723 24kbs ADPCM\n") ;
				break  ;
										
		case  AU_ENCODING_NEXT :	
				psf_log_printf (psf, "Weird NeXT encoding format (unsupported)\n") ;
				break  ;

		default : 
				psf_log_printf (psf, "Unknown!!\n") ;
				break ;
		} ;

	psf_log_printf (psf, "  Sample Rate : %d\n", au_fmt.samplerate) ;
	psf_log_printf (psf, "  Channels    : %d\n", au_fmt.channels) ;
	
	psf->blockwidth = psf->sf.channels * psf->bytewidth ;
	
	if (! psf->sf.samples && psf->blockwidth)
		psf->sf.samples = (psf->filelength - psf->dataoffset) / psf->blockwidth ;

	return 0 ;
} /* au_read_header */
