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

/* These required here because we write the header in this file. */

#define RIFF_MARKER	(MAKE_MARKER ('R', 'I', 'F', 'F')) 
#define WAVE_MARKER	(MAKE_MARKER ('W', 'A', 'V', 'E')) 
#define fmt_MARKER	(MAKE_MARKER ('f', 'm', 't', ' ')) 
#define fact_MARKER	(MAKE_MARKER ('f', 'a', 'c', 't')) 
#define data_MARKER	(MAKE_MARKER ('d', 'a', 't', 'a')) 

#define WAVE_FORMAT_MS_ADPCM	0x0002

typedef struct
{	unsigned int	channels, blocksize, samplesperblock, blocks, dataremaining ; 
	int				blockcount, samplecount ;
	unsigned char	*block ;
	short			*samples ;
	unsigned char	dummydata [4] ; /* Dummy size */
} MSADPCM_PRIVATE ;

/*============================================================================================
** MS ADPCM static data and functions.
*/

static int AdaptationTable []    = 
{	230, 230, 230, 230, 307, 409, 512, 614,
	768, 614, 512, 409, 307, 230, 230, 230 
} ;

/* TODO : The first 7 coef's are are always hardcode and must
   appear in the actual WAVE file.  They should be read in
   in case a sound program added extras to the list. */

static int AdaptCoeff1 [] = 
{	256, 512, 0, 192, 240, 460, 392 
} ;

static int AdaptCoeff2 [] = 
{	0, -256, 0, 64, 0, -208, -232
} ;

/*============================================================================================
**	MS ADPCM Block Layout.
**	======================
**	Block is usually 256, 512 or 1024 bytes depending on sample rate.
**	For a mono file, the block is laid out as follows:
**		byte	purpose
**		0		block predictor [0..6]
**		1,2		initial idelta (positive)
**		3,4		sample 1
**		5,6		sample 0
**		7..n	packed bytecodes
**
**	For a stereo file, the block is laid out as follows:
**		byte	purpose
**		0		block predictor [0..6] for left channel
**		1		block predictor [0..6] for right channel
**		2,3		initial idelta (positive) for left channel
**		4,5		initial idelta (positive) for right channel
**		6,7		sample 1 for left channel
**		8,9		sample 1 for right channel
**		10,11	sample 0 for left channel
**		12,13	sample 0 for right channel
**		14..n	packed bytecodes
*/

/*============================================================================================
** Static functions.
*/

static	int	msadpcm_decode_block (SF_PRIVATE *psf, MSADPCM_PRIVATE *pms) ;
static	int msadpcm_read (SF_PRIVATE *psf, MSADPCM_PRIVATE *pms, short *ptr, int len) ;

static	int	msadpcm_encode_block (SF_PRIVATE *psf, MSADPCM_PRIVATE *pms) ;
static	int msadpcm_write (SF_PRIVATE *psf, MSADPCM_PRIVATE *pms, short *ptr, int len) ;

static	int	msadpcm_read_s (SF_PRIVATE *psf, short *ptr, int len) ;
static	int	msadpcm_read_i (SF_PRIVATE *psf, int *ptr, int len) ;
static	int	msadpcm_read_f (SF_PRIVATE *psf, float *ptr, int len) ;
static	int	msadpcm_read_d (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;

static int	msadpcm_write_s (SF_PRIVATE *psf, short *ptr, int len) ;
static int	msadpcm_write_i  (SF_PRIVATE *psf, int *ptr, int len) ;
static int	msadpcm_write_f  (SF_PRIVATE *psf, float *ptr, int len) ;
static int	msadpcm_write_d  (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;

static long msadpcm_seek   (SF_PRIVATE *psf, long offset, int whence) ;
static	int	msadpcm_close	(SF_PRIVATE  *psf) ;

static int	msadpcm_write_header (SF_PRIVATE  *psf) ;

static	unsigned int srate2blocksize (unsigned int srate_chan_product) ;
static	void	choose_predictor (unsigned int channels, short *data, int *bpred, int *idelta) ;

/*============================================================================================
** MS ADPCM Read Functions.
*/

int	
wav_msadpcm_reader_init (SF_PRIVATE *psf, WAV_FMT *fmt)
{	MSADPCM_PRIVATE	*pms ;
	unsigned int	pmssize ;
	int				count ;
	
	pmssize = sizeof (MSADPCM_PRIVATE) + fmt->msadpcm.blockalign + 3 * fmt->msadpcm.channels * fmt->msadpcm.samplesperblock ;

	if (! (psf->fdata = malloc (pmssize)))
		return SFE_MALLOC_FAILED ;
	pms = (MSADPCM_PRIVATE*) psf->fdata ;
	memset (pms, 0, pmssize) ;
	
	pms->block   = (unsigned char*) pms->dummydata ;
	pms->samples = (short*) (pms->dummydata + fmt->msadpcm.blockalign) ;
	
	pms->channels        = fmt->msadpcm.channels ;
	pms->blocksize       = fmt->msadpcm.blockalign ;
	pms->samplesperblock = fmt->msadpcm.samplesperblock ;

	pms->dataremaining	 = psf->datalength ;

	if (psf->datalength % pms->blocksize)
		pms->blocks = psf->datalength / pms->blocksize  + 1 ;
	else
		pms->blocks = psf->datalength / pms->blocksize ;

	count = 2 * (pms->blocksize - 6 * pms->channels) / pms->channels ;
	if (pms->samplesperblock != count)
		psf_log_printf (psf, "*** Warning : samplesperblock shoud be %d.\n", count) ;

	psf->sf.samples = (psf->datalength / pms->blocksize) * pms->samplesperblock ;

	psf_log_printf (psf, " bpred   idelta\n") ;

	msadpcm_decode_block (psf, pms) ;
	
	psf->read_short  = (func_short)  msadpcm_read_s ;
	psf->read_int    = (func_int)    msadpcm_read_i ;
	psf->read_float  = (func_float)  msadpcm_read_f ;
	psf->read_double = (func_double) msadpcm_read_d ;

	psf->seek_func   = (func_seek)   msadpcm_seek ;

	return 0 ;
} /* wav_msadpcm_reader_init */

static int		
msadpcm_decode_block (SF_PRIVATE *psf, MSADPCM_PRIVATE *pms)
{	int		chan, k, blockindex, sampleindex ;
	short	bytecode, bpred [2], chan_idelta [2] ;
	
    int predict ;
    int current ;
    int idelta ;

	pms->blockcount ++ ;
	pms->samplecount = 0 ;
	
	if (pms->blockcount > pms->blocks)
	{	memset (pms->samples, 0, pms->samplesperblock * pms->channels) ;
		return 1 ;
		} ;

	if ((k = fread (pms->block, 1, pms->blocksize, psf->file)) != pms->blocksize)
		psf_log_printf (psf, "*** Warning : short read (%d != %d).\n", k, pms->blocksize) ;

	/* Read and check the block header. */
	
	if (pms->channels == 1)
	{	bpred [0] = pms->block [0] ;
	
		if (bpred [0] >= 7)
			psf_log_printf (psf, "MS ADPCM synchronisation error (%d).\n", bpred [0]) ;
	
		chan_idelta [0] = pms->block [1] | (pms->block [2] << 8) ;
		chan_idelta [1] = 0 ;

		psf_log_printf (psf, "(%d) (%d)\n", bpred [0], chan_idelta [0]) ;

		pms->samples [1] = pms->block [3] | (pms->block [4] << 8) ;
		pms->samples [0] = pms->block [5] | (pms->block [6] << 8) ;
		blockindex = 7 ;
		}
	else
	{	bpred [0] = pms->block [0] ;
		bpred [1] = pms->block [1] ;
	
		if (bpred [0] >= 7 || bpred [1] >= 7)
			psf_log_printf (psf, "MS ADPCM synchronisation error (%d %d).\n", bpred [0], bpred [1]) ;
	
		chan_idelta [0] = pms->block [2] | (pms->block [3] << 8) ;
		chan_idelta [1] = pms->block [4] | (pms->block [5] << 8) ;

		psf_log_printf (psf, "(%d, %d) (%d, %d)\n", bpred [0], bpred [1], chan_idelta [0], chan_idelta [1]) ;

		pms->samples [2] = pms->block [6] | (pms->block [7] << 8) ;
		pms->samples [3] = pms->block [8] | (pms->block [9] << 8) ;

		pms->samples [0] = pms->block [10] | (pms->block [11] << 8) ;
		pms->samples [1] = pms->block [12] | (pms->block [13] << 8) ;

		blockindex = 14 ;
		} ;

    if (chan_idelta [0] & 0x8000) 
		chan_idelta [0] -= 0x10000 ;
    if (chan_idelta [1] & 0x8000) 
		chan_idelta [1] -= 0x10000 ;
		
	/* Pull apart the packed 4 bit samples and store them in their
	** correct sample positions.
	*/
	
	sampleindex = 2 * pms->channels ;
	while (blockindex <  pms->blocksize)
	{	bytecode = pms->block [blockindex++] ;
  		pms->samples [sampleindex++] = (bytecode >> 4) & 0x0F ;
		pms->samples [sampleindex++] = bytecode & 0x0F ;
		} ;
		
	/* Decode the encoded 4 bit samples. */
	
	for (k = 2 * pms->channels ; k < (pms->samplesperblock * pms->channels) ; k ++)
	{	chan = (pms->channels > 1) ? (k % 2) : 0 ;

		bytecode = pms->samples [k] & 0xF ;

	    /** Compute next Adaptive Scale Factor (ASF) **/
	    idelta = chan_idelta [chan] ;
	    chan_idelta [chan] = (AdaptationTable [bytecode] * idelta) >> 8 ; /* => / 256 => FIXED_POINT_ADAPTATION_BASE == 256 */
	    if (chan_idelta [chan] < 16) 
			chan_idelta [chan] = 16 ;
	    if (bytecode & 0x8) 
			bytecode -= 0x10 ;
		
    	predict = ((pms->samples [k - pms->channels] * AdaptCoeff1 [bpred [chan]]) 
					+ (pms->samples [k - 2 * pms->channels] * AdaptCoeff2 [bpred [chan]])) >> 8 ; /* => / 256 => FIXED_POINT_COEFF_BASE == 256 */
    	current = (bytecode * idelta) + predict;
    
	    if (current > 32767) 
			current = 32767 ;
	    else if (current < -32768) 
			current = -32768 ;
    
		pms->samples [k] = current ;
		} ;

	return 1 ;
} /* msadpcm_decode_block */

static int 
msadpcm_read (SF_PRIVATE *psf, MSADPCM_PRIVATE *pms, short *ptr, int len)
{	int		count, total = 0, index = 0 ;
	
	while (index < len)
	{	if (pms->blockcount >= pms->blocks && pms->samplecount >= pms->samplesperblock)
		{	memset (&(ptr[index]), 0, (len - index) * sizeof (short)) ;
			return total ;
			} ;
		
		if (pms->samplecount >= pms->samplesperblock)
			msadpcm_decode_block (psf, pms) ;
		
		count = (pms->samplesperblock - pms->samplecount) * pms->channels ;
		count = (len - index > count) ? count : len - index ;
		
		memcpy (&(ptr[index]), &(pms->samples [pms->samplecount * pms->channels]), count * sizeof (short)) ;
		index += count ;
		pms->samplecount += count / pms->channels ;
		total = index ;
		} ;

	return total ;		
} /* msadpcm_read */

static int		
msadpcm_read_s (SF_PRIVATE *psf, short *ptr, int len)
{	MSADPCM_PRIVATE 	*pms ; 
	int				total ;

	if (! psf->fdata)
		return 0 ;
	pms = (MSADPCM_PRIVATE*) psf->fdata ;
	
	total = msadpcm_read (psf, pms, ptr, len) ;

	return total ;
} /* msadpcm_read_s */

static int		
msadpcm_read_i  (SF_PRIVATE *psf, int *ptr, int len)
{	MSADPCM_PRIVATE *pms ; 
	short		*sptr ;
	int			k, bufferlen, readcount = 0, count ;
	int			index = 0, total = 0 ;

	if (! psf->fdata)
		return 0 ;
	pms = (MSADPCM_PRIVATE*) psf->fdata ;
	
	sptr = (short*) psf->buffer ;
	bufferlen = ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (short) ;
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : len ;
		count = msadpcm_read (psf, pms, sptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [index+k] = (int) (sptr [k]) ;
		index += readcount ;
		total += count ;
		len -= readcount ;
		} ;
	return total ;
} /* msadpcm_read_i */

static int
msadpcm_read_f  (SF_PRIVATE *psf, float *ptr, int len)
{	MSADPCM_PRIVATE *pms ; 
	short		*sptr ;
	int			k, bufferlen, readcount = 0, count ;
	int			index = 0, total = 0 ;
	float		normfact ;

	if (! psf->fdata)
		return 0 ;
	pms = (MSADPCM_PRIVATE*) psf->fdata ;
	
	normfact = (psf->norm_float == SF_TRUE) ? 1.0 / ((float) 0x8000) : 1.0 ;
	sptr = (short*) psf->buffer ;
	bufferlen = ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (short) ;
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : len ;
		count = msadpcm_read (psf, pms, sptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [index+k] = normfact * (float) (sptr [k]) ;
		index += readcount ;
		total += count ;
		len -= readcount ;
		} ;
	return total ;
} /* msadpcm_read_f */

static int		
msadpcm_read_d  (SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	MSADPCM_PRIVATE *pms ; 
	short		*sptr ;
	int			k, bufferlen, readcount = 0, count ;
	int			index = 0, total = 0 ;
	double 		normfact ;
	
	normfact = (normalize ? 1.0 / ((double) 0x8000) : 1.0) ;

	if (! psf->fdata)
		return 0 ;
	pms = (MSADPCM_PRIVATE*) psf->fdata ;
	
	sptr = (short*) psf->buffer ;
	bufferlen = ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (short) ;
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : len ;
		count = msadpcm_read (psf, pms, sptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [index+k] = normfact * (double) (sptr [k]) ;
		index += readcount ;
		total += count ;
		len -= readcount ;
		} ;
	return total ;
} /* msadpcm_read_d */

static long    
msadpcm_seek   (SF_PRIVATE *psf, long offset, int whence)
{	MSADPCM_PRIVATE *pms ; 
	int			newblock, newsample ;
	
	if (! psf->fdata)
		return 0 ;
	pms = (MSADPCM_PRIVATE*) psf->fdata ;

	if (! (psf->blockwidth && psf->datalength && psf->dataoffset))
	{	psf->error = SFE_BAD_SEEK ;
		return	((long) -1) ;
		} ;
		
	switch (whence)
	{	case SEEK_SET :
				if (offset < 0 || offset > pms->blocks * pms->samplesperblock)
				{	psf->error = SFE_BAD_SEEK ;
					return	((long) -1) ;
					} ;
				newblock  = offset / pms->samplesperblock ;
				newsample = offset % pms->samplesperblock ;
				break ;
				
		case SEEK_CUR :
				if (psf->current + offset < 0 || psf->current + offset > pms->blocks * pms->samplesperblock)
				{	psf->error = SFE_BAD_SEEK ;
					return	((long) -1) ;
					} ;
				newblock  = (psf->current + offset) / pms->samplesperblock ;
				newsample = (psf->current + offset) % pms->samplesperblock ;
				break ;
				
		case SEEK_END :
				if (offset > 0 || pms->samplesperblock * pms->blocks + offset < 0)
				{	psf->error = SFE_BAD_SEEK ;
					return	((long) -1) ;
					} ;
				newblock  = (pms->samplesperblock * pms->blocks + offset) / pms->samplesperblock ;
				newsample = (pms->samplesperblock * pms->blocks + offset) % pms->samplesperblock ;
				break ;
				
		default : 
				psf->error = SFE_BAD_SEEK ;
				return	((long) -1) ;
		} ;
		
	if (psf->mode == SF_MODE_READ)
	{	fseek (psf->file, (int) (psf->dataoffset + newblock * pms->blocksize), SEEK_SET) ;
		pms->blockcount  = newblock ;
		msadpcm_decode_block (psf, pms) ;
		pms->samplecount = newsample ;
		}
	else
	{	/* What to do about write??? */ 
		psf->error = SFE_BAD_SEEK ;
		return	((long) -1) ;
		} ;

	psf->current = newblock * pms->samplesperblock + newsample ;
	return psf->current ;
} /* msadpcm_seek */

/*==========================================================================================
** MS ADPCM Write Functions.
*/

int	
wav_msadpcm_writer_init (SF_PRIVATE *psf)
{	MSADPCM_PRIVATE	*pms ;
	unsigned int 	pmssize, blockalign, samplesperblock ;
	
	blockalign      = srate2blocksize (psf->sf.samplerate * psf->sf.channels) ;	
	samplesperblock = 2 + 2 * (blockalign - 7 * psf->sf.channels) / psf->sf.channels ;
	
	pmssize = sizeof (MSADPCM_PRIVATE) + blockalign + 3 * psf->sf.channels * samplesperblock ;

	if (! (psf->fdata = malloc (pmssize)))
		return SFE_MALLOC_FAILED ;
	pms = (MSADPCM_PRIVATE*) psf->fdata ;
	memset (pms, 0, pmssize) ;
	
	pms->channels        = psf->sf.channels ;
	pms->blocksize       = blockalign ;
	pms->samplesperblock = samplesperblock ;

	pms->block   = (unsigned char*) pms->dummydata ;
	pms->samples = (short*) (pms->dummydata + blockalign) ;
	
	pms->samplecount = 0 ;
	
	msadpcm_write_header (psf) ;
	
	psf->write_short  = (func_short)   msadpcm_write_s ;
	psf->write_int    = (func_int)     msadpcm_write_i ;
	psf->write_float  = (func_float)   msadpcm_write_f ;
	psf->write_double = (func_double)  msadpcm_write_d ;
	
	psf->seek_func    = (func_seek)    msadpcm_seek ;
	psf->close        = (func_close)   msadpcm_close ;
	psf->write_header = (func_wr_hdr)  msadpcm_write_header ;

	return 0 ;
} /* wav_msadpcm_writer_init */

/*==========================================================================================
*/

static int		
msadpcm_encode_block (SF_PRIVATE *psf, MSADPCM_PRIVATE *pms)
{	unsigned int	blockindex ;
	unsigned char	byte ;
	int				chan, k, predict, bpred [2], idelta [2], errordelta, newsamp ;
	
	choose_predictor (pms->channels, pms->samples, bpred, idelta) ;

	/* Write the block header. */

	if (pms->channels == 1)
	{	pms->block [0]	= bpred [0] ;	
		pms->block [1]	= idelta [0] & 0xFF ;
		pms->block [2]	= idelta [0] >> 8 ;
		pms->block [3]	= pms->samples [1] & 0xFF ;
		pms->block [4]	= pms->samples [1] >> 8 ;
		pms->block [5]	= pms->samples [0] & 0xFF ;
		pms->block [6]	= pms->samples [0] >> 8 ;

		blockindex = 7 ;
		byte = 0 ;

		/* Encode the samples as 4 bit. */
		
		for (k = 2 ; k < pms->samplesperblock ; k++)
		{	predict = (pms->samples [k-1] * AdaptCoeff1 [bpred [0]] + pms->samples [k-2] * AdaptCoeff2 [bpred [0]]) >> 8 ;
			errordelta = (pms->samples [k] - predict) / idelta [0] ;
			if (errordelta < -8)
				errordelta = -8 ;
			else if (errordelta > 7)
				errordelta = 7 ;
			newsamp = predict + (idelta [0] * errordelta) ;
			if (newsamp > 32767)
				newsamp = 32767 ;
			else if (newsamp < -32768)
				newsamp = -32768 ;
			if (errordelta < 0)
				errordelta += 0x10 ;
				
			byte = (byte << 4) | (errordelta & 0xF) ;
			if (k % 2)
			{	pms->block [blockindex++] = byte ;
				byte = 0 ;
				} ;

			idelta [0] = (idelta [0] * AdaptationTable [errordelta]) >> 8 ;
			if (idelta [0] < 16)
				idelta [0] = 16 ;
			pms->samples [k] = newsamp ;
			} ;
		}
	else
	{	/* Stereo file. */
		pms->block [0]	= bpred [0] ;	
		pms->block [1]	= bpred [1] ;	

		pms->block [2]	= idelta [0] & 0xFF ;
		pms->block [3]	= idelta [0] >> 8 ;
		pms->block [4]	= idelta [1] & 0xFF ;
		pms->block [5]	= idelta [1] >> 8 ;
		
		pms->block [6]	= pms->samples [2] & 0xFF ;
		pms->block [7]	= pms->samples [2] >> 8 ;
		pms->block [8]	= pms->samples [3] & 0xFF ;
		pms->block [9]	= pms->samples [3] >> 8 ;

		pms->block [10]	= pms->samples [0] & 0xFF ;
		pms->block [11]	= pms->samples [0] >> 8 ;
		pms->block [12]	= pms->samples [1] & 0xFF ;
		pms->block [13]	= pms->samples [1] >> 8 ;
	
		blockindex = 14 ;
		byte = 0 ;
		chan = 1 ;
		
		for (k = 4 ; k < 2 * pms->samplesperblock ; k++)
		{	chan = k & 1 ;

			predict = (pms->samples [k-2] * AdaptCoeff1 [bpred [chan]] + pms->samples [k-4] * AdaptCoeff2 [bpred [chan]]) >> 8 ;
			errordelta = (pms->samples [k] - predict) / idelta [chan] ;


			if (errordelta < -8)
				errordelta = -8 ;
			else if (errordelta > 7)
				errordelta = 7 ;
			newsamp = predict + (idelta [chan] * errordelta) ;
			if (newsamp > 32767)
				newsamp = 32767 ;
			else if (newsamp < -32768)
				newsamp = -32768 ;
			if (errordelta < 0)
				errordelta += 0x10 ;
				
			byte = (byte << 4) | (errordelta & 0xF) ;

			if (chan)
			{	pms->block [blockindex++] = byte ;
				byte = 0 ;
				} ;

			idelta [chan] = (idelta [chan] * AdaptationTable [errordelta]) >> 8 ;
			if (idelta [chan] < 16)
				idelta [chan] = 16 ;
			pms->samples [k] = newsamp ;
			} ;
		} ;

	/* Write the block to disk. */

	if ((k = fwrite (pms->block, 1, pms->blocksize, psf->file)) != pms->blocksize)
		psf_log_printf (psf, "*** Warning : short write (%d != %d).\n", k, pms->blocksize) ;
		
	memset (pms->samples, 0, pms->samplesperblock * sizeof (short)) ;

	pms->blockcount ++ ;
	pms->samplecount = 0 ;
			
	return 1 ;
} /* msadpcm_encode_block */

static int 
msadpcm_write (SF_PRIVATE *psf, MSADPCM_PRIVATE *pms, short *ptr, int len)
{	int		count, total = 0, index = 0 ;
	
	while (index < len)
	{	count = (pms->samplesperblock - pms->samplecount) * pms->channels ;

		if (count > len - index)
			count = len - index ;

		memcpy (&(pms->samples [pms->samplecount * pms->channels]), &(ptr [index]), count * sizeof (short)) ;
		index += count ;
		pms->samplecount += count / pms->channels ;
		total = index ;

		if (pms->samplecount >= pms->samplesperblock)
			msadpcm_encode_block (psf, pms) ;	
		} ;

	return total ;		
} /* msadpcm_write */

static int		
msadpcm_write_s (SF_PRIVATE *psf, short *ptr, int len)
{	MSADPCM_PRIVATE *pms ; 
	int				total ;

	if (! psf->fdata)
		return 0 ;
	pms = (MSADPCM_PRIVATE*) psf->fdata ;
	
	total = msadpcm_write (psf, pms, ptr, len) ;

	return total ;
} /* msadpcm_write_s */

static int		
msadpcm_write_i  (SF_PRIVATE *psf, int *ptr, int len)
{	MSADPCM_PRIVATE *pms ; 
	short		*sptr ;
	int			k, bufferlen, writecount = 0, count ;
	int			index = 0, total = 0 ;

	if (! psf->fdata)
		return 0 ;
	pms = (MSADPCM_PRIVATE*) psf->fdata ;
	
	sptr = (short*) psf->buffer ;
	bufferlen = ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (short) ;
	while (len > 0)
	{	writecount = (len >= bufferlen) ? bufferlen : len ;
		for (k = 0 ; k < writecount ; k++)
			sptr [k] = (short) ptr [index+k] ;
		count = msadpcm_write (psf, pms, sptr, writecount) ;
		index += writecount ;
		total += count ;
		len -= writecount ;
		} ;
	return total ;
} /* msadpcm_write_i */

static int
msadpcm_write_f  (SF_PRIVATE *psf, float *ptr, int len)
{	MSADPCM_PRIVATE *pms ; 
	short		*sptr ;
	int			k, bufferlen, writecount = 0, count ;
	int			index = 0, total = 0 ;
	float		normfact ;
	
	if (! psf->fdata)
		return 0 ;
	pms = (MSADPCM_PRIVATE*) psf->fdata ;
	
	normfact = (psf->norm_float == SF_TRUE) ? ((float) 0x8000) : 1.0 ;
	
	sptr = (short*) psf->buffer ;
	bufferlen = ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (short) ;
	while (len > 0)
	{	writecount = (len >= bufferlen) ? bufferlen : len ;
		for (k = 0 ; k < writecount ; k++)
			sptr [k] = FLOAT_TO_SHORT (normfact * ptr [index+k]) ;
		count = msadpcm_write (psf, pms, sptr, writecount) ;
		index += writecount ;
		total += count ;
		len -= writecount ;
		} ;
	return total ;
} /* msadpcm_write_f */

static int		
msadpcm_write_d  (SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	MSADPCM_PRIVATE *pms ; 
	short		*sptr ;
	int			k, bufferlen, writecount = 0, count ;
	int			index = 0, total = 0 ;
	double 		normfact ;
	
	normfact = (normalize ? ((double) 0x8000) : 1.0) ;

	if (! psf->fdata)
		return 0 ;
	pms = (MSADPCM_PRIVATE*) psf->fdata ;
	
	sptr = (short*) psf->buffer ;
	bufferlen = ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (short) ;
	while (len > 0)
	{	writecount = (len >= bufferlen) ? bufferlen : len ;
		for (k = 0 ; k < writecount ; k++)
			sptr [k] = DOUBLE_TO_SHORT (normfact * ptr [index+k]) ;
		count = msadpcm_write (psf, pms, sptr, writecount) ;
		index += writecount ;
		total += count ;
		len -= writecount ;
		} ;
	return total ;
} /* msadpcm_write_d */

/*========================================================================================
*/

static int
msadpcm_write_header (SF_PRIVATE  *psf)
{	int  k, fmt_size, blockalign, samplesperblock, bytespersec, extrabytes ;

	blockalign      = srate2blocksize (psf->sf.samplerate * psf->sf.channels) ;	
	samplesperblock = 2 + 2 * (blockalign - 7 * psf->sf.channels) / psf->sf.channels ;
	bytespersec     = (psf->sf.samplerate * blockalign) / samplesperblock ;
	
	/* Reset the current header length to zero. */
	psf->header [0] = 0 ;
	psf->headindex = 0 ;
	fseek (psf->file, 0, SEEK_SET) ;

	/* RIFF marker, length, WAVE and 'fmt ' markers. */		
	psf_binheader_writef (psf, "mlmm", RIFF_MARKER, psf->filelength - 8, WAVE_MARKER, fmt_MARKER) ;

	/* fmt chunk. */
	extrabytes = 2 + 2 + 7 * (2 + 2) ;
	fmt_size   = 2 + 2 + 4 + 4 + 2 + 2 + 2 + extrabytes ;
	
	/* fmt : size, WAV format type, channels. */
	psf_binheader_writef (psf, "lww", fmt_size, WAVE_FORMAT_MS_ADPCM, psf->sf.channels) ;

	/* fmt : samplerate, bytespersec. */
	psf_binheader_writef (psf, "ll", psf->sf.samplerate, bytespersec) ;

	/* fmt : blockalign, bitwidth, extrabytes, samplesperblock. */
	psf_binheader_writef (psf, "wwwww", blockalign, 4, extrabytes, samplesperblock, 7) ;
	
	for (k = 0 ; k < 7 ; k++)
		psf_binheader_writef (psf, "ww", AdaptCoeff1 [k], AdaptCoeff2 [k]) ;

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
} /* msadpcm_write_header */

static int	
msadpcm_close	(SF_PRIVATE  *psf)
{	MSADPCM_PRIVATE *pms ; 

	if (! psf->fdata)
		return 0 ;

	pms = (MSADPCM_PRIVATE*) psf->fdata ;

	if (psf->mode == SF_MODE_WRITE)
	{	/*  Now we know static int for certain the length of the file we can
		**  re-write the header.
		*/
		 
		if (pms->samplecount && pms->samplecount < pms->samplesperblock)
			msadpcm_encode_block (psf, pms) ;	

		fseek (psf->file, 0, SEEK_END) ;
		psf->filelength = ftell (psf->file) ;

		psf->sf.samples = pms->samplesperblock * pms->blockcount ;
		psf->datalength = psf->filelength - psf->dataoffset ;

		msadpcm_write_header (psf) ;
		} ;

	if (psf->fdata)
		free (psf->fdata) ;
	psf->fdata = NULL ;

	return 0 ;
} /* msadpcm_close */

/*========================================================================================
** Static functions.
*/

static	unsigned int 
srate2blocksize (unsigned int srate_chan_product)
{	if (srate_chan_product < 12000)
		return 256 ;
	if (srate_chan_product < 23000)
		return 512 ;
	if (srate_chan_product < 44000)
		return 1024 ;
	return 2048 ;
} /* srate2blocksize */

/*----------------------------------------------------------------------------------------
**	Choosing the block predictor.
**	Each block requires a predictor and an idelta for each channel. 
**	The predictor is in the range [0..6] which is an index into the	two AdaptCoeff tables. 
**	The predictor is chosen by trying all of the possible predictors on a small set of
**	samples at the beginning of the block. The predictor with the smallest average
**	abs (idelta) is chosen as the best predictor for this block. 
**	The value of idelta is chosen to to give a 4 bit code value of +/- 4 (approx. half the 
**	max. code value). If the average abs (idelta) is zero, the sixth predictor is chosen.
**	If the value of idelta is less then 16 it is set to 16.
**
**	Microsoft uses an IDELTA_COUNT (number of sample pairs used to choose best predictor)
**	value of 3. The best possible results would be obtained by using all the samples to
**	choose the predictor.
*/

#define		IDELTA_COUNT	3

static	void	
choose_predictor (unsigned int channels, short *data, int *block_pred, int *idelta)
{	unsigned int	chan, k, bpred, idelta_sum, best_bpred, best_idelta ;
	
	for (chan = 0 ; chan < channels; chan++)
	{	best_bpred = best_idelta = 0 ;

		for (bpred = 0 ; bpred < 7 ; bpred++)
		{	idelta_sum = 0 ;
			for (k = 2 ; k < 2 + IDELTA_COUNT ; k++)
				idelta_sum += abs (data [k*channels] - ((data [(k-1)*channels] * AdaptCoeff1 [bpred] + data [(k-2)*channels] * AdaptCoeff2 [bpred]) >> 8)) ;
			idelta_sum /= (4 * IDELTA_COUNT) ;
			
			if (bpred == 0 || idelta_sum < best_idelta)
			{	best_bpred = bpred ;
				best_idelta = idelta_sum ;
				} ;
				
			if (! idelta_sum)
			{	best_bpred = bpred ;
				best_idelta = 16 ;
				break ;
				} ;
		
			} ; /* for bpred ... */
		if (best_idelta < 16)
			best_idelta = 16 ;
	
		block_pred [chan] = best_bpred ;
		idelta [chan]     = best_idelta ;
		} ;

	return ;
} /* choose_predictor */
