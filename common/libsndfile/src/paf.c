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

#include	"config.h"
#include	"sndfile.h"
#include	"sfendian.h"
#include	"floatcast.h"
#include	"common.h"

/*------------------------------------------------------------------------------
** Macros to handle big/little endian issues.
*/

#define FAP_MARKER	(MAKE_MARKER ('f', 'a', 'p', ' ')) 
#define PAF_MARKER	(MAKE_MARKER (' ', 'p', 'a', 'f')) 

/*------------------------------------------------------------------------------
** Other defines.
*/

#define	PAF_HEADER_LENGTH 	2048

/*------------------------------------------------------------------------------
** Typedefs.
*/

typedef	struct
{	unsigned int	version ;
	unsigned int	endianness ;
    unsigned int	samplerate ;
    unsigned int	format ;
	unsigned int	channels ;
	unsigned int	source ;
} PAF_FMT ;

typedef struct
{	unsigned int	index, blocks, channels, samplesperblock, blockcount, blocksize, samplecount ;
	unsigned char	*block ;
	int				*samples ;
	unsigned char	data [1] ; /* Data size fixed during malloc (). */
} PAF24_PRIVATE ;

/*------------------------------------------------------------------------------
** Private static functions.
*/

static int paf24_reader_init (SF_PRIVATE  *psf) ;
static int paf24_writer_init (SF_PRIVATE  *psf) ;

static int paf24_read_block (SF_PRIVATE  *psf, PAF24_PRIVATE *ppaf24) ;
	
static int paf24_close (SF_PRIVATE  *psf) ;

static int paf24_read_s (SF_PRIVATE *psf, short *ptr, int len) ;
static int paf24_read_i (SF_PRIVATE *psf, int *ptr, int len) ;
static int paf24_read_f (SF_PRIVATE *psf, float *ptr, int len) ;
static int paf24_read_d (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;

static int paf24_write_s (SF_PRIVATE *psf, short *ptr, int len) ;
static int paf24_write_i (SF_PRIVATE *psf, int *ptr, int len) ;
static int paf24_write_f (SF_PRIVATE *psf, float *ptr, int len) ;
static int paf24_write_d (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;

static long paf24_seek (SF_PRIVATE *psf, long offset, int whence) ;

/*
static void	endswap_paf_fmt (PAF_FMT *ppaf_fmt) ;
*/

/*------------------------------------------------------------------------------
** Public functions.
*/

int
paf_open_read	(SF_PRIVATE *psf)
{	PAF_FMT			paf_fmt ;
	unsigned int	marker ;
	int				error ;
	
	psf_binheader_readf (psf, "pm", 0, &marker) ;
	
	psf_log_printf (psf, "Signature   : %D\n", marker) ;

	if (marker == PAF_MARKER)
	{	psf_binheader_readf (psf, "LLLLLL", &(paf_fmt.version), &(paf_fmt.endianness), 
			&(paf_fmt.samplerate), &(paf_fmt.format), &(paf_fmt.channels), &(paf_fmt.source)) ;
		}
	else if (marker == FAP_MARKER)
	{	psf_binheader_readf (psf, "llllll", &(paf_fmt.version), &(paf_fmt.endianness), 
			&(paf_fmt.samplerate), &(paf_fmt.format), &(paf_fmt.channels), &(paf_fmt.source)) ;
		}
	else
		return SFE_PAF_NO_MARKER ;
		
	psf_log_printf (psf, "Version     : %d\n", paf_fmt.version) ;
	if (paf_fmt.version != 0)
	{	psf_log_printf (psf, "*** Bad version number. should be zero.\n") ;
		return SFE_PAF_VERSION ;
		} ;
		
	psf_log_printf (psf, "Endianness  : %d => ", paf_fmt.endianness) ;
	if (paf_fmt.endianness)
		psf_log_printf (psf, "Little\n", paf_fmt.endianness) ;
	else
		psf_log_printf (psf, "Big\n", paf_fmt.endianness) ;
	psf_log_printf (psf, "Sample Rate : %d\n", paf_fmt.samplerate) ;

	if (psf->filelength < PAF_HEADER_LENGTH)
		return SFE_PAF_SHORT_HEADER ;
		
 	psf->dataoffset = PAF_HEADER_LENGTH ;
	psf->datalength = psf->filelength - psf->dataoffset ;

 	psf->current  = 0 ;
	psf->endian   = paf_fmt.endianness ? SF_ENDIAN_LITTLE : SF_ENDIAN_BIG ;

	psf_binheader_readf (psf, "p", psf->dataoffset) ;
	
	psf->sf.samplerate	= paf_fmt.samplerate ;
	psf->sf.channels 	= paf_fmt.channels ;
					
	/* Only fill in type major. */
	psf->sf.format = SF_FORMAT_PAF ;

	psf->sf.sections 	= 1 ;

	psf_log_printf (psf, "Format      : %d => ", paf_fmt.format) ;

	switch (paf_fmt.format)
	{	case  0 :	psf_log_printf (psf, "16 bit linear PCM\n") ;
					psf->sf.pcmbitwidth = 16 ;
					psf->bytewidth      = BITWIDTH2BYTES (psf->sf.pcmbitwidth) ;

					psf->sf.format |= paf_fmt.endianness ? SF_FORMAT_PCM_LE : SF_FORMAT_PCM_BE ; ;
					
					if ((error = pcm_read_init (psf)))
						return error ;

					psf->blockwidth = psf->bytewidth * psf->sf.channels ;

	psf_log_printf (psf, "X blockwidth : %d\n", psf->blockwidth) ;

					if (psf->blockwidth)
						psf->sf.samples = psf->datalength / psf->blockwidth ;
					else
						psf_log_printf (psf, "*** Warning : blockwidth == 0.\n") ;
					
	psf_log_printf (psf, "X samples : %d\n", psf->sf.samples) ;
					break ;

		case  1 :	psf_log_printf (psf, "24 bit linear PCM\n") ;
					psf->sf.pcmbitwidth = 24 ;
					psf->bytewidth      = BITWIDTH2BYTES (psf->sf.pcmbitwidth) ;

					psf->sf.format |= paf_fmt.endianness ? SF_FORMAT_PCM_LE : SF_FORMAT_PCM_BE ; ;
					
					psf->read_short  = (func_short)  paf24_read_s ;
					psf->read_int    = (func_int)    paf24_read_i ;
					psf->read_float  = (func_float) paf24_read_f ;
					psf->read_double = (func_double) paf24_read_d ;

					if ((error = paf24_reader_init (psf)))
						return error ;

					psf->seek_func = (func_seek) paf24_seek ;
					psf->close = (func_close) paf24_close ;

					psf->blockwidth = psf->bytewidth * psf->sf.channels ;
	psf_log_printf (psf, "X blockwidth : %d\n", psf->blockwidth) ;
					psf->sf.samples = 10 * psf->datalength / (32 * psf->sf.channels) ;
	psf_log_printf (psf, "X samples : %d\n", psf->sf.samples) ;
					break ;
		
		default :   psf_log_printf (psf, "Unknown\n") ;
					return SFE_PAF_UNKNOWN_FORMAT ;
					break ;
		} ;

	psf_log_printf (psf, "Channels    : %d\n", paf_fmt.channels) ;
	psf_log_printf (psf, "Source      : %d => ", paf_fmt.source) ;
	
	switch (paf_fmt.source)
	{	case  1 : psf_log_printf (psf, "Analog Recording\n") ;
					break ;
		case  2 : psf_log_printf (psf, "Digital Transfer\n") ;
					break ;
		case  3 : psf_log_printf (psf, "Multi-track Mixdown\n") ;
					break ;
		case  5 : psf_log_printf (psf, "Audio Resulting From DSP Processing\n") ;
					break ;
		default : psf_log_printf (psf, "Unknown\n") ;
					break ;
		} ;

	return 0 ;
} /* paf_open_read */

/*------------------------------------------------------------------------------
*/

int
paf_open_write	(SF_PRIVATE *psf)
{	PAF_FMT			paf_fmt ;
	int				subformat, error, paf_format ;
	
	if ((psf->sf.format & SF_FORMAT_TYPEMASK) != SF_FORMAT_PAF)
		return	SFE_BAD_OPEN_FORMAT ;

	subformat = psf->sf.format & SF_FORMAT_SUBMASK ;

	if (subformat == SF_FORMAT_PCM_BE)
		psf->endian = SF_ENDIAN_BIG ;
	else if (subformat == SF_FORMAT_PCM_LE)
		psf->endian = SF_ENDIAN_LITTLE ;
	else if (CPU_IS_BIG_ENDIAN && subformat == SF_FORMAT_PCM)
		psf->endian = SF_ENDIAN_BIG ;
	else if (CPU_IS_LITTLE_ENDIAN && subformat == SF_FORMAT_PCM)
		psf->endian = SF_ENDIAN_LITTLE ;
	else
		return	SFE_BAD_OPEN_FORMAT ;
		
	psf->bytewidth = BITWIDTH2BYTES (psf->sf.pcmbitwidth) ;

	psf->blockwidth  = psf->bytewidth * psf->sf.channels ;
 	psf->dataoffset  = PAF_HEADER_LENGTH ;
	psf->datalength  = psf->blockwidth * psf->sf.samples ;
	psf->filelength  = psf->datalength + psf->dataoffset ;
	psf->error       = 0 ;

	switch (psf->sf.pcmbitwidth)
	{	case  16 : 	paf_format = 0 ;
					psf->bytewidth = 2 ;
					break ;

		case  24 :	paf_format = 1 ;
					psf->bytewidth = 3 ;
					break ;
	
		default : return SFE_PAF_UNKNOWN_FORMAT ;
		} ;
		
	switch (paf_format)
	{	case  0 :	/* 16-bit linear PCM. */
					if ((error = pcm_write_init (psf)))
						return error ;
					break ;

		case  1 :	/* 24-bit linear PCM */
					psf->write_short  = (func_short)  paf24_write_s ;
					psf->write_int    = (func_int)    paf24_write_i ;
					psf->write_float  = (func_float)  paf24_write_f ;
					psf->write_double = (func_double) paf24_write_d ;
					
					if ((error = paf24_writer_init (psf)))
						return error ;
					
					psf->seek_func = (func_seek) paf24_seek ;
					psf->close = (func_close) paf24_close ;
					break ;

		default :   break ;
		} ;
	
	/* Reset the current header length to zero. */
	psf->header [0] = 0 ;
	psf->headindex = 0 ;
	
	if (psf->endian == SF_ENDIAN_BIG)
	{	/* Marker, version, endianness, samplerate */
		psf_binheader_writef (psf, "mLLL", PAF_MARKER, 0, 0, psf->sf.samplerate) ;
		/* format, channels, source */
		psf_binheader_writef (psf, "LLL", paf_format, psf->sf.channels, paf_fmt.source) ;	
		}
	else if (psf->endian == SF_ENDIAN_LITTLE)
	{	/* Marker, version, endianness, samplerate */
		psf_binheader_writef (psf, "mlll", FAP_MARKER, 0, 1, psf->sf.samplerate) ;
		/* format, channels, source */
		psf_binheader_writef (psf, "lll", paf_format, psf->sf.channels, 0) ;
		} ;

	/* Zero fill to dataoffset. */
	psf_binheader_writef (psf, "z", psf->dataoffset - psf->headindex) ;

	fwrite (psf->header, psf->headindex, 1, psf->file) ;

	return 0 ;
} /* paf_open_write */

/*===============================================================================
**	24 bit PAF files have a really weird encoding. 
**  For a mono file, 10 samples (each being 3 bytes) are packed into a 32 byte 
**	block. The 8 ints in this 32 byte block are then endian swapped (as ints) 
**	if necessary before being written to disk.
**  For a stereo file, blocks of 10 samples from the same channel are encoded 
**  into 32 bytes as fro the mono case. The 32 block bytes are then interleaved 
**	on disk.
**	Reading has to reverse the above process :-).
**	Weird!!!
**
**	The code below attempts to gain efficiency while maintaining readability.
*/

static int 
paf24_reader_init (SF_PRIVATE  *psf)
{	PAF24_PRIVATE	*ppaf24 ; 
	unsigned int	paf24size ;	
	
	paf24size = sizeof (PAF24_PRIVATE) + psf->sf.channels * (32 + 10 * sizeof (int)) ;
	if (! (psf->fdata = malloc (paf24size)))
		return SFE_MALLOC_FAILED ;
		
	ppaf24 = (PAF24_PRIVATE*) psf->fdata ;
	memset (ppaf24, 0, paf24size) ;
	
	ppaf24->channels = psf->sf.channels ;
	ppaf24->block    = (unsigned char*) ppaf24->data ;
	ppaf24->samples  = (int*) (ppaf24->data + 32 * ppaf24->channels) ;
	
	ppaf24->blocksize = 32 * ppaf24->channels ;
	ppaf24->samplesperblock = 10 ;
	
	if (psf->datalength % ppaf24->blocksize)
	{	psf_log_printf (psf, "*** Warning : file seems to be truncated.\n") ;
		ppaf24->blocks = psf->datalength / ppaf24->blocksize  + 1 ;
		}
	else
		ppaf24->blocks = psf->datalength / ppaf24->blocksize ;
		
	psf->sf.samples = ppaf24->samplesperblock * ppaf24->blocks ;

	paf24_read_block (psf, ppaf24) ;	/* Read first block. */
	
	return 0 ;
} /* paf24_reader_init */


static int	
paf24_read_block (SF_PRIVATE  *psf, PAF24_PRIVATE *ppaf24)
{	int				k, *iptr, newsample, channel ;
	unsigned char	*cptr ;
	
	ppaf24->blockcount ++ ;
	ppaf24->samplecount = 0 ;
	
	if (ppaf24->blockcount > ppaf24->blocks)
	{	memset (ppaf24->samples, 0, ppaf24->samplesperblock * ppaf24->channels) ;
		return 1 ;
		} ;
		
	/* Read the block. */

	if ((k = fread (ppaf24->block, 1, ppaf24->blocksize, psf->file)) != ppaf24->blocksize)
		psf_log_printf (psf, "*** Warning : short read (%d != %d).\n", k, ppaf24->blocksize) ;

	/* Do endian swapping if necessary. */

	iptr = (int*) (ppaf24->data) ;
	if ((CPU_IS_BIG_ENDIAN && psf->endian == SF_ENDIAN_LITTLE) ||
			(CPU_IS_LITTLE_ENDIAN && psf->endian == SF_ENDIAN_BIG))
	{	for (k = 0 ; k < 8 * ppaf24->channels ; k++)
			iptr [k] = ENDSWAP_INT (iptr [k]) ;
		} ;
		
	/* Unpack block. */	

	for (k = 0 ; k < 10 * ppaf24->channels ; k++)
	{	channel = k % ppaf24->channels ;
		cptr = ppaf24->block + 32 * channel + 3 * (k / ppaf24->channels) ;
		newsample = (cptr [0] << 8) | (cptr [1] << 16) | (cptr [2] << 24) ;
		ppaf24->samples [k] = newsample / 256 ;
		} ;

	return 1 ;
} /* paf24_read_block */

static int 
paf24_read (SF_PRIVATE *psf, PAF24_PRIVATE *ppaf24, int *ptr, int len)
{	int		count, total = 0, index = 0 ;
	
	while (index < len)
	{	if (ppaf24->blockcount >= ppaf24->blocks && ppaf24->samplecount >= ppaf24->samplesperblock)
		{	memset (&(ptr[index]), 0, (len - index) * sizeof (int)) ;
			return total ;
			} ;
		
		if (ppaf24->samplecount >= ppaf24->samplesperblock)
			paf24_read_block (psf, ppaf24) ;
		
		count = (ppaf24->samplesperblock - ppaf24->samplecount) * ppaf24->channels ;
		count = (len - index > count) ? count : len - index ;
		
		memcpy (&(ptr[index]), &(ppaf24->samples [ppaf24->samplecount * ppaf24->channels]), count * sizeof (int)) ;
		index += count ;
		ppaf24->samplecount += count / ppaf24->channels ;
		total = index ;
		} ;

	return total ;		
} /* paf24_read */

static int 
paf24_read_s (SF_PRIVATE *psf, short *ptr, int len)
{	PAF24_PRIVATE 	*ppaf24 ; 
	int				*iptr ;
	int				k, bufferlen, readcount = 0, count ;
	int				index = 0, total = 0 ;

	if (! psf->fdata)
		return 0 ;
	ppaf24 = (PAF24_PRIVATE*) psf->fdata ;
	
	iptr = (int*) psf->buffer ;
	bufferlen = psf->sf.channels * ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (int) ;
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : len ;
		count = paf24_read (psf, ppaf24, iptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [index+k] = (short) (iptr [k] / 256) ;
		index += readcount ;
		total += count ;
		len -= readcount ;
		} ;
	return total ;
} /* paf24_read_s */

static int 
paf24_read_i (SF_PRIVATE *psf, int *ptr, int len) 
{	PAF24_PRIVATE *ppaf24 ; 
	int				total ;

	if (! psf->fdata)
		return 0 ;
	ppaf24 = (PAF24_PRIVATE*) psf->fdata ;
	
	total = paf24_read (psf, ppaf24, ptr, len) ;

	return total ;
} /* paf24_read_i */

static int 
paf24_read_f (SF_PRIVATE *psf, float *ptr, int len)
{	PAF24_PRIVATE 	*ppaf24 ; 
	int				*iptr ;
	int				k, bufferlen, readcount = 0, count ;
	int				index = 0, total = 0 ;

	if (! psf->fdata)
		return 0 ;
	ppaf24 = (PAF24_PRIVATE*) psf->fdata ;
	
	iptr = (int*) psf->buffer ;
	bufferlen = psf->sf.channels * ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (int) ;
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : len ;
		count = paf24_read (psf, ppaf24, iptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [index+k] = (float) (iptr [k]) ;
		index += readcount ;
		total += count ;
		len -= readcount ;
		} ;
	return total ;
} /* paf24_read_f */

static int 
paf24_read_d (SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	PAF24_PRIVATE 	*ppaf24 ; 
	int				*iptr ;
	int				k, bufferlen, readcount = 0, count ;
	int				index = 0, total = 0 ;

	if (! psf->fdata)
		return 0 ;
	ppaf24 = (PAF24_PRIVATE*) psf->fdata ;
	
	iptr = (int*) psf->buffer ;
	bufferlen = psf->sf.channels * ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (int) ;
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : len ;
		count = paf24_read (psf, ppaf24, iptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [index+k] = (double) (iptr [k]) ;
		index += readcount ;
		total += count ;
		len -= readcount ;
		} ;
	return total ;
} /* paf24_read_d */

/*---------------------------------------------------------------------------
*/

static int 
paf24_writer_init (SF_PRIVATE  *psf)
{	PAF24_PRIVATE	*ppaf24 ; 
	unsigned int	paf24size ;	
	
	paf24size = sizeof (PAF24_PRIVATE) + psf->sf.channels * (32 + 10 * sizeof (int)) ;
	if (! (psf->fdata = malloc (paf24size)))
		return SFE_MALLOC_FAILED ;
		
	ppaf24 = (PAF24_PRIVATE*) psf->fdata ;
	memset (ppaf24, 0, paf24size) ;
	
	ppaf24->channels = psf->sf.channels ;
	ppaf24->block    = (unsigned char*) ppaf24->data ;
	ppaf24->samples  = (int*) (ppaf24->data + 32 * ppaf24->channels) ;
	
	ppaf24->blocksize = 32 * ppaf24->channels ;
	ppaf24->samplesperblock = 10 ;
	
	if (psf->datalength % ppaf24->blocksize)
	{	psf_log_printf (psf, "*** Warning : file seems to be truncated.\n") ;
		ppaf24->blocks = psf->datalength / ppaf24->blocksize  + 1 ;
		}
	else
		ppaf24->blocks = psf->datalength / ppaf24->blocksize ;
		
	psf->sf.samples = ppaf24->samplesperblock * ppaf24->blocks ;

	return 0 ;
} /* paf24_writer_init */


static int	
paf24_write_block (SF_PRIVATE  *psf, PAF24_PRIVATE *ppaf24)
{	int				k, *iptr, nextsample, channel ;
	unsigned char	*cptr ;
	
	/* First pack block. */	

	for (k = 0 ; k < 10 * ppaf24->channels ; k++)
	{	channel = k % ppaf24->channels ;
		cptr = ppaf24->block + 32 * channel + 3 * (k / ppaf24->channels) ;
		nextsample = ppaf24->samples [k] ;
		cptr [0] = nextsample & 0xFF ;
		cptr [1] = (nextsample >> 8) & 0xFF ;
		cptr [2] = (nextsample >> 16) & 0xFF ;
		} ;
	
	/* Do endian swapping if necessary. */

	iptr = (int*) (ppaf24->data) ;
	if ((CPU_IS_BIG_ENDIAN && psf->endian == SF_ENDIAN_LITTLE) ||
			(CPU_IS_LITTLE_ENDIAN && psf->endian == SF_ENDIAN_BIG))
	{	for (k = 0 ; k < 8 * ppaf24->channels ; k++)
			iptr [k] = ENDSWAP_INT (iptr [k]) ;
		} ;
		
	/* Write block to disk. */
	
	if ((k = fwrite (ppaf24->block, 1, ppaf24->blocksize, psf->file)) != ppaf24->blocksize)
		psf_log_printf (psf, "*** Warning : short write (%d != %d).\n", k, ppaf24->blocksize) ;

	ppaf24->blockcount ++ ;
	ppaf24->samplecount = 0 ;
	
	return 1 ;
} /* paf24_write_block */

static int 
paf24_write (SF_PRIVATE *psf, PAF24_PRIVATE *ppaf24, int *ptr, int len)
{	int		count, total = 0, index = 0 ;
	
	while (index < len)
	{	count = (ppaf24->samplesperblock - ppaf24->samplecount) * ppaf24->channels ;

		if (count > len - index)
			count = len - index ;

		memcpy (&(ppaf24->samples [ppaf24->samplecount * ppaf24->channels]), &(ptr [index]), count * sizeof (int)) ;
		index += count ;
		ppaf24->samplecount += count / ppaf24->channels ;
		total = index ;

		if (ppaf24->samplecount >= ppaf24->samplesperblock)
			paf24_write_block (psf, ppaf24) ;	
		} ;

	return total ;		
} /* paf24_write */

static int 
paf24_write_s (SF_PRIVATE *psf, short *ptr, int len)
{	PAF24_PRIVATE 	*ppaf24 ; 
	int				*iptr ;
	int				k, bufferlen, writecount = 0, count ;
	int				index = 0, total = 0 ;

	if (! psf->fdata)
		return 0 ;
	ppaf24 = (PAF24_PRIVATE*) psf->fdata ;
	
	iptr = (int*) psf->buffer ;
	bufferlen = psf->sf.channels * ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (int) ;
	while (len > 0)
	{	writecount = (len >= bufferlen) ? bufferlen : len ;
		for (k = 0 ; k < writecount ; k++)
			iptr [k] = ((int) ptr [index+k]) * 256 ;
		count = paf24_write (psf, ppaf24, iptr, writecount) ;
		index += writecount ;
		total += count ;
		len -= writecount ;
		} ;
	return total ;
} /* paf24_write_s */

static int 
paf24_write_i (SF_PRIVATE *psf, int *ptr, int len)
{	PAF24_PRIVATE 	*ppaf24 ; 
	int				total = 0 ;

	if (! psf->fdata)
		return 0 ;
	ppaf24 = (PAF24_PRIVATE*) psf->fdata ;
	
	total = paf24_write (psf, ppaf24, ptr, len) ;

	return total ;
} /* paf24_write_i */

static int 
paf24_write_f (SF_PRIVATE *psf, float *ptr, int len) 
{	PAF24_PRIVATE 	*ppaf24 ; 
	int				*iptr ;
	int				k, bufferlen, writecount = 0, count ;
	int				index = 0, total = 0 ;

	if (! psf->fdata)
		return 0 ;
	ppaf24 = (PAF24_PRIVATE*) psf->fdata ;
	
	iptr = (int*) psf->buffer ;
	bufferlen = psf->sf.channels * ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (int) ;
	while (len > 0)
	{	writecount = (len >= bufferlen) ? bufferlen : len ;
		for (k = 0 ; k < writecount ; k++)
			iptr [k] = FLOAT_TO_INT (ptr [index+k]) ;
		count = paf24_write (psf, ppaf24, iptr, writecount) ;
		index += writecount ;
		total += count ;
		len -= writecount ;
		} ;
	return total ;
} /* paf24_write_f */

static int 
paf24_write_d (SF_PRIVATE *psf, double *ptr, int len, int normalize) 
{	PAF24_PRIVATE 	*ppaf24 ; 
	int				*iptr ;
	int				k, bufferlen, writecount = 0, count ;
	int				index = 0, total = 0 ;

	if (! psf->fdata)
		return 0 ;
	ppaf24 = (PAF24_PRIVATE*) psf->fdata ;
	
	iptr = (int*) psf->buffer ;
	bufferlen = psf->sf.channels * ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (int) ;
	while (len > 0)
	{	writecount = (len >= bufferlen) ? bufferlen : len ;
		for (k = 0 ; k < writecount ; k++)
			iptr [k] = DOUBLE_TO_INT (ptr [index+k]) ;
		count = paf24_write (psf, ppaf24, iptr, writecount) ;
		index += writecount ;
		total += count ;
		len -= writecount ;
		} ;
	return total ;
} /* paf24_write_d */

/*---------------------------------------------------------------------------
*/

static long 
paf24_seek (SF_PRIVATE *psf, long offset, int whence)
{	PAF24_PRIVATE	*ppaf24 ; 
	int				newblock, newsample ;
	
	if (! psf->fdata)
		return 0 ;

	ppaf24 = (PAF24_PRIVATE*) psf->fdata ;

	if (! (psf->blockwidth && psf->datalength && psf->dataoffset))
	{	psf->error = SFE_BAD_SEEK ;
		return	((long) -1) ;
		} ;
		
	switch (whence)
	{	case SEEK_SET :
				if (offset < 0 || offset > ppaf24->blocks * ppaf24->samplesperblock)
				{	psf->error = SFE_BAD_SEEK ;
					return	((long) -1) ;
					} ;
				newblock  = offset / ppaf24->samplesperblock ;
				newsample = offset % ppaf24->samplesperblock ;
				break ;
				
		case SEEK_CUR :
				if (psf->current + offset < 0 || psf->current + offset > ppaf24->blocks * ppaf24->samplesperblock)
				{	psf->error = SFE_BAD_SEEK ;
					return	((long) -1) ;
					} ;
				newblock  = (psf->current + offset) / ppaf24->samplesperblock ;
				newsample = (psf->current + offset) % ppaf24->samplesperblock ;
				break ;
				
		case SEEK_END :
				if (offset > 0 || ppaf24->samplesperblock * ppaf24->blocks + offset < 0)
				{	psf->error = SFE_BAD_SEEK ;
					return	((long) -1) ;
					} ;
				newblock  = (ppaf24->samplesperblock * ppaf24->blocks + offset) / ppaf24->samplesperblock ;
				newsample = (ppaf24->samplesperblock * ppaf24->blocks + offset) % ppaf24->samplesperblock ;
				break ;
				
		default : 
				psf->error = SFE_BAD_SEEK ;
				return	((long) -1) ;
		} ;
		
	if (psf->mode == SF_MODE_READ)
	{	fseek (psf->file, (int) (psf->dataoffset + newblock * ppaf24->blocksize), SEEK_SET) ;
		ppaf24->blockcount  = newblock ;
		paf24_read_block (psf, ppaf24) ;
		ppaf24->samplecount = newsample ;
		}
	else
	{	/* What to do about write??? */ 
		psf->error = SFE_BAD_SEEK ;
		return	((long) -1) ;
		} ;

	psf->current = newblock * ppaf24->samplesperblock + newsample ;

	return psf->current ;
} /* paf24_seek */

/*---------------------------------------------------------------------------
*/

static int 
paf24_close (SF_PRIVATE  *psf)
{	PAF24_PRIVATE *ppaf24 ; 
	
	if (! psf->fdata)
		return 0 ;
		
	ppaf24 = (PAF24_PRIVATE*) psf->fdata ;
	
	if (psf->mode == SF_MODE_WRITE)
	{	if (ppaf24->samplecount && ppaf24->samplecount < ppaf24->samplesperblock)
			paf24_write_block (psf, ppaf24) ;	
		} ;
		
	free (psf->fdata) ;
	psf->fdata = NULL ;

	return 0 ;
} /* paf24_close */

/*---------------------------------------------------------------------------
*/

/*-
static void
endswap_paf_fmt (PAF_FMT *ppaf_fmt)
{	ppaf_fmt->version    = ENDSWAP_INT (ppaf_fmt->version) ;
	ppaf_fmt->endianness = ENDSWAP_INT (ppaf_fmt->endianness) ;
	ppaf_fmt->samplerate = ENDSWAP_INT (ppaf_fmt->samplerate) ;
	ppaf_fmt->format     = ENDSWAP_INT (ppaf_fmt->format) ;
	ppaf_fmt->channels   = ENDSWAP_INT (ppaf_fmt->channels) ;
	ppaf_fmt->source     = ENDSWAP_INT (ppaf_fmt->source) ;
} /+* endswap_paf_fmt *+/
-*/
