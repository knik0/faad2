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
#include	<string.h>
#include	<math.h>

#include	"sndfile.h"
#include	"config.h"
#include	"sfendian.h"
#include	"floatcast.h"
#include	"common.h"
#include	"wav.h"
#include	"GSM610/gsm.h"

#if (CPU_IS_LITTLE_ENDIAN == 1)
#	define	MAKE_MARKER(a,b,c,d)		((a)|((b)<<8)|((c)<<16)|((d)<<24))
#elif (CPU_IS_BIG_ENDIAN == 1)
#	define	MAKE_MARKER(a,b,c,d)		(((a)<<24)|((b)<<16)|((c)<<8)|(d))
#else
#	error "Cannot determine endian-ness of processor."
#endif

#define RIFF_MARKER	(MAKE_MARKER ('R', 'I', 'F', 'F')) 
#define WAVE_MARKER	(MAKE_MARKER ('W', 'A', 'V', 'E')) 
#define fmt_MARKER	(MAKE_MARKER ('f', 'm', 't', ' ')) 
#define fact_MARKER	(MAKE_MARKER ('f', 'a', 'c', 't')) 
#define data_MARKER	(MAKE_MARKER ('d', 'a', 't', 'a')) 

#define 	WAVE_FORMAT_GSM610	0x0031

#define		GSM610_BLOCKSIZE	65
#define		GSM610_SAMPLES		320

typedef struct
{	unsigned int	blocks ; 
	int				blockcount, samplecount ;
	unsigned char	block [GSM610_BLOCKSIZE] ;
	short			samples [GSM610_SAMPLES] ;
	gsm				gsm_data ;
} GSM610_PRIVATE ;


static	int	wav_gsm610_read_block (SF_PRIVATE *psf, GSM610_PRIVATE *pgsm610) ;
static	int wav_gsm610_read (SF_PRIVATE *psf, GSM610_PRIVATE *pgsm610, short *ptr, int len) ;

static	int	wav_gsm610_write_block (SF_PRIVATE *psf, GSM610_PRIVATE *pgsm610) ;
static	int wav_gsm610_write (SF_PRIVATE *psf, GSM610_PRIVATE *pgsm610, short *ptr, int len) ;

static long  wav_gsm610_seek   (SF_PRIVATE *psf, long offset, int whence) ;

static int	wav_gsm610_read_s (SF_PRIVATE *psf, short *ptr, int len) ;
static int	wav_gsm610_read_i (SF_PRIVATE *psf, int *ptr, int len) ;
static int	wav_gsm610_read_f (SF_PRIVATE *psf, float *ptr, int len) ;
static int	wav_gsm610_read_d (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;

static int	wav_gsm610_write_s (SF_PRIVATE *psf, short *ptr, int len) ;
static int	wav_gsm610_write_i (SF_PRIVATE *psf, int *ptr, int len) ;
static int	wav_gsm610_write_f (SF_PRIVATE *psf, float *ptr, int len) ;
static int	wav_gsm610_write_d (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;

static int	wav_gsm610_close	(SF_PRIVATE  *psf) ;
static int	wav_gsm610_write_header (SF_PRIVATE  *psf) ;

/*============================================================================================
** WAV GSM610 Reader initialisation function.
*/

int	
wav_gsm610_reader_init (SF_PRIVATE *psf, WAV_FMT *fmt)
{	GSM610_PRIVATE	*pgsm610 ;
	int  true = 1 ;
	
	psf->sf.seekable = SF_FALSE ;
	/*-psf->sf.seekable = SF_TRUE ;		-*/
	psf->seek_func = (func_seek) wav_gsm610_seek ;

	psf->read_short  = (func_short)  wav_gsm610_read_s ;
	psf->read_int    = (func_int)    wav_gsm610_read_i ;
	psf->read_float  = (func_float)  wav_gsm610_read_f ;
	psf->read_double = (func_double) wav_gsm610_read_d ;

	if (psf->mode != SF_MODE_READ)
		return SFE_BAD_MODE_RW ;

	if (! (pgsm610 = malloc (sizeof (GSM610_PRIVATE))))
		return SFE_MALLOC_FAILED ;

	psf->fdata = (void*) pgsm610 ;

	memset (pgsm610, 0, sizeof (GSM610_PRIVATE)) ;

	if (! (pgsm610->gsm_data = gsm_create ()))
		return SFE_MALLOC_FAILED ;
	gsm_option (pgsm610->gsm_data,  GSM_OPT_WAV49, &true) ;

	if (psf->datalength % GSM610_BLOCKSIZE)
	{	psf_log_printf (psf, "*** Warning : data chunk seems to be truncated.\n") ;
		pgsm610->blocks = psf->datalength / GSM610_BLOCKSIZE + 1 ;
		}
	else
		pgsm610->blocks = psf->datalength / GSM610_BLOCKSIZE ;
	
	psf->sf.samples = GSM610_SAMPLES * pgsm610->blocks ;
	
	wav_gsm610_read_block (psf, pgsm610) ;	/* Read first block. */
	
	return 0 ;	
} /* wav_gsm610_reader_init */

/*============================================================================================
** WAV GSM610 writer initialisation function.
*/

int	
wav_gsm610_writer_init (SF_PRIVATE *psf)
{	GSM610_PRIVATE	*pgsm610 ;
	int  true = 1, samplesperblock, bytespersec ;
	
	if (psf->mode != SF_MODE_WRITE)
		return SFE_BAD_MODE_RW ;

	if (! (pgsm610 = malloc (sizeof (GSM610_PRIVATE))))
		return SFE_MALLOC_FAILED ;
		
	psf->fdata = (void*) pgsm610 ;

	memset (pgsm610, 0, sizeof (GSM610_PRIVATE)) ;
	
	if (! (pgsm610->gsm_data = gsm_create ()))
		return SFE_MALLOC_FAILED ;
	gsm_option (pgsm610->gsm_data,  GSM_OPT_WAV49, &true) ;

	pgsm610->blockcount  = 0 ;
	pgsm610->samplecount = 0 ;
	
	samplesperblock = GSM610_SAMPLES ;
	bytespersec     = psf->sf.samplerate * GSM610_BLOCKSIZE / GSM610_SAMPLES ;
	
	if (bytespersec * GSM610_SAMPLES / GSM610_BLOCKSIZE < psf->sf.samplerate) 
		bytespersec ++ ;

	psf->datalength  = (psf->sf.samples / samplesperblock) * samplesperblock ;
	if (psf->sf.samples % samplesperblock)
		psf->datalength += samplesperblock ;

	wav_gsm610_write_header (psf) ;

	psf->write_short  = (func_short)   wav_gsm610_write_s ;
	psf->write_int    = (func_int)     wav_gsm610_write_i ;
	psf->write_float  = (func_float)   wav_gsm610_write_f ;
	psf->write_double = (func_double)  wav_gsm610_write_d ;
	
	psf->seek_func    = NULL ; /* Not seekable */
	psf->close        = (func_close)   wav_gsm610_close ;
	psf->write_header = (func_wr_hdr)  wav_gsm610_write_header ;
					
	return 0 ;
} /* wav_gsm610_writer_init */

/*============================================================================================
** GSM 6.10 Read Functions.
*/

static int		
wav_gsm610_read_block (SF_PRIVATE *psf, GSM610_PRIVATE *pgsm610)
{	int	k ;
	
	pgsm610->blockcount ++ ;
	pgsm610->samplecount = 0 ;
	
	if (pgsm610->blockcount > pgsm610->blocks)
	{	memset (pgsm610->samples, 0, GSM610_SAMPLES * sizeof (short)) ;
		return 1 ;
		} ;

	if ((k = fread (pgsm610->block, 1, GSM610_BLOCKSIZE, psf->file)) != GSM610_BLOCKSIZE)
		psf_log_printf (psf, "*** Warning : short read (%d != %d).\n", k, GSM610_BLOCKSIZE) ;

	if (gsm_decode (pgsm610->gsm_data, pgsm610->block, pgsm610->samples) < 0)
	{	psf_log_printf (psf, "Error from gsm_decode() on frame : %d\n", pgsm610->blockcount) ;
		return 0 ;
		} ;
			
	if (gsm_decode (pgsm610->gsm_data, pgsm610->block+(GSM610_BLOCKSIZE+1)/2, pgsm610->samples+GSM610_SAMPLES/2) < 0)
	{	psf_log_printf (psf, "Error from gsm_decode() on frame : %d.5\n", pgsm610->blockcount) ;
		return 0 ;
		} ;

	return 1 ;
} /* wav_gsm610_read_block */

static int 
wav_gsm610_read (SF_PRIVATE *psf, GSM610_PRIVATE *pgsm610, short *ptr, int len)
{	int		count, total = 0, index = 0 ;

	while (index < len)
	{	if (pgsm610->blockcount >= pgsm610->blocks && pgsm610->samplecount >= GSM610_SAMPLES)
		{	memset (&(ptr[index]), 0, (len - index) * sizeof (short)) ;
			return total ;
			} ;
		
		if (pgsm610->samplecount >= GSM610_SAMPLES)
			wav_gsm610_read_block (psf, pgsm610) ;
		
		count = GSM610_SAMPLES - pgsm610->samplecount ;
		count = (len - index > count) ? count : len - index ;
		
		memcpy (&(ptr[index]), &(pgsm610->samples [pgsm610->samplecount]), count * sizeof (short)) ;
		index += count ;
		pgsm610->samplecount += count ;
		total = index ;
		} ;

	return total ;		
} /* wav_gsm610_read */

static int		
wav_gsm610_read_s (SF_PRIVATE *psf, short *ptr, int len)
{	GSM610_PRIVATE 	*pgsm610 ; 
	int				total ;

	if (! psf->fdata)
		return 0 ;
	pgsm610 = (GSM610_PRIVATE*) psf->fdata ;
	
	total = wav_gsm610_read (psf, pgsm610, ptr, len) ;

	return total ;
} /* wav_gsm610_read_s */

static int		
wav_gsm610_read_i  (SF_PRIVATE *psf, int *ptr, int len)
{	GSM610_PRIVATE *pgsm610 ; 
	short		*sptr ;
	int			k, bufferlen, readcount = 0, count ;
	int			index = 0, total = 0 ;

	if (! psf->fdata)
		return 0 ;
	pgsm610 = (GSM610_PRIVATE*) psf->fdata ;
	
	sptr = (short*) psf->buffer ;
	bufferlen = ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (short) ;
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : len ;
		count = wav_gsm610_read (psf, pgsm610, sptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [index+k] = (int) (sptr [k]) ;
		index += readcount ;
		total += count ;
		len -= readcount ;
		} ;
	return total ;
} /* wav_gsm610_read_i */

static int		
wav_gsm610_read_f  (SF_PRIVATE *psf, float *ptr, int len)
{	GSM610_PRIVATE *pgsm610 ; 
	short		*sptr ;
	int			k, bufferlen, readcount = 0, count ;
	int			index = 0, total = 0 ;
	float		normfact ;

	if (! psf->fdata)
		return 0 ;
	pgsm610 = (GSM610_PRIVATE*) psf->fdata ;
	
	normfact = (psf->norm_float == SF_TRUE) ? 1.0 / ((float) 0x8000) : 1.0 ;

	sptr = (short*) psf->buffer ;
	bufferlen = ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (short) ;
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : len ;
		count = wav_gsm610_read (psf, pgsm610, sptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [index+k] = normfact * (sptr [k]) ;
		index += readcount ;
		total += count ;
		len -= readcount ;
		} ;
	return total ;
} /* wav_gsm610_read_d */

static int		
wav_gsm610_read_d  (SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	GSM610_PRIVATE *pgsm610 ; 
	short		*sptr ;
	int			k, bufferlen, readcount = 0, count ;
	int			index = 0, total = 0 ;
	double		normfact ;
	
	normfact = (normalize ? 1.0 / ((double) 0x8000) : 1.0) ;

	if (! psf->fdata)
		return 0 ;
	pgsm610 = (GSM610_PRIVATE*) psf->fdata ;
	
	sptr = (short*) psf->buffer ;
	bufferlen = ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (short) ;
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : len ;
		count = wav_gsm610_read (psf, pgsm610, sptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [index+k] = normfact * (double) (sptr [k]) ;
		index += readcount ;
		total += count ;
		len -= readcount ;
		} ;
	return total ;
} /* wav_gsm610_read_d */

static long    
wav_gsm610_seek   (SF_PRIVATE *psf, long offset, int whence)
{	GSM610_PRIVATE *pgsm610 ; 
	int			newblock, newsample ;
	
	if (! psf->fdata)
		return 0 ;
	pgsm610 = (GSM610_PRIVATE*) psf->fdata ;

	if (! (psf->blockwidth && psf->datalength && psf->dataoffset))
	{	psf->error = SFE_BAD_SEEK ;
		return	((long) -1) ;
		} ;
		
	switch (whence)
	{	case SEEK_SET :
				if (offset < 0 || offset > pgsm610->blocks * GSM610_SAMPLES)
				{	psf->error = SFE_BAD_SEEK ;
					return	((long) -1) ;
					} ;
				newblock  = offset / GSM610_SAMPLES ;
				newsample = offset % GSM610_SAMPLES ;
				break ;
				
		case SEEK_CUR :
				if (psf->current + offset < 0 || psf->current + offset > pgsm610->blocks * GSM610_SAMPLES)
				{	psf->error = SFE_BAD_SEEK ;
					return	((long) -1) ;
					} ;
				newblock  = (psf->current + offset) / GSM610_SAMPLES ;
				newsample = (psf->current + offset) % GSM610_SAMPLES ;
				break ;
				
		case SEEK_END :
				if (offset > 0 || GSM610_SAMPLES * pgsm610->blocks + offset < 0)
				{	psf->error = SFE_BAD_SEEK ;
					return	((long) -1) ;
					} ;
				newblock  = (GSM610_SAMPLES * pgsm610->blocks + offset) / GSM610_SAMPLES ;
				newsample = (GSM610_SAMPLES * pgsm610->blocks + offset) % GSM610_SAMPLES ;
				break ;
				
		default : 
				psf->error = SFE_BAD_SEEK ;
				return	((long) -1) ;
		} ;
		
	if (psf->mode == SF_MODE_READ)
	{	fseek (psf->file, (int) (psf->dataoffset + newblock * GSM610_BLOCKSIZE), SEEK_SET) ;
		pgsm610->blockcount  = newblock ;
		wav_gsm610_read_block (psf, pgsm610) ;
		pgsm610->samplecount = newsample ;
		}
	else
	{	/* What to do about write??? */ 
		psf->error = SFE_BAD_SEEK ;
		return	((long) -1) ;
		} ;

	psf->current = newblock * GSM610_SAMPLES + newsample ;
	return psf->current ;
} /* wav_gsm610_seek */

/*==========================================================================================
** GSM 6.10 Write Functions.
*/



/*==========================================================================================
*/

static int
wav_gsm610_write_block (SF_PRIVATE *psf, GSM610_PRIVATE *pgsm610)
{	int k ;

	/* Encode the samples. */
	gsm_encode (pgsm610->gsm_data, pgsm610->samples, pgsm610->block) ;
	gsm_encode (pgsm610->gsm_data, pgsm610->samples+GSM610_SAMPLES/2, pgsm610->block+GSM610_BLOCKSIZE/2) ;

	/* Write the block to disk. */
	if ((k = fwrite (pgsm610->block, 1, GSM610_BLOCKSIZE, psf->file)) != GSM610_BLOCKSIZE)
		psf_log_printf (psf, "*** Warning : short write (%d != %d).\n", k, GSM610_BLOCKSIZE) ;

	pgsm610->samplecount = 0 ;
	pgsm610->blockcount ++ ;

	/* Set samples to zero for next block. */
	memset (pgsm610->samples, 0, GSM610_SAMPLES * sizeof (short)) ;

	return 1 ;
} /* wav_gsm610_write_block */

static int 
wav_gsm610_write (SF_PRIVATE *psf, GSM610_PRIVATE *pgsm610, short *ptr, int len)
{	int		count, total = 0, index = 0 ;
	
	while (index < len)
	{	count = GSM610_SAMPLES - pgsm610->samplecount ;

		if (count > len - index)
			count = len - index ;

		memcpy (&(pgsm610->samples [pgsm610->samplecount]), &(ptr [index]), count * sizeof (short)) ;
		index += count ;
		pgsm610->samplecount += count ;
		total = index ;

		if (pgsm610->samplecount >= GSM610_SAMPLES)
			wav_gsm610_write_block (psf, pgsm610) ;	
		} ;

	return total ;		
} /* wav_gsm610_write */

static int		
wav_gsm610_write_s (SF_PRIVATE *psf, short *ptr, int len)
{	GSM610_PRIVATE 	*pgsm610 ; 
	int				total ;

	if (! psf->fdata)
		return 0 ;
	pgsm610 = (GSM610_PRIVATE*) psf->fdata ;
	
	total = wav_gsm610_write (psf, pgsm610, ptr, len) ;

	return total ;
} /* wav_gsm610_write_s */

static int		
wav_gsm610_write_i  (SF_PRIVATE *psf, int *ptr, int len)
{	GSM610_PRIVATE *pgsm610 ; 
	short		*sptr ;
	int			k, bufferlen, writecount = 0, count ;
	int			index = 0, total = 0 ;

	if (! psf->fdata)
		return 0 ;
	pgsm610 = (GSM610_PRIVATE*) psf->fdata ;
	
	sptr = (short*) psf->buffer ;
	bufferlen = ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (short) ;
	while (len > 0)
	{	writecount = (len >= bufferlen) ? bufferlen : len ;
		for (k = 0 ; k < writecount ; k++)
			sptr [k] = (short) ptr [index+k] ;
		count = wav_gsm610_write (psf, pgsm610, sptr, writecount) ;
		index += writecount ;
		total += count ;
		len -= writecount ;
		} ;
	return total ;
} /* wav_gsm610_write_i */

static int		
wav_gsm610_write_f  (SF_PRIVATE *psf, float *ptr, int len)
{	GSM610_PRIVATE *pgsm610 ; 
	short		*sptr ;
	int			k, bufferlen, writecount = 0, count ;
	int			index = 0, total = 0 ;
	float		normfact ;
	
	if (! psf->fdata)
		return 0 ;
	pgsm610 = (GSM610_PRIVATE*) psf->fdata ;
	
	normfact = (psf->norm_float == SF_TRUE) ? ((float) 0x8000) : 1.0 ;
	
	sptr = (short*) psf->buffer ;
	bufferlen = ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (short) ;
	while (len > 0)
	{	writecount = (len >= bufferlen) ? bufferlen : len ;
		for (k = 0 ; k < writecount ; k++)
			sptr [k] = FLOAT_TO_SHORT (normfact * ptr [index+k])  ;
		count = wav_gsm610_write (psf, pgsm610, sptr, writecount) ;
		index += writecount ;
		total += count ;
		len -= writecount ;
		} ;
	return total ;
} /* wav_gsm610_write_f */

static int		
wav_gsm610_write_d  (SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	GSM610_PRIVATE *pgsm610 ; 
	short		*sptr ;
	int			k, bufferlen, writecount = 0, count ;
	int			index = 0, total = 0 ;
	double		normfact ;
	
	normfact = (normalize ? (double) 0x8000 : 1.0) ;

	if (! psf->fdata)
		return 0 ;
	pgsm610 = (GSM610_PRIVATE*) psf->fdata ;
	
	sptr = (short*) psf->buffer ;
	bufferlen = ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (short) ;
	while (len > 0)
	{	writecount = (len >= bufferlen) ? bufferlen : len ;
		for (k = 0 ; k < writecount ; k++)
			sptr [k] = DOUBLE_TO_SHORT (normfact * ptr [index+k])  ;
		count = wav_gsm610_write (psf, pgsm610, sptr, writecount) ;
		index += writecount ;
		total += count ;
		len -= writecount ;
		} ;
	return total ;
} /* wav_gsm610_write_d */

static int
wav_gsm610_write_header (SF_PRIVATE  *psf)
{	int  fmt_size, blockalign, samplesperblock, bytespersec ;

	blockalign      = GSM610_BLOCKSIZE ;
	samplesperblock = GSM610_SAMPLES ;
	bytespersec     = (psf->sf.samplerate * blockalign) / samplesperblock ;

	/* Reset the current header length to zero. */
	psf->header [0] = 0 ;
	psf->headindex = 0 ;
	fseek (psf->file, 0, SEEK_SET) ;

	/* RIFF marker, length, WAVE and 'fmt ' markers. */		
	psf_binheader_writef (psf, "mlmm", RIFF_MARKER, psf->filelength - 8, WAVE_MARKER, fmt_MARKER) ;

	/* fmt chunk. */
	fmt_size = 2 + 2 + 4 + 4 + 2 + 2 + 2 + 2 ;
	
	/* fmt : size, WAV format type, channels. */
	psf_binheader_writef (psf, "lww", fmt_size, WAVE_FORMAT_GSM610, psf->sf.channels) ;

	/* fmt : samplerate, bytespersec. */
	psf_binheader_writef (psf, "ll", psf->sf.samplerate, bytespersec) ;

	/* fmt : blockalign, bitwidth, extrabytes, samplesperblock. */
	psf_binheader_writef (psf, "wwww", blockalign, 0, 2, samplesperblock) ;

	/* Fact chunk : marker, chunk size, samples */	
	psf_binheader_writef (psf, "mll", fact_MARKER, sizeof (int), psf->sf.samples) ;

	/* DATA chunk : marker, datalength. */
	psf_binheader_writef (psf, "ml", data_MARKER, psf->datalength) ;

	fwrite (psf->header, psf->headindex, 1, psf->file) ;

	psf->dataoffset = psf->headindex ;

	psf->datalength  = (psf->sf.samples / samplesperblock) * samplesperblock ;
	if (psf->sf.samples % samplesperblock)
		psf->datalength += samplesperblock ;

	return 0 ;
} /* wav_gsm610_write_header */

int	
wav_gsm610_close	(SF_PRIVATE  *psf)
{	GSM610_PRIVATE *pgsm610 ; 

	if (! psf->fdata)
		return 0 ;

	pgsm610 = (GSM610_PRIVATE*) psf->fdata ;

	if (psf->mode == SF_MODE_WRITE)
	{	/*	If a block has been partially assembled, write it out
		**	as the final block.
		*/
	
		if (pgsm610->samplecount && pgsm610->samplecount < GSM610_SAMPLES)
			wav_gsm610_write_block (psf, pgsm610) ;	

		/*  Now we know for certain the length of the file we can
		**  re-write correct values for the RIFF and data chunks.
		*/
		 
		fseek (psf->file, 0, SEEK_END) ;
		psf->filelength = ftell (psf->file) ;

		psf->sf.samples = GSM610_SAMPLES * pgsm610->blockcount ;
		psf->datalength = psf->filelength - psf->dataoffset ;

		wav_gsm610_write_header (psf) ;
		} ;

	if (pgsm610->gsm_data)
		gsm_destroy (pgsm610->gsm_data) ;

	if (psf->fdata)
		free (psf->fdata) ;
	psf->fdata = NULL ;

	return 0 ;
} /* wav_gsm610_close */

