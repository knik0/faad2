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

#define RIFF_MARKER	(MAKE_MARKER ('R', 'I', 'F', 'F')) 
#define WAVE_MARKER	(MAKE_MARKER ('W', 'A', 'V', 'E')) 
#define fmt_MARKER	(MAKE_MARKER ('f', 'm', 't', ' ')) 
#define fact_MARKER	(MAKE_MARKER ('f', 'a', 'c', 't')) 
#define data_MARKER	(MAKE_MARKER ('d', 'a', 't', 'a')) 

#define WAVE_FORMAT_IMA_ADPCM		0x0011		/* Intel Corporation */

typedef struct
{	unsigned int	channels ;
	unsigned int	blocksize ;
	unsigned int	samplesperblock ; 
} IMA_ADPCM_PUBLIC ;

typedef struct
{	unsigned int	channels, blocksize, samplesperblock, blocks ; 
	int				blockcount, samplecount ;
	int				previous [2] ;
	int				stepindex [2] ;
	unsigned char	*block ;
	short			*samples ;
	unsigned char	data	[4] ; /* Dummy size */
} IMA_ADPCM_PRIVATE ;

/*============================================================================================
** Predefined IMA ADPCM data.
*/

static int ima_index_adjust [16] = 
{	-1, -1, -1, -1,		/* +0 - +3, decrease the step size */
     2,  4,  6,  8,     /* +4 - +7, increase the step size */
    -1, -1, -1, -1,		/* -0 - -3, decrease the step size */
     2,  4,  6,  8,		/* -4 - -7, increase the step size */
} ;

static int ima_step_size [89] = 
{	7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 
	50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 
	253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 
	1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 
	3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
	11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 
	32767
} ;

static int	ima_reader_init (SF_PRIVATE *psf, IMA_ADPCM_PUBLIC *public) ;
static int	ima_writer_init (SF_PRIVATE *psf, IMA_ADPCM_PUBLIC *public) ;

static int ima_read_block (SF_PRIVATE *psf, IMA_ADPCM_PRIVATE *pima) ;
static int ima_read (SF_PRIVATE *psf, IMA_ADPCM_PRIVATE *pima, short *ptr, int len) ;

static int ima_write_block (SF_PRIVATE *psf, IMA_ADPCM_PRIVATE *pima) ;
static int ima_write (SF_PRIVATE *psf, IMA_ADPCM_PRIVATE *pima, short *ptr, int len) ;

static int ima_read_s (SF_PRIVATE *psf, short *ptr, int len) ;
static int ima_read_i (SF_PRIVATE *psf, int *ptr, int len) ;
static int ima_read_f (SF_PRIVATE *psf, float *ptr, int len) ;
static int ima_read_d (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;

static int ima_write_s (SF_PRIVATE *psf, short *ptr, int len) ;
static int ima_write_i (SF_PRIVATE *psf, int *ptr, int len) ;
static int ima_write_f (SF_PRIVATE *psf, float *ptr, int len) ;
static int ima_write_d (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;

static long	ima_seek   (SF_PRIVATE *psf, long offset, int whence) ;
static int		wav_ima_close			(SF_PRIVATE  *psf) ;
static int		wav_ima_write_header 	(SF_PRIVATE  *psf) ;

static	unsigned int wav_srate2blocksize (unsigned int srate) ;

/*============================================================================================
** IMA ADPCM Reader initialisation function.
*/

int	
wav_ima_reader_init (SF_PRIVATE *psf, WAV_FMT *fmt)
{	IMA_ADPCM_PUBLIC public ;

	public.blocksize       = fmt->ima.blockalign ;
	public.channels        = fmt->ima.channels ;
	public.samplesperblock = fmt->ima.samplesperblock ;
	return ima_reader_init (psf, &public) ;
} /* wav_ima_reader_init */

int	
wav_ima_writer_init (SF_PRIVATE *psf)
{	IMA_ADPCM_PUBLIC 	public ;
	int					error ;
	
	public.blocksize = wav_srate2blocksize (psf->sf.samplerate) ;	
	public.channels  = psf->sf.channels ;
	public.samplesperblock = 2 * (public.blocksize - 4 * psf->sf.channels) / psf->sf.channels + 1 ;
	
	if ((error = ima_writer_init (psf, &public)))
		return error ;
		
	wav_ima_write_header (psf) ;

	psf->write_short  = (func_short)  ima_write_s ;
	psf->write_int    = (func_int)    ima_write_i ;
	psf->write_float  = (func_float)  ima_write_f ;
	psf->write_double = (func_double) ima_write_d ;
	psf->seek_func    = (func_seek)   ima_seek ;
	psf->close        = (func_close)  wav_ima_close ;
	psf->write_header = (func_wr_hdr) wav_ima_write_header ;
		
	return 0 ;
} /* wav_ima_writer_init */

/*============================================================================================
** IMA ADPCM Read Functions.
*/

static int
ima_reader_init (SF_PRIVATE *psf, IMA_ADPCM_PUBLIC *public)
{	IMA_ADPCM_PRIVATE	*pima ;
	unsigned int		pimasize, count ;
	
	if (psf->mode != SF_MODE_READ)
		return SFE_BAD_MODE_RW ;

	pimasize = sizeof (IMA_ADPCM_PRIVATE) + public->blocksize + 3 * public->channels * public->samplesperblock ;

	if (! (pima = malloc (pimasize)))
		return SFE_MALLOC_FAILED ;

	psf->fdata = (void*) pima ;

	memset (pima, 0, pimasize) ;

	pima->block   = (unsigned char*) pima->data ;
	pima->samples = (short*) (pima->data + public->blocksize) ;
	
	pima->channels        = public->channels ;
	pima->blocksize       = public->blocksize ;
	pima->samplesperblock = public->samplesperblock ;
	
	if (psf->datalength % pima->blocksize)
	{	psf_log_printf (psf, "*** Warning : data chunk seems to be truncated.\n") ;
		pima->blocks = psf->datalength / pima->blocksize  + 1 ;
		}
	else
		pima->blocks = psf->datalength / pima->blocksize ;
	
	count = 2 * (pima->blocksize - 4 * pima->channels) / pima->channels + 1 ;
	if (pima->samplesperblock != count)
		psf_log_printf (psf, "*** Warning : samplesperblock should be %d.\n", count) ;

	psf->sf.samples = pima->samplesperblock * pima->blocks ;

	ima_read_block (psf, pima) ;	/* Read first block. */
	
	psf->read_short  = (func_short)  ima_read_s ;
	psf->read_int    = (func_int)    ima_read_i ;
	psf->read_float  = (func_float)  ima_read_f ;
	psf->read_double = (func_double) ima_read_d ;

	psf->seek_func = (func_seek) ima_seek ;

	return 0 ;	
} /* ima_reader_init */

static int
ima_read_block (SF_PRIVATE *psf, IMA_ADPCM_PRIVATE *pima)
{	int		chan, k, current, blockindex, index, indexstart ;
	short	step, diff, bytecode, stepindex [2] ;
	
	pima->blockcount ++ ;
	pima->samplecount = 0 ;
	
	if (pima->blockcount > pima->blocks)
	{	memset (pima->samples, 0, pima->samplesperblock * pima->channels * sizeof (short)) ;
		return 1 ;
		} ;

	if ((k = fread (pima->block, 1, pima->blocksize, psf->file)) != pima->blocksize)
		psf_log_printf (psf, "*** Warning : short read (%d != %d).\n", k, pima->blocksize) ;

	/* Read and check the block header. */
	
	for (chan = 0 ; chan < pima->channels ; chan++)
	{	current = pima->block [chan*4] | (pima->block [chan*4+1] << 8) ;
		if (current & 0x8000)
			current -= 0x10000 ;
			
		stepindex [chan] = pima->block [chan*4+2] ;
		if (stepindex [chan] < 0)
			stepindex [chan] = 0 ;
		else if (stepindex [chan] > 88)
			stepindex [chan] = 88 ;

		if (pima->block [chan*4+3] != 0)
			psf_log_printf (psf, "IMA ADPCM synchronisation error.\n") ;
		
		pima->samples [chan] = current ;

		/* psf_log_printf (psf, "block %d : channel %d (current, index) : (%d, %d)\n", 
		**	pima->blockcount, chan,  current, stepindex [chan]) ;
		*/
		} ;
		
	/* Pull apart the packed 4 bit samples and store them in their
	** correct sample positions.
	*/
	
	blockindex = 4 * pima->channels ;
	
	indexstart = pima->channels ;
	while (blockindex <  pima->blocksize)
	{	for (chan = 0 ; chan < pima->channels ; chan++)
		{	index = indexstart + chan ;
			for (k = 0 ; k < 4 ; k++)
			{	bytecode = pima->block [blockindex++] ;
				pima->samples [index] = bytecode & 0x0F ;
				index += pima->channels ;
				pima->samples [index] = (bytecode >> 4) & 0x0F ;
				index += pima->channels ;
				} ;
			} ;
		indexstart += 8 * pima->channels ;
		} ;
		
	/* Decode the encoded 4 bit samples. */
	
	for (k = pima->channels ; k < (pima->samplesperblock * pima->channels) ; k ++)
	{	chan = (pima->channels > 1) ? (k % 2) : 0 ;

		bytecode = pima->samples [k] & 0xF ;
		
		step = ima_step_size [stepindex [chan]] ;
		current = pima->samples [k - pima->channels] ;
  
		diff = step >> 3 ;
		if (bytecode & 1) 
			diff += step >> 2 ;
		if (bytecode & 2) 
			diff += step >> 1 ;
		if (bytecode & 4) 
			diff += step ;
		if (bytecode & 8) 
			diff = -diff ;

		current += diff ;

		if (current > 32767) 
			current = 32767;
		else if (current < -32768) 
			current = -32768 ;

		stepindex [chan] += ima_index_adjust [bytecode] ;
	
		if (stepindex [chan] < 0) 
			stepindex [chan] = 0 ;
		else if (stepindex [chan] > 88) 
			stepindex [chan] = 88 ;

		pima->samples [k] = current ;
		} ;

	return 1 ;
} /* ima_read_block */

static int 
ima_read (SF_PRIVATE *psf, IMA_ADPCM_PRIVATE *pima, short *ptr, int len)
{	int		count, total = 0, index = 0 ;

	while (index < len)
	{	if (pima->blockcount >= pima->blocks && pima->samplecount >= pima->samplesperblock)
		{	memset (&(ptr[index]), 0, (len - index) * sizeof (short)) ;
			return total ;
			} ;
		
		if (pima->samplecount >= pima->samplesperblock)
			ima_read_block (psf, pima) ;
		
		count = (pima->samplesperblock - pima->samplecount) * pima->channels ;
		count = (len - index > count) ? count : len - index ;
		
		memcpy (&(ptr[index]), &(pima->samples [pima->samplecount * pima->channels]), count * sizeof (short)) ;
		index += count ;
		pima->samplecount += count / pima->channels ;
		total = index ;
		} ;

	return total ;		
} /* ima_read */

static int		
ima_read_s (SF_PRIVATE *psf, short *ptr, int len)
{	IMA_ADPCM_PRIVATE 	*pima ; 
	int				total ;

	if (! psf->fdata)
		return 0 ;
	pima = (IMA_ADPCM_PRIVATE*) psf->fdata ;
	
	total = ima_read (psf, pima, ptr, len) ;

	return total ;
} /* ima_read_s */

static int		
ima_read_i  (SF_PRIVATE *psf, int *ptr, int len)
{	IMA_ADPCM_PRIVATE *pima ; 
	short		*sptr ;
	int			k, bufferlen, readcount = 0, count ;
	int			index = 0, total = 0 ;

	if (! psf->fdata)
		return 0 ;
	pima = (IMA_ADPCM_PRIVATE*) psf->fdata ;
	
	sptr = (short*) psf->buffer ;
	bufferlen = ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (short) ;
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : len ;
		count = ima_read (psf, pima, sptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [index+k] = (int) (sptr [k]) ;
		index += readcount ;
		total += count ;
		len -= readcount ;
		} ;
	return total ;
} /* ima_read_i */

static int
ima_read_f  (SF_PRIVATE *psf, float *ptr, int len)
{	IMA_ADPCM_PRIVATE *pima ; 
	short		*sptr ;
	int			k, bufferlen, readcount = 0, count ;
	int			index = 0, total = 0 ;
	float		normfact ;

	if (! psf->fdata)
		return 0 ;
	pima = (IMA_ADPCM_PRIVATE*) psf->fdata ;
	
	normfact = (psf->norm_float == SF_TRUE) ? 1.0 / ((float) 0x8000) : 1.0 ;

	sptr = (short*) psf->buffer ;
	bufferlen = ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (short) ;
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : len ;
		count = ima_read (psf, pima, sptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [index+k] = normfact * (float) (sptr [k]) ;
		index += readcount ;
		total += count ;
		len -= readcount ;
		} ;
	return total ;
} /* ima_read_f */

static int
ima_read_d  (SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	IMA_ADPCM_PRIVATE *pima ; 
	short		*sptr ;
	int			k, bufferlen, readcount = 0, count ;
	int			index = 0, total = 0 ;
	double 		normfact ;
	
	normfact = (normalize ? 1.0 / ((double) 0x8000) : 1.0) ;

	if (! psf->fdata)
		return 0 ;
	pima = (IMA_ADPCM_PRIVATE*) psf->fdata ;
	
	sptr = (short*) psf->buffer ;
	bufferlen = ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (short) ;
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : len ;
		count = ima_read (psf, pima, sptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [index+k] = normfact * (double) (sptr [k]) ;
		index += readcount ;
		total += count ;
		len -= readcount ;
		} ;
	return total ;
} /* ima_read_d */

static long    
ima_seek   (SF_PRIVATE *psf, long offset, int whence)
{	IMA_ADPCM_PRIVATE *pima ; 
	int			newblock, newsample ;
	
	if (! psf->fdata)
		return 0 ;
	pima = (IMA_ADPCM_PRIVATE*) psf->fdata ;

	if (! (psf->blockwidth && psf->datalength && psf->dataoffset))
	{	psf->error = SFE_BAD_SEEK ;
		return	((long) -1) ;
		} ;
		
	switch (whence)
	{	case SEEK_SET :
				if (offset < 0 || offset > pima->blocks * pima->samplesperblock)
				{	psf->error = SFE_BAD_SEEK ;
					return	((long) -1) ;
					} ;
				newblock  = offset / pima->samplesperblock ;
				newsample = offset % pima->samplesperblock ;
				break ;
				
		case SEEK_CUR :
				if (psf->current + offset < 0 || psf->current + offset > pima->blocks * pima->samplesperblock)
				{	psf->error = SFE_BAD_SEEK ;
					return	((long) -1) ;
					} ;
				newblock  = (psf->current + offset) / pima->samplesperblock ;
				newsample = (psf->current + offset) % pima->samplesperblock ;
				break ;
				
		case SEEK_END :
				if (offset > 0 || pima->samplesperblock * pima->blocks + offset < 0)
				{	psf->error = SFE_BAD_SEEK ;
					return	((long) -1) ;
					} ;
				newblock  = (pima->samplesperblock * pima->blocks + offset) / pima->samplesperblock ;
				newsample = (pima->samplesperblock * pima->blocks + offset) % pima->samplesperblock ;
				break ;
				
		default : 
				psf->error = SFE_BAD_SEEK ;
				return	((long) -1) ;
		} ;
		
	if (psf->mode == SF_MODE_READ)
	{	fseek (psf->file, (int) (psf->dataoffset + newblock * pima->blocksize), SEEK_SET) ;
		pima->blockcount  = newblock ;
		ima_read_block (psf, pima) ;
		pima->samplecount = newsample ;
		}
	else
	{	/* What to do about write??? */ 
		psf->error = SFE_BAD_SEEK ;
		return	((long) -1) ;
		} ;

	psf->current = newblock * pima->samplesperblock + newsample ;
	return psf->current ;
} /* ima_seek */

/*==========================================================================================
** IMA ADPCM Write Functions.
*/

static int	
ima_writer_init (SF_PRIVATE *psf, IMA_ADPCM_PUBLIC *public)
{	IMA_ADPCM_PRIVATE	*pima ;
	unsigned int 		pimasize ;
	
	if (psf->mode != SF_MODE_WRITE)
		return SFE_BAD_MODE_RW ;

	public->blocksize       = wav_srate2blocksize (psf->sf.samplerate) ;	
	public->samplesperblock = 2 * (public->blocksize - 4 * public->channels) / public->channels + 1 ;

	pimasize = sizeof (IMA_ADPCM_PRIVATE) + public->blocksize + 3 * public->channels * public->samplesperblock ;

	if (! (pima = malloc (pimasize)))
		return SFE_MALLOC_FAILED ;
		
	psf->fdata = (void*) pima ;

	memset (pima, 0, pimasize) ;
	
	pima->channels        = public->channels ;
	pima->blocksize       = public->blocksize ;
	pima->samplesperblock = public->samplesperblock ;

	pima->block   = (unsigned char*) pima->data ;
	pima->samples = (short*) (pima->data + public->blocksize) ;
	
	pima->samplecount = 0 ;
	
	return 0 ;
} /* ima_writer_init */



/*==========================================================================================
*/

static int		
ima_write_block (SF_PRIVATE *psf, IMA_ADPCM_PRIVATE *pima)
{	int		chan, k, step, diff, vpdiff, blockindex, index, indexstart ;
	short	bytecode, mask ;
	
	/* Write the block header. */

	for (chan = 0 ; chan < pima->channels ; chan++)
	{	pima->block [chan*4]   = pima->samples [chan] & 0xFF ;
		pima->block [chan*4+1] = (pima->samples [chan] >> 8) & 0xFF ;
		
		pima->block [chan*4+2] = pima->stepindex [chan] ;
		pima->block [chan*4+3] = 0 ;
		} ;
		
	pima->previous  [0] = pima->samples [0] ;
	pima->previous  [1] = pima->samples [1] ;

	/* Encode the samples as 4 bit. */

	for (k = pima->channels ; k < (pima->samplesperblock * pima->channels) ; k ++)
	{	chan = (pima->channels > 1) ? (k % 2) : 0 ;
	
		diff = pima->samples [k] - pima->previous [chan] ;
	
		bytecode = 0 ;
		step = ima_step_size [pima->stepindex [chan]] ;
		vpdiff = step >> 3 ;
		if (diff < 0) 
		{	bytecode = 8 ; 
			diff = -diff ;
			} ;
		mask = 4 ;
		while (mask)
		{	if (diff >= step)
			{	bytecode |= mask ;
				diff -= step ;
				vpdiff += step ;
				} ;
			step >>= 1 ;
			mask >>= 1 ;
			} ;

		if (bytecode & 8)
			pima->previous [chan] -= vpdiff ;
		else
			pima->previous [chan] += vpdiff ;

		if (pima->previous [chan] > 32767) 
			pima->previous [chan] = 32767;
		else if (pima->previous [chan] < -32768) 
			pima->previous [chan] = -32768;

		pima->stepindex [chan] += ima_index_adjust [bytecode] ;
		if (pima->stepindex [chan] < 0)
			pima->stepindex [chan] = 0 ;
		else if (pima->stepindex [chan] > 88)
			pima->stepindex [chan] = 88 ;
		
		pima->samples [k] = bytecode ;
		} ;

	/* Pack the 4 bit encoded samples. */

	blockindex = 4 * pima->channels ;
	
	indexstart = pima->channels ;
	while (blockindex <  pima->blocksize)
	{	for (chan = 0 ; chan < pima->channels ; chan++)
		{	index = indexstart + chan ;
			for (k = 0 ; k < 4 ; k++)
			{	pima->block [blockindex] = pima->samples [index] & 0x0F ;
				index += pima->channels ;
				pima->block [blockindex] |= (pima->samples [index] << 4) & 0xF0 ;
				index += pima->channels ;
				blockindex ++ ;
				} ;
			} ;
		indexstart += 8 * pima->channels ;
		} ;
		
	/* Write the block to disk. */

	if ((k = fwrite (pima->block, 1, pima->blocksize, psf->file)) != pima->blocksize)
		psf_log_printf (psf, "*** Warning : short write (%d != %d).\n", k, pima->blocksize) ;
		
	memset (pima->samples, 0, pima->samplesperblock * sizeof (short)) ;
	pima->samplecount = 0 ;
			
	return 1 ;
} /* ima_write_block */

static int 
ima_write (SF_PRIVATE *psf, IMA_ADPCM_PRIVATE *pima, short *ptr, int len)
{	int		count, total = 0, index = 0 ;
	
	while (index < len)
	{	count = (pima->samplesperblock - pima->samplecount) * pima->channels ;

		if (count > len - index)
			count = len - index ;

		memcpy (&(pima->samples [pima->samplecount * pima->channels]), &(ptr [index]), count * sizeof (short)) ;
		index += count ;
		pima->samplecount += count / pima->channels ;
		total = index ;

		if (pima->samplecount >= pima->samplesperblock)
			ima_write_block (psf, pima) ;	
		} ;

	return total ;		
} /* ima_write */

static int		
ima_write_s (SF_PRIVATE *psf, short *ptr, int len)
{	IMA_ADPCM_PRIVATE 	*pima ; 
	int				total ;

	if (! psf->fdata)
		return 0 ;
	pima = (IMA_ADPCM_PRIVATE*) psf->fdata ;
	
	total = ima_write (psf, pima, ptr, len) ;

	return total ;
} /* ima_write_s */

static int		
ima_write_i  (SF_PRIVATE *psf, int *ptr, int len)
{	IMA_ADPCM_PRIVATE *pima ; 
	short		*sptr ;
	int			k, bufferlen, writecount = 0, count ;
	int			index = 0, total = 0 ;

	if (! psf->fdata)
		return 0 ;
	pima = (IMA_ADPCM_PRIVATE*) psf->fdata ;
	
	sptr = (short*) psf->buffer ;
	bufferlen = ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (short) ;
	while (len > 0)
	{	writecount = (len >= bufferlen) ? bufferlen : len ;
		for (k = 0 ; k < writecount ; k++)
			sptr [k] = (short) ptr [index+k] ;
		count = ima_write (psf, pima, sptr, writecount) ;
		index += writecount ;
		total += count ;
		len -= writecount ;
		} ;
	return total ;
} /* ima_write_i */

static int		
ima_write_f  (SF_PRIVATE *psf, float *ptr, int len)
{	IMA_ADPCM_PRIVATE *pima ; 
	short		*sptr ;
	int			k, bufferlen, writecount = 0, count ;
	int			index = 0, total = 0 ;
	float		normfact ;

	if (! psf->fdata)
		return 0 ;
	pima = (IMA_ADPCM_PRIVATE*) psf->fdata ;
	
	normfact = (psf->norm_float == SF_TRUE) ? ((float) 0x8000) : 1.0 ;
	
	sptr = (short*) psf->buffer ;
	bufferlen = ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (short) ;
	while (len > 0)
	{	writecount = (len >= bufferlen) ? bufferlen : len ;
		for (k = 0 ; k < writecount ; k++)
			sptr [k] = FLOAT_TO_SHORT (normfact * ptr [index+k])  ;
		count = ima_write (psf, pima, sptr, writecount) ;
		index += writecount ;
		total += count ;
		len -= writecount ;
		} ;
	return total ;
} /* ima_write_f */

static int		
ima_write_d  (SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	IMA_ADPCM_PRIVATE *pima ; 
	short		*sptr ;
	int			k, bufferlen, writecount = 0, count ;
	int			index = 0, total = 0 ;
	double 		normfact ;
	
	normfact = (normalize ? ((double) 0x8000) : 1.0) ;

	if (! psf->fdata)
		return 0 ;
	pima = (IMA_ADPCM_PRIVATE*) psf->fdata ;
	
	sptr = (short*) psf->buffer ;
	bufferlen = ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (short) ;
	while (len > 0)
	{	writecount = (len >= bufferlen) ? bufferlen : len ;
		for (k = 0 ; k < writecount ; k++)
			sptr [k] = DOUBLE_TO_SHORT (normfact * ptr [index+k])  ;
		count = ima_write (psf, pima, sptr, writecount) ;
		index += writecount ;
		total += count ;
		len -= writecount ;
		} ;
	return total ;
} /* ima_write_d */

static int
wav_ima_write_header (SF_PRIVATE  *psf)
{	int  fmt_size, blockalign, samplesperblock, bytespersec ;

	blockalign      = wav_srate2blocksize (psf->sf.samplerate) ;
	samplesperblock = 2 * (blockalign - 4 * psf->sf.channels) / psf->sf.channels + 1 ;
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
	psf_binheader_writef (psf, "lww", fmt_size, WAVE_FORMAT_IMA_ADPCM, psf->sf.channels) ;

	/* fmt : samplerate, bytespersec. */
	psf_binheader_writef (psf, "ll", psf->sf.samplerate, bytespersec) ;

	/* fmt : blockalign, bitwidth, extrabytes, samplesperblock. */
	psf_binheader_writef (psf, "wwww", blockalign, 4, 2, samplesperblock) ;

	/* Fact chunk. */	
	psf_binheader_writef (psf, "mll", fact_MARKER, sizeof (int), psf->sf.samples) ;

	/* DATA chunk. */
	psf_binheader_writef (psf, "ml", data_MARKER, psf->datalength) ;

	fwrite (psf->header, psf->headindex, 1, psf->file) ;

	psf->dataoffset = psf->headindex ;

	psf->datalength  = (psf->sf.samples / samplesperblock) * samplesperblock ;
	if (psf->sf.samples % samplesperblock)
		psf->datalength += samplesperblock ;

	return 0 ;
} /* wav_ima_write_header */

static int	
wav_ima_close	(SF_PRIVATE  *psf)
{	IMA_ADPCM_PRIVATE *pima ; 

	if (! psf->fdata)
		return 0 ;

	pima = (IMA_ADPCM_PRIVATE*) psf->fdata ;

	if (psf->mode == SF_MODE_WRITE)
	{	/*	If a block has been partially assembled, write it out
		**	as the final block.
		*/
	
		if (pima->samplecount && pima->samplecount < pima->samplesperblock)
			ima_write_block (psf, pima) ;	

		/*  Now we know for certain the length of the file we can
		**  re-write the header.
		*/
		 
		fseek (psf->file, 0, SEEK_END) ;
		psf->filelength = ftell (psf->file) ;

		psf->sf.samples = pima->samplesperblock * pima->blockcount ;
		psf->datalength = psf->filelength - psf->dataoffset ;

		wav_ima_write_header (psf) ;
		} ;

	if (psf->fdata)
		free (psf->fdata) ;
	psf->fdata = NULL ;

	return 0 ;
} /* wav_ima_close */

static unsigned 
int wav_srate2blocksize (unsigned int srate)
{	if (srate < 12000)
		return 256 ;
	if (srate < 23000)
		return 512 ;
	return 1024 ;
} /* wav_srate2blocksize */
