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

#include	"config.h"
#include	"sndfile.h"
#include	"sfendian.h"
#include	"floatcast.h"
#include	"common.h"
#include	"au.h"
#include	"G72x/g72x.h"

static	int	au_g72x_read_block (SF_PRIVATE *psf, G72x_DATA *pg72x) ;
static	int au_g72x_read (SF_PRIVATE *psf, G72x_DATA *pg72x, short *ptr, int len) ;

static	int	au_g72x_write_block (SF_PRIVATE *psf, G72x_DATA *pg72x) ;
static	int au_g72x_write (SF_PRIVATE *psf, G72x_DATA *pg72x, short *ptr, int len) ;

static	int	au_g72x_read_s (SF_PRIVATE *psf, short *ptr, int len) ;
static	int	au_g72x_read_i (SF_PRIVATE *psf, int *ptr, int len) ;
static	int	au_g72x_read_f (SF_PRIVATE *psf, float *ptr, int len) ;
static	int	au_g72x_read_d (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;

static	int	au_g72x_write_s (SF_PRIVATE *psf, short *ptr, int len) ;
static	int	au_g72x_write_i (SF_PRIVATE *psf, int *ptr, int len) ;
static	int	au_g72x_write_f (SF_PRIVATE *psf, float *ptr, int len) ;
static	int	au_g72x_write_d (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;

static  long au_g72x_seek (SF_PRIVATE *psf, long offset, int whence) ;

static	int	au_g72x_close	(SF_PRIVATE  *psf) ;


/*============================================================================================
** WAV G721 Reader initialisation function.
*/

int	
au_g72x_reader_init (SF_PRIVATE *psf, int codec)
{	G72x_DATA	*pg72x ;
	int	bitspersample ;
	
	psf->sf.seekable = SF_FALSE ;
	
	if (psf->mode != SF_MODE_READ)
		return SFE_BAD_MODE_RW ;

	if (! (pg72x = malloc (sizeof (G72x_DATA))))
		return SFE_MALLOC_FAILED ;

	psf->fdata = (void*) pg72x ;

	pg72x->blockcount  = 0 ;
	pg72x->samplecount = 0 ;

	switch (codec)
	{	case AU_H_G721_32 :
				g72x_reader_init (pg72x, G721_32_BITS_PER_SAMPLE) ;
				pg72x->bytesperblock = G721_32_BYTES_PER_BLOCK ;
				bitspersample = G721_32_BITS_PER_SAMPLE ;
				break ;
				
		case AU_H_G723_24:
				g72x_reader_init (pg72x, G723_24_BITS_PER_SAMPLE) ;
				pg72x->bytesperblock = G723_24_BYTES_PER_BLOCK ;
				bitspersample = G723_24_BITS_PER_SAMPLE ;
				break ;
		default : return 0 ;
		} ;
				
	psf->read_short  = (func_short)  au_g72x_read_s ;
	psf->read_int    = (func_int)    au_g72x_read_i ;
	psf->read_float  = (func_float)  au_g72x_read_f ;
	psf->read_double = (func_double) au_g72x_read_d ;
 
 	psf->seek_func   = (func_seek)   au_g72x_seek ;
 	psf->close       = (func_close)  au_g72x_close ;

	if (psf->datalength % pg72x->blocksize)
		pg72x->blocks = (psf->datalength / pg72x->blocksize) + 1 ;
	else
		pg72x->blocks = psf->datalength / pg72x->blocksize ;

	psf->sf.samples = (8 * psf->datalength) / bitspersample ;

	if ((psf->sf.samples * bitspersample) / 8 != psf->datalength)
		psf_log_printf (psf, "*** Warning : weird psf->datalength.\n") ;
			
	psf->blockwidth = psf->bytewidth = 1 ;

	au_g72x_read_block (psf, pg72x) ;
	
	return 0 ;	
} /* au_g72x_reader_init */

/*============================================================================================
** WAV G721 writer initialisation function.
*/

int	
au_g72x_writer_init (SF_PRIVATE *psf, int codec)
{	G72x_DATA	*pg72x ;
	
	psf->sf.seekable = SF_FALSE ;
	
	if (psf->mode != SF_MODE_WRITE)
		return SFE_BAD_MODE_RW ;

	if (! (pg72x = malloc (sizeof (G72x_DATA))))
		return SFE_MALLOC_FAILED ;

	psf->fdata = (void*) pg72x ;

	pg72x->blockcount  = 0 ;
	pg72x->samplecount = 0 ;

	switch (codec)
	{	case AU_H_G721_32 :
				g72x_writer_init (pg72x, G721_32_BITS_PER_SAMPLE) ;
				pg72x->bytesperblock = G721_32_BYTES_PER_BLOCK ;
				break ;
				
		case AU_H_G723_24:
				g72x_writer_init (pg72x, G723_24_BITS_PER_SAMPLE) ;
				pg72x->bytesperblock = G723_24_BYTES_PER_BLOCK ;
				break ;
		default : return 0 ;
		} ;

	psf->write_short  = (func_short)  au_g72x_write_s ;
	psf->write_int    = (func_int)    au_g72x_write_i ;
	psf->write_float  = (func_float)  au_g72x_write_f ;
	psf->write_double = (func_double) au_g72x_write_d ;
 
 	psf->seek_func   = (func_seek)    au_g72x_seek ;
 	psf->close       = (func_close)   au_g72x_close ;
 
	psf->blockwidth = psf->bytewidth = 1 ;

	return 0 ;
} /* au_g72x_writer_init */



/*============================================================================================
** G721 Read Functions.
*/

static int
au_g72x_read_block (SF_PRIVATE *psf, G72x_DATA *pg72x)
{	int	k ;
	
	pg72x->blockcount ++ ;
	pg72x->samplecount = 0 ;
	
	if (pg72x->samplecount > pg72x->blocksize)
	{	memset (pg72x->samples, 0, G72x_BLOCK_SIZE * sizeof (short)) ;
		return 1 ;
		} ;

	if ((k = fread (pg72x->block, 1, pg72x->bytesperblock, psf->file)) != pg72x->bytesperblock)
		psf_log_printf (psf, "*** Warning : short read (%d != %d).\n", k, pg72x->bytesperblock) ;

	pg72x->blocksize = k ;
	g72x_decode_block (pg72x) ;

	return 1 ;
} /* au_g72x_read_block */

static int 
au_g72x_read (SF_PRIVATE *psf, G72x_DATA *pg72x, short *ptr, int len)
{	int		count, total = 0, index = 0 ;

	while (index < len)
	{	if (pg72x->blockcount >= pg72x->blocks && pg72x->samplecount >= pg72x->samplesperblock)
		{	memset (&(ptr[index]), 0, (len - index) * sizeof (short)) ;
			return total ;
			} ;
		
		if (pg72x->samplecount >= pg72x->samplesperblock)
			au_g72x_read_block (psf, pg72x) ;
		
		count = pg72x->samplesperblock - pg72x->samplecount ;
		count = (len - index > count) ? count : len - index ;
		
		memcpy (&(ptr[index]), &(pg72x->samples [pg72x->samplecount]), count * sizeof (short)) ;
		index += count ;
		pg72x->samplecount += count ;
		total = index ;
		} ;

	return total ;		
} /* au_g72x_read */

static int		
au_g72x_read_s (SF_PRIVATE *psf, short *ptr, int len)
{	G72x_DATA 	*pg72x ; 
	int				total ;

	if (! psf->fdata)
		return 0 ;
	pg72x = (G72x_DATA*) psf->fdata ;
	
	total = au_g72x_read (psf, pg72x, ptr, len) ;

	return total ;
} /* au_g72x_read_s */

static int		
au_g72x_read_i  (SF_PRIVATE *psf, int *ptr, int len)
{	G72x_DATA *pg72x ; 
	short		*sptr ;
	int			k, bufferlen, readcount = 0, count ;
	int			index = 0, total = 0 ;

	if (! psf->fdata)
		return 0 ;
	pg72x = (G72x_DATA*) psf->fdata ;
	
	sptr = (short*) psf->buffer ;
	bufferlen = SF_BUFFER_LEN / sizeof (short) ;
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : len ;
		count = au_g72x_read (psf, pg72x, sptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [index+k] = (int) (sptr [k]) ;
		index += readcount ;
		total += count ;
		len -= readcount ;
		} ;
	return total ;
} /* au_g72x_read_i */

static int
au_g72x_read_f  (SF_PRIVATE *psf, float *ptr, int len)
{	G72x_DATA *pg72x ; 
	short		*sptr ;
	int			k, bufferlen, readcount = 0, count ;
	int			index = 0, total = 0 ;
	float 		normfact ;

	if (! psf->fdata)
		return 0 ;
	pg72x = (G72x_DATA*) psf->fdata ;
	
	normfact = (psf->norm_float == SF_TRUE) ? 1.0 / ((float) 0x8000) : 1.0 ;

	sptr = (short*) psf->buffer ;
	bufferlen = SF_BUFFER_LEN / sizeof (short) ;
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : len ;
		count = au_g72x_read (psf, pg72x, sptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [index+k] = normfact * sptr [k] ;
		index += readcount ;
		total += count ;
		len -= readcount ;
		} ;

	return total ;
} /* au_g72x_read_f */

static int		
au_g72x_read_d  (SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	G72x_DATA *pg72x ; 
	short		*sptr ;
	int			k, bufferlen, readcount = 0, count ;
	int			index = 0, total = 0 ;
	double		normfact ;
	
	if (! psf->fdata)
		return 0 ;
	pg72x = (G72x_DATA*) psf->fdata ;
	
	normfact = (normalize) ? 1.0 / ((double) 0x8000) : 1.0 ;

	sptr = (short*) psf->buffer ;
	bufferlen = SF_BUFFER_LEN / sizeof (short) ;
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : len ;
		count = au_g72x_read (psf, pg72x, sptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [index+k] = normfact * (double) (sptr [k]) ;
		index += readcount ;
		total += count ;
		len -= readcount ;
		} ;

	return total ;
} /* au_g72x_read_d */

static long    
au_g72x_seek   (SF_PRIVATE *psf, long offset, int whence)
{

	/*	No simple solution. To do properly, would need to seek
	**	to start of file and decode everything up to seek position.
	**	Maybe implement SEEK_SET to 0 only?
	*/
	return 0 ;
	
/*
**		G72x_DATA	*pg72x ; 
**		int			newblock, newsample, samplecount ;
**	
**		if (! psf->fdata)
**			return 0 ;
**		pg72x = (G72x_DATA*) psf->fdata ;
**	
**		if (! (psf->datalength && psf->dataoffset))
**		{	psf->error = SFE_BAD_SEEK ;
**			return	((long) -1) ;
**			} ;
**			
**		samplecount = (8 * psf->datalength) / G721_32_BITS_PER_SAMPLE ;
**			
**		switch (whence)
**		{	case SEEK_SET :
**					if (offset < 0 || offset > samplecount)
**					{	psf->error = SFE_BAD_SEEK ;
**						return	((long) -1) ;
**						} ;
**					newblock  = offset / pg72x->samplesperblock ;
**					newsample = offset % pg72x->samplesperblock ;
**					break ;
**					
**			case SEEK_CUR :
**					if (psf->current + offset < 0 || psf->current + offset > samplecount)
**					{	psf->error = SFE_BAD_SEEK ;
**						return	((long) -1) ;
**						} ;
**					newblock  = (8 * (psf->current + offset)) / pg72x->samplesperblock ;
**					newsample = (8 * (psf->current + offset)) % pg72x->samplesperblock ;
**					break ;
**					
**			case SEEK_END :
**					if (offset > 0 || samplecount + offset < 0)
**					{	psf->error = SFE_BAD_SEEK ;
**						return	((long) -1) ;
**						} ;
**					newblock  = (samplecount + offset) / pg72x->samplesperblock ;
**					newsample = (samplecount + offset) % pg72x->samplesperblock ;
**					break ;
**					
**			default : 
**					psf->error = SFE_BAD_SEEK ;
**					return	((long) -1) ;
**			} ;
**			
**		if (psf->mode == SF_MODE_READ)
**		{	fseek (psf->file, (int) (psf->dataoffset + newblock * pg72x->blocksize), SEEK_SET) ;
**			pg72x->blockcount  = newblock ;
**			au_g72x_read_block (psf, pg72x) ;
**			pg72x->samplecount = newsample ;
**			}
**		else
**		{	/+* What to do about write??? *+/ 
**			psf->error = SFE_BAD_SEEK ;
**			return	((long) -1) ;
**			} ;
**	
**		psf->current = newblock * pg72x->samplesperblock + newsample ;
**		return psf->current ;
**	
*/
} /* au_g72x_seek */

/*==========================================================================================
** G72x Write Functions.
*/

static int		
au_g72x_write_block (SF_PRIVATE *psf, G72x_DATA *pg72x)
{	int k ;

	/* Encode the samples. */
	g72x_encode_block (pg72x) ;

	/* Write the block to disk. */
	if ((k = fwrite (pg72x->block, 1, pg72x->blocksize, psf->file)) != pg72x->blocksize)
		psf_log_printf (psf, "*** Warning : short write (%d != %d).\n", k, pg72x->blocksize) ;

	pg72x->samplecount = 0 ;
	pg72x->blockcount ++ ;

	/* Set samples to zero for next block. */
	memset (pg72x->samples, 0, G72x_BLOCK_SIZE * sizeof (short)) ;
			
	return 1 ;
} /* au_g72x_write_block */

static int 
au_g72x_write (SF_PRIVATE *psf, G72x_DATA *pg72x, short *ptr, int len)
{	int		count, total = 0, index = 0 ;
	
	while (index < len)
	{	count = pg72x->samplesperblock - pg72x->samplecount ;

		if (count > len - index)
			count = len - index ;

		memcpy (&(pg72x->samples [pg72x->samplecount]), &(ptr [index]), count * sizeof (short)) ;
		index += count ;
		pg72x->samplecount += count ;
		total = index ;

		if (pg72x->samplecount >= pg72x->samplesperblock)
			au_g72x_write_block (psf, pg72x) ;	
		} ;

	return total ;		
} /* au_g72x_write */

static int		
au_g72x_write_s (SF_PRIVATE *psf, short *ptr, int len)
{	G72x_DATA 	*pg72x ; 
	int				total ;

	if (! psf->fdata)
		return 0 ;
	pg72x = (G72x_DATA*) psf->fdata ;
	
	total = au_g72x_write (psf, pg72x, ptr, len) ;

	return total ;
} /* au_g72x_write_s */

static int		
au_g72x_write_i  (SF_PRIVATE *psf, int *ptr, int len)
{	G72x_DATA *pg72x ; 
	short		*sptr ;
	int			k, bufferlen, writecount = 0, count ;
	int			index = 0, total = 0 ;

	if (! psf->fdata)
		return 0 ;
	pg72x = (G72x_DATA*) psf->fdata ;
	
	sptr = (short*) psf->buffer ;
	bufferlen = ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (short) ;
	while (len > 0)
	{	writecount = (len >= bufferlen) ? bufferlen : len ;
		for (k = 0 ; k < writecount ; k++)
			sptr [k] = ptr [index+k] ;
		count = au_g72x_write (psf, pg72x, sptr, writecount) ;
		index += writecount ;
		total += count ;
		len -= writecount ;
		} ;
	return total ;
} /* au_g72x_write_i */

static int		
au_g72x_write_f  (SF_PRIVATE *psf, float *ptr, int len)
{	G72x_DATA *pg72x ; 
	short		*sptr ;
	int			k, bufferlen, writecount = 0, count ;
	int			index = 0, total = 0 ;
	float		normfact ;
	
	if (! psf->fdata)
		return 0 ;
	pg72x = (G72x_DATA*) psf->fdata ;
	
	normfact = (psf->norm_float == SF_TRUE) ? ((float) 0x8000) : 1.0 ;

	sptr = (short*) psf->buffer ;
	bufferlen = ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (short) ;
	while (len > 0)
	{	writecount = (len >= bufferlen) ? bufferlen : len ;
		for (k = 0 ; k < writecount ; k++)
			sptr [k] = FLOAT_TO_SHORT (normfact * ptr [index+k])  ;
		count = au_g72x_write (psf, pg72x, sptr, writecount) ;
		index += writecount ;
		total += count ;
		len -= writecount ;
		} ;
	return total ;
} /* au_g72x_write_f */

static int		
au_g72x_write_d  (SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	G72x_DATA *pg72x ; 
	short		*sptr ;
	int			k, bufferlen, writecount = 0, count ;
	int			index = 0, total = 0 ;
	double		normfact ;

	if (! psf->fdata)
		return 0 ;
	pg72x = (G72x_DATA*) psf->fdata ;
	
	normfact = (normalize) ? ((float) 0x8000) : 1.0 ;

	sptr = (short*) psf->buffer ;
	bufferlen = ((SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth) / sizeof (short) ;
	while (len > 0)
	{	writecount = (len >= bufferlen) ? bufferlen : len ;
		for (k = 0 ; k < writecount ; k++)
			sptr [k] = DOUBLE_TO_SHORT (normfact * ptr [index+k])  ;
		count = au_g72x_write (psf, pg72x, sptr, writecount) ;
		index += writecount ;
		total += count ;
		len -= writecount ;
		} ;
	return total ;
} /* au_g72x_write_d */

static int	
au_g72x_close	(SF_PRIVATE  *psf)
{	G72x_DATA *pg72x ; 

	if (! psf->fdata)
		return 0 ;

	pg72x = (G72x_DATA*) psf->fdata ;

	if (psf->mode == SF_MODE_WRITE)
	{	/*	If a block has been partially assembled, write it out
		**	as the final block.
		*/
	
		if (pg72x->samplecount && pg72x->samplecount < G72x_BLOCK_SIZE)
			au_g72x_write_block (psf, pg72x) ;	

		/*  Now we know for certain the length of the file we can
		**  re-write correct values for the RIFF and data chunks.
		*/
		
		fseek (psf->file, 0, SEEK_END) ;
		psf->filelength = ftell (psf->file) ;

		psf->sf.samples = pg72x->samplesperblock * pg72x->blockcount ;
		psf->datalength = psf->filelength - psf->dataoffset ;

		if (psf->write_header)
			psf->write_header (psf) ; 
		} ;

	if (psf->fdata)
		free (psf->fdata) ;
	psf->fdata = NULL ;

	return 0 ;
} /* au_g72x_close */

