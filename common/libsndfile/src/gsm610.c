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
#include	<string.h>

#include	"sndfile.h"
#include	"config.h"
#include	"sfendian.h"
#include	"float_cast.h"
#include	"common.h"
#include	"wav_w64.h"
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

typedef struct
{	int				blocks ; 
	int				blockcount, samplecount ;
	short			samples [WAV_W64_GSM610_SAMPLES] ;
	unsigned char	block [WAV_W64_GSM610_BLOCKSIZE] ;
	gsm				gsm_data ;
} GSM610_PRIVATE ;


static sf_count_t	gsm610_read_s (SF_PRIVATE *psf, short *ptr, sf_count_t len) ;
static sf_count_t	gsm610_read_i (SF_PRIVATE *psf, int *ptr, sf_count_t len) ;
static sf_count_t	gsm610_read_f (SF_PRIVATE *psf, float *ptr, sf_count_t len) ;
static sf_count_t	gsm610_read_d (SF_PRIVATE *psf, double *ptr, sf_count_t len) ;

static sf_count_t	gsm610_write_s (SF_PRIVATE *psf, short *ptr, sf_count_t len) ;
static sf_count_t	gsm610_write_i (SF_PRIVATE *psf, int *ptr, sf_count_t len) ;
static sf_count_t	gsm610_write_f (SF_PRIVATE *psf, float *ptr, sf_count_t len) ;
static sf_count_t	gsm610_write_d (SF_PRIVATE *psf, double *ptr, sf_count_t len) ;

static	int gsm610_read_block (SF_PRIVATE *psf, GSM610_PRIVATE *pgsm610, short *ptr, sf_count_t len) ;
static	int gsm610_write_block (SF_PRIVATE *psf, GSM610_PRIVATE *pgsm610, short *ptr, sf_count_t len) ;

static	int	gsm610_decode_block (SF_PRIVATE *psf, GSM610_PRIVATE *pgsm610) ;
static	int	gsm610_encode_block (SF_PRIVATE *psf, GSM610_PRIVATE *pgsm610) ;

static sf_count_t  gsm610_seek   (SF_PRIVATE *psf, int mode, sf_count_t offset) ;

static int	gsm610_close	(SF_PRIVATE  *psf) ;

/*============================================================================================
** WAV GSM610 initialisation function.
*/

int	
gsm610_init (SF_PRIVATE *psf)
{	GSM610_PRIVATE	*pgsm610 ;
	int  true = 1 ;
	
	if (psf->mode == SFM_RDWR)
		return SFE_BAD_MODE_RW ;

	psf->sf.seekable = SF_FALSE ;

	if (! (pgsm610 = malloc (sizeof (GSM610_PRIVATE))))
		return SFE_MALLOC_FAILED ;

	psf->fdata = (void*) pgsm610 ;

	memset (pgsm610, 0, sizeof (GSM610_PRIVATE)) ;

/*============================================================

Need separate gsm_data structs for encode and decode.

============================================================*/

	if (! (pgsm610->gsm_data = gsm_create ()))
		return SFE_MALLOC_FAILED ;
		
	if ((psf->sf.format & SF_FORMAT_TYPEMASK) == SF_FORMAT_WAV ||
				(psf->sf.format & SF_FORMAT_TYPEMASK) == SF_FORMAT_W64)
		gsm_option (pgsm610->gsm_data,  GSM_OPT_WAV49, &true) ;

	if (psf->mode == SFM_READ)
	{	if (psf->datalength % WAV_W64_GSM610_BLOCKSIZE)
		{	psf_log_printf (psf, "*** Warning : data chunk seems to be truncated.\n") ;
			pgsm610->blocks = psf->datalength / WAV_W64_GSM610_BLOCKSIZE + 1 ;
			}
		else
			pgsm610->blocks = psf->datalength / WAV_W64_GSM610_BLOCKSIZE ;
	
		psf->sf.samples = WAV_W64_GSM610_SAMPLES * pgsm610->blocks ;

		gsm610_decode_block (psf, pgsm610) ;	/* Read first block. */
		
		psf->read_short  = gsm610_read_s ;
		psf->read_int    = gsm610_read_i ;
		psf->read_float  = gsm610_read_f ;
		psf->read_double = gsm610_read_d ;
		} ;
		
	if (psf->mode == SFM_WRITE)
	{	pgsm610->blockcount  = 0 ;
		pgsm610->samplecount = 0 ;

		psf->write_short  = gsm610_write_s ;
		psf->write_int    = gsm610_write_i ;
		psf->write_float  = gsm610_write_f ;
		psf->write_double = gsm610_write_d ;
		} ;
		
	psf->close    = gsm610_close ;
	psf->new_seek = gsm610_seek ;

	psf->filelength = psf_get_filelen (psf->filedes) ;
	psf->datalength = psf->filelength - psf->dataoffset ;

	return 0 ;	
} /* gsm610_init */

/*============================================================================================
** GSM 6.10 Read Functions.
*/

static int		
gsm610_decode_block (SF_PRIVATE *psf, GSM610_PRIVATE *pgsm610)
{	int	k ;
	
	pgsm610->blockcount ++ ;
	pgsm610->samplecount = 0 ;
	
	if (pgsm610->blockcount > pgsm610->blocks)
	{	memset (pgsm610->samples, 0, WAV_W64_GSM610_SAMPLES * sizeof (short)) ;
		return 1 ;
		} ;

	if ((k = psf_fread (pgsm610->block, 1, WAV_W64_GSM610_BLOCKSIZE, psf->filedes)) != WAV_W64_GSM610_BLOCKSIZE)
		psf_log_printf (psf, "*** Warning : short read (%d != %d).\n", k, WAV_W64_GSM610_BLOCKSIZE) ;

	if (gsm_decode (pgsm610->gsm_data, pgsm610->block, pgsm610->samples) < 0)
	{	psf_log_printf (psf, "Error from gsm_decode() on frame : %d\n", pgsm610->blockcount) ;
		return 0 ;
		} ;
			
	if (gsm_decode (pgsm610->gsm_data, pgsm610->block+(WAV_W64_GSM610_BLOCKSIZE+1)/2, pgsm610->samples+WAV_W64_GSM610_SAMPLES/2) < 0)
	{	psf_log_printf (psf, "Error from gsm_decode() on frame : %d.5\n", pgsm610->blockcount) ;
		return 0 ;
		} ;

	return 1 ;
} /* gsm610_decode_block */

static int 
gsm610_read_block (SF_PRIVATE *psf, GSM610_PRIVATE *pgsm610, short *ptr, sf_count_t len)
{	sf_count_t	count, total = 0, index = 0 ;

	while (index < len)
	{	if (pgsm610->blockcount >= pgsm610->blocks && pgsm610->samplecount >= WAV_W64_GSM610_SAMPLES)
		{	memset (&(ptr[index]), 0, (size_t) ((len - index) * sizeof (short))) ;
			return total ;
			} ;
		
		if (pgsm610->samplecount >= WAV_W64_GSM610_SAMPLES)
			gsm610_decode_block (psf, pgsm610) ;
		
		count = WAV_W64_GSM610_SAMPLES - pgsm610->samplecount ;
		count = (len - index > count) ? count : len - index ;
		
		memcpy (&(ptr[index]), &(pgsm610->samples [pgsm610->samplecount]), count * sizeof (short)) ;
		index += count ;
		pgsm610->samplecount += count ;
		total = index ;
		} ;

	return total ;		
} /* gsm610_read_block */

static sf_count_t
gsm610_read_s (SF_PRIVATE *psf, short *ptr, sf_count_t len)
{	GSM610_PRIVATE 	*pgsm610 ; 
	int				total ;

	if (! psf->fdata)
		return 0 ;
	pgsm610 = (GSM610_PRIVATE*) psf->fdata ;
	
	total = gsm610_read_block (psf, pgsm610, ptr, len) ;

	return total ;
} /* gsm610_read_s */

static sf_count_t	
gsm610_read_i  (SF_PRIVATE *psf, int *ptr, sf_count_t len)
{	GSM610_PRIVATE *pgsm610 ; 
	short		*sptr ;
	int			k, bufferlen, readcount = 0, count ;
	int			index = 0, total = 0 ;

	if (! psf->fdata)
		return 0 ;
	pgsm610 = (GSM610_PRIVATE*) psf->fdata ;
	
	sptr = (short*) psf->buffer ;
	bufferlen = SF_BUFFER_LEN / sizeof (short) ;
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : len ;
		count = gsm610_read_block (psf, pgsm610, sptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [index + k] = sptr [k] << 16 ;
		index += readcount ;
		total += count ;
		len -= readcount ;
		} ;
	return total ;
} /* gsm610_read_i */

static sf_count_t	
gsm610_read_f  (SF_PRIVATE *psf, float *ptr, sf_count_t len)
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
	bufferlen = SF_BUFFER_LEN / sizeof (short) ;
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : len ;
		count = gsm610_read_block (psf, pgsm610, sptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [index + k] = normfact * sptr [k] ;
		index += readcount ;
		total += count ;
		len -= readcount ;
		} ;
	return total ;
} /* gsm610_read_f */

static sf_count_t	
gsm610_read_d  (SF_PRIVATE *psf, double *ptr, sf_count_t len)
{	GSM610_PRIVATE *pgsm610 ; 
	short		*sptr ;
	int			k, bufferlen, readcount = 0, count ;
	int			index = 0, total = 0 ;
	double		normfact ;
	
	normfact = (psf->norm_double == SF_TRUE) ? 1.0 / ((double) 0x8000) : 1.0 ;

	if (! psf->fdata)
		return 0 ;
	pgsm610 = (GSM610_PRIVATE*) psf->fdata ;
	
	sptr = (short*) psf->buffer ;
	bufferlen = SF_BUFFER_LEN / sizeof (short) ;
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : len ;
		count = gsm610_read_block (psf, pgsm610, sptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [index + k] = normfact * sptr [k] ;
		index += readcount ;
		total += count ;
		len -= readcount ;
		} ;
	return total ;
} /* gsm610_read_d */

static sf_count_t    
gsm610_seek   (SF_PRIVATE *psf, int mode, sf_count_t offset)
{	GSM610_PRIVATE *pgsm610 ; 
	int			newblock, newsample ;
	
	if (! psf->fdata)
		return 0 ;
	pgsm610 = (GSM610_PRIVATE*) psf->fdata ;

	if (psf->dataoffset < 0)
	{	psf->error = SFE_BAD_SEEK ;
		return	((sf_count_t) -1) ;
		} ;
		
	if (offset == 0)
	{	int true = 1 ;
	
		psf_fseek (psf->filedes, psf->dataoffset, SEEK_SET) ;
		pgsm610->blockcount  = 0 ;
		
		gsm_init (pgsm610->gsm_data) ;
		if ((psf->sf.format & SF_FORMAT_TYPEMASK) == SF_FORMAT_WAV ||
				(psf->sf.format & SF_FORMAT_TYPEMASK) == SF_FORMAT_W64)
			gsm_option (pgsm610->gsm_data,  GSM_OPT_WAV49, &true) ;

		gsm610_decode_block (psf, pgsm610) ;
		pgsm610->samplecount = 0 ;
		return 0 ;
		} ;

	if (offset < 0 || offset > pgsm610->blocks * WAV_W64_GSM610_SAMPLES)
	{	psf->error = SFE_BAD_SEEK ;
		return	((sf_count_t) -1) ;
		} ;

	newblock  = offset / WAV_W64_GSM610_SAMPLES ;
	newsample = offset % WAV_W64_GSM610_SAMPLES ;
		
	if (psf->mode == SFM_READ)
	{	if (psf->read_current != newblock * WAV_W64_GSM610_BLOCKSIZE + newsample)
		{	psf_fseek (psf->filedes, psf->dataoffset + newblock * WAV_W64_GSM610_BLOCKSIZE, SEEK_SET) ;
			pgsm610->blockcount  = newblock ;
			gsm610_decode_block (psf, pgsm610) ;
			pgsm610->samplecount = newsample ;
			} ;
		
		return newblock * WAV_W64_GSM610_SAMPLES + newsample ;
		} ;
		
	/* What to do about write??? */ 
	psf->error = SFE_BAD_SEEK ;
	return	((sf_count_t) -1) ;
} /* gsm610_seek */

/*==========================================================================================
** GSM 6.10 Write Functions.
*/



/*==========================================================================================
*/

static int
gsm610_encode_block (SF_PRIVATE *psf, GSM610_PRIVATE *pgsm610)
{	int k ;

	/* Encode the samples. */
	gsm_encode (pgsm610->gsm_data, pgsm610->samples, pgsm610->block) ;
	gsm_encode (pgsm610->gsm_data, pgsm610->samples+WAV_W64_GSM610_SAMPLES/2, pgsm610->block+WAV_W64_GSM610_BLOCKSIZE/2) ;

	/* Write the block to disk. */
	if ((k = psf_fwrite (pgsm610->block, 1, WAV_W64_GSM610_BLOCKSIZE, psf->filedes)) != WAV_W64_GSM610_BLOCKSIZE)
		psf_log_printf (psf, "*** Warning : short write (%d != %d).\n", k, WAV_W64_GSM610_BLOCKSIZE) ;

	pgsm610->samplecount = 0 ;
	pgsm610->blockcount ++ ;

	/* Set samples to zero for next block. */
	memset (pgsm610->samples, 0, WAV_W64_GSM610_SAMPLES * sizeof (short)) ;

	return 1 ;
} /* wav_gsm610_encode_block */

static int 
gsm610_write_block (SF_PRIVATE *psf, GSM610_PRIVATE *pgsm610, short *ptr, sf_count_t len)
{	int		count, total = 0, index = 0 ;
	
	while (index < len)
	{	count = WAV_W64_GSM610_SAMPLES - pgsm610->samplecount ;

		if (count > len - index)
			count = len - index ;

		memcpy (&(pgsm610->samples [pgsm610->samplecount]), &(ptr [index]), count * sizeof (short)) ;
		index += count ;
		pgsm610->samplecount += count ;
		total = index ;

		if (pgsm610->samplecount >= WAV_W64_GSM610_SAMPLES)
			gsm610_encode_block (psf, pgsm610) ;	
		} ;

	return total ;		
} /* gsm610_write_block */

static sf_count_t
gsm610_write_s (SF_PRIVATE *psf, short *ptr, sf_count_t len)
{	GSM610_PRIVATE 	*pgsm610 ; 
	int				total ;

	if (! psf->fdata)
		return 0 ;
	pgsm610 = (GSM610_PRIVATE*) psf->fdata ;
	
	total = gsm610_write_block (psf, pgsm610, ptr, len) ;

	return total ;
} /* gsm610_write_s */

static sf_count_t	
gsm610_write_i  (SF_PRIVATE *psf, int *ptr, sf_count_t len)
{	GSM610_PRIVATE *pgsm610 ; 
	short		*sptr ;
	int			k, bufferlen, writecount = 0, count ;
	int			index = 0, total = 0 ;

	if (! psf->fdata)
		return 0 ;
	pgsm610 = (GSM610_PRIVATE*) psf->fdata ;
	
	sptr = (short*) psf->buffer ;
	bufferlen = SF_BUFFER_LEN / sizeof (short) ;
	while (len > 0)
	{	writecount = (len >= bufferlen) ? bufferlen : len ;
		for (k = 0 ; k < writecount ; k++)
			sptr [k] = ptr [index + k] >> 16 ;
		count = gsm610_write_block (psf, pgsm610, sptr, writecount) ;
		index += writecount ;
		total += count ;
		len -= writecount ;
		} ;
	return total ;
} /* gsm610_write_i */

static sf_count_t	
gsm610_write_f  (SF_PRIVATE *psf, float *ptr, sf_count_t len)
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
	bufferlen = SF_BUFFER_LEN / sizeof (short) ;
	while (len > 0)
	{	writecount = (len >= bufferlen) ? bufferlen : len ;
		for (k = 0 ; k < writecount ; k++)
			sptr [k] = lrintf (normfact * ptr [index + k])  ;
		count = gsm610_write_block (psf, pgsm610, sptr, writecount) ;
		index += writecount ;
		total += count ;
		len -= writecount ;
		} ;
	return total ;
} /* gsm610_write_f */

static sf_count_t	
gsm610_write_d  (SF_PRIVATE *psf, double *ptr, sf_count_t len)
{	GSM610_PRIVATE *pgsm610 ; 
	short		*sptr ;
	int			k, bufferlen, writecount = 0, count ;
	int			index = 0, total = 0 ;
	double		normfact ;
	
	if (! psf->fdata)
		return 0 ;
	pgsm610 = (GSM610_PRIVATE*) psf->fdata ;
	
	normfact = (psf->norm_double == SF_TRUE) ? ((double) 0x8000) : 1.0 ;

	sptr = (short*) psf->buffer ;
	bufferlen = SF_BUFFER_LEN / sizeof (short) ;
	while (len > 0)
	{	writecount = (len >= bufferlen) ? bufferlen : len ;
		for (k = 0 ; k < writecount ; k++)
			sptr [k] = lrint (normfact * ptr [index + k]) ;
		count = gsm610_write_block (psf, pgsm610, sptr, writecount) ;
		index += writecount ;
		total += count ;
		len -= writecount ;
		} ;
	return total ;
} /* gsm610_write_d */

static int	
gsm610_close	(SF_PRIVATE  *psf)
{	GSM610_PRIVATE *pgsm610 ; 

	if (! psf->fdata)
		return 0 ;

	pgsm610 = (GSM610_PRIVATE*) psf->fdata ;

	if (psf->mode == SFM_WRITE)
	{	/*	If a block has been partially assembled, write it out
		**	as the final block.
		*/
	
		if (pgsm610->samplecount && pgsm610->samplecount < WAV_W64_GSM610_SAMPLES)
			gsm610_encode_block (psf, pgsm610) ;	

		/*  Now we know for certain the length of the file we can
		**  re-write correct values for the RIFF and data chunks.
		*/
		 
		psf_fseek (psf->filedes, 0, SEEK_END) ;
		psf->filelength = psf_ftell (psf->filedes) ;

		psf->sf.samples = WAV_W64_GSM610_SAMPLES * pgsm610->blockcount ;
		psf->datalength = psf->filelength - psf->dataoffset ;

		if (psf->write_header)
			psf->write_header (psf) ;
		} ;

	if (pgsm610->gsm_data)
		gsm_destroy (pgsm610->gsm_data) ;

	if (psf->fdata)
		free (psf->fdata) ;
	psf->fdata = NULL ;

	return 0 ;
} /* gsm610_close */

