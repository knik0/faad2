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
#include	"common.h"
#include	"float_cast.h"

/*--------------------------------------------------------------------------------------------
**	Processor floating point capabilities. double64_get_capability () returns one of the 
**	latter three values.
*/

enum
{	DOUBLE_UNKNOWN		= 0x00,
	DOUBLE_CAN_RW_LE	= 0x23,
	DOUBLE_CAN_RW_BE	= 0x34,
	DOUBLE_BROKEN_LE	= 0x35,
	DOUBLE_BROKEN_BE	= 0x36
} ;

/*--------------------------------------------------------------------------------------------
**	Prototypes for private functions.
*/

static sf_count_t		host_read_d2s   (SF_PRIVATE *psf, short *ptr, sf_count_t len) ;
static sf_count_t		host_read_d2i   (SF_PRIVATE *psf, int *ptr, sf_count_t len) ;
static sf_count_t		host_read_d2f   (SF_PRIVATE *psf, float *ptr, sf_count_t len) ;
static sf_count_t		host_read_d     (SF_PRIVATE *psf, double *ptr, sf_count_t len) ;

static sf_count_t		host_write_s2d   (SF_PRIVATE *psf, short *ptr, sf_count_t len) ;
static sf_count_t		host_write_i2d   (SF_PRIVATE *psf, int *ptr, sf_count_t len) ;
static sf_count_t		host_write_f2d   (SF_PRIVATE *psf, float *ptr, sf_count_t len) ;
static sf_count_t		host_write_d     (SF_PRIVATE *psf, double *ptr, sf_count_t len) ;

static	void	d2s_array 	(double *buffer, unsigned int count, short *ptr) ;
static	void	d2i_array 	(double *buffer, unsigned int count, int *ptr) ;
static	void	d2f_array 	(double *buffer, unsigned int count, float *ptr) ;

static 	void	s2d_array 	(short *ptr, double *buffer, unsigned int count) ;
static 	void	i2d_array 	(int *ptr, double *buffer, unsigned int count) ;
static 	void	f2d_array 	(float *ptr, double *buffer, unsigned int count) ;

static void		double64_peak_update (SF_PRIVATE *psf, double *buffer, int count, int index) ;

static int		double64_get_capability (void) ;

/*--------------------------------------------------------------------------------------------
**	Exported functions.
*/

int
double64_init (SF_PRIVATE *psf)
{	static int double64_caps = DOUBLE_UNKNOWN ;

	if (double64_caps == DOUBLE_UNKNOWN)
		double64_caps = double64_get_capability () ;
	
	psf->blockwidth = sizeof (double) * psf->sf.channels ;

	if (psf->mode == SFM_READ || psf->mode == SFM_RDWR)
	{	switch (psf->endian + double64_caps)
		{	case (SF_ENDIAN_BIG + DOUBLE_CAN_RW_BE) :
					psf->float_endswap = SF_FALSE ;
					psf->read_short  = host_read_d2s ;
					psf->read_int    = host_read_d2i ;
					psf->read_float  = host_read_d2f ;
					psf->read_double = host_read_d ;
					break ;
					
			case (SF_ENDIAN_LITTLE + DOUBLE_CAN_RW_LE) :
					psf->float_endswap = SF_FALSE ;
					psf->read_short  = host_read_d2s ;
					psf->read_int    = host_read_d2i ;
					psf->read_float  = host_read_d2f ;
					psf->read_double = host_read_d ;
					break ;
	
			case (SF_ENDIAN_BIG + DOUBLE_CAN_RW_LE) :
					psf->float_endswap = SF_TRUE ;
					psf->read_short  = host_read_d2s ;
					psf->read_int    = host_read_d2i ;
					psf->read_float  = host_read_d2f ;
					psf->read_double = host_read_d ;
					break ;
					
			case (SF_ENDIAN_LITTLE + DOUBLE_CAN_RW_BE) :
					psf->float_endswap = SF_TRUE ;
					psf->read_short  = host_read_d2s ;
					psf->read_int    = host_read_d2i ;
					psf->read_float  = host_read_d2f ;
					psf->read_double = host_read_d ;
					break ;
					
			default : break ;
			} ;
		} ;

	if (psf->mode == SFM_WRITE || psf->mode == SFM_RDWR)
	{	switch (psf->endian + double64_caps)
		{	case (SF_ENDIAN_LITTLE + DOUBLE_CAN_RW_LE) :
					psf->float_endswap = SF_FALSE ;
					psf->write_short  = host_write_s2d ;
					psf->write_int    = host_write_i2d ;
					psf->write_float  = host_write_f2d ;
					psf->write_double = host_write_d ;
					break ;
	
			case (SF_ENDIAN_BIG + DOUBLE_CAN_RW_BE) :
					psf->float_endswap = SF_FALSE ;
					psf->write_short  = host_write_s2d ;
					psf->write_int    = host_write_i2d ;
					psf->write_float  = host_write_f2d ;
					psf->write_double = host_write_d ;
					break ;
					
			case (SF_ENDIAN_BIG + DOUBLE_CAN_RW_LE) :
					psf->float_endswap = SF_TRUE ;
					psf->write_short  = host_write_s2d ;
					psf->write_int    = host_write_i2d ;
					psf->write_float  = host_write_f2d ;
					psf->write_double = host_write_d ;
					break ;
					
			case (SF_ENDIAN_LITTLE + DOUBLE_CAN_RW_BE) :
					psf->float_endswap = SF_TRUE ;
					psf->write_short  = host_write_s2d ;
					psf->write_int    = host_write_i2d ;
					psf->write_float  = host_write_f2d ;
					psf->write_double = host_write_d ;
					break ;
					
			default : break ;
			} ;
		};

	psf->filelength = psf_get_filelen (psf->filedes) ;
	psf->datalength = (psf->dataend) ? psf->dataend - psf->dataoffset : 
							psf->filelength - psf->dataoffset ;
	psf->sf.samples = psf->datalength / (psf->sf.channels * sizeof (double)) ;

	return 0 ;
} /* double64_init */	

double
double64_read (unsigned char *cptr)
{	int		exponent, mantissa, negative ;
	double	fvalue ;

	if (CPU_IS_LITTLE_ENDIAN)
	{	negative = cptr [3] & 0x80 ;
		exponent = ((cptr [3] & 0x7F) << 1) | ((cptr [2] & 0x80) ? 1 : 0);
		mantissa = ((cptr [2] & 0x7F) << 16) | (cptr [1] << 8) | (cptr [0]) ;
		}
	else
	{	negative = cptr [0] & 0x80 ;
		exponent = ((cptr [0] & 0x7F) << 1) | ((cptr [1] & 0x80) ? 1 : 0);
		mantissa = ((cptr [1] & 0x7F) << 16) | (cptr [2] << 8) | (cptr [3]) ;
		} ;

	if (! (exponent || mantissa))
		return 0.0 ;

	mantissa |= 0x800000 ;
	exponent = exponent ? exponent - 127 : 0 ;
                
	fvalue = mantissa ? ((float) mantissa) / ((float) 0x800000) : 0.0 ;
                
	if (negative)
		fvalue *= -1 ;
                
	if (exponent > 0)
		fvalue *= (1 << exponent) ;
	else if (exponent < 0)
		fvalue /= (1 << abs (exponent)) ;

	return fvalue ;
} /* double64_read */

void	
double64_write (double in, unsigned char *out)
{	int		exponent, mantissa, negative = 0 ;

	*((int*) out) = 0 ;
	
	if (in == 0.0)
		return ;
	
	if (in < 0.0)
	{	in *= -1.0 ;
		negative = 1 ;
		} ;
		
	in = frexp (in, &exponent) ;
	
	exponent += 126 ;
	
	in *= (float) 0x1000000 ;
	mantissa = (((int) in) & 0x7FFFFF) ;

	if (CPU_IS_LITTLE_ENDIAN)	
	{	if (negative)
			out [3] |= 0x80 ;
			
		if (exponent & 0x01)
			out [2] |= 0x80 ;
	
		out [0]  = mantissa & 0xFF ;
		out [1]  = (mantissa >> 8) & 0xFF ;
		out [2] |= (mantissa >> 16) & 0x7F ;
		out [3] |= (exponent >> 1) & 0x7F ;
		}
	else
	{	if (negative)
			out [0] |= 0x80 ;
			
		if (exponent & 0x01)
			out [1] |= 0x80 ;
	
		out [3]  = mantissa & 0xFF ;
		out [2]  = (mantissa >> 8) & 0xFF ;
		out [1] |= (mantissa >> 16) & 0x7F ;
		out [0] |= (exponent >> 1) & 0x7F ;
		}
	
	return ;
} /* double64_write */

/*==============================================================================================
**	Private functions.
*/

static void
double64_peak_update (SF_PRIVATE *psf, double *buffer, int count, int index)
{	int 	chan ;
	int		k, position ;
	float	fmaxval;
	
	for (chan = 0 ; chan < psf->sf.channels ; chan++)
	{	fmaxval = fabs (buffer [chan]) ;
		position = 0 ;
		for (k = chan ; k < count ; k += psf->sf.channels)
			if (fmaxval < fabs (buffer [k]))
			{	fmaxval = fabs (buffer [k]) ;
				position = k ;
				} ;
				
		if (fmaxval > psf->peak.peak[chan].value)
		{	psf->peak.peak[chan].value = fmaxval ;
			psf->peak.peak[chan].position = psf->write_current + index + (position /psf->sf.channels) ;
			} ;
		} ;

	return ;	
} /* double64_peak_update */

static int
double64_get_capability (void)
{	union 
	{	double			d ;
		int				i [2] ;
		unsigned char	c [8] ;
	} data ;

	data.d = 1.234567890123456789 ; /* Some abitrary value. */
	
	if (FORCE_BROKEN_FLOAT)
		return (CPU_IS_LITTLE_ENDIAN) ? DOUBLE_BROKEN_LE : DOUBLE_BROKEN_BE ;

	/* If this test is true ints and floats are compatible and little endian. */
	if (data.i [0] == 0x428c59fb && data.i [1] == 0x3ff3c0ca && 
		data.c [0] == 0xfb && data.c [2] == 0x8c && data.c [4] == 0xca && data.c [6] == 0xf3)
		return DOUBLE_CAN_RW_LE ;

	/* If this test is true ints and floats are compatible and big endian. */
	if (data.i [0] == 0x3ff3c0ca && data.i [1] == 0x428c59fb &&
		data.c [0] == 0x3f && data.c [2] == 0xc0 && data.c [4] == 0x42 && data.c [6] == 0x59)
		return DOUBLE_CAN_RW_BE ;
		
	/* Doubles are broken. Don't expect reading or writing to be fast. */
	return (CPU_IS_LITTLE_ENDIAN) ? DOUBLE_BROKEN_LE : DOUBLE_BROKEN_BE ;
} /* double64_get_capability */

/*----------------------------------------------------------------------------------------------
*/


static sf_count_t
host_read_d2s (SF_PRIVATE *psf, short *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		
		if (psf->float_endswap == SF_TRUE)
			endswap_long_array ((long*) psf->buffer, readcount / sizeof (int)) ;

		d2s_array ((double*) (psf->buffer), thisread / psf->bytewidth, ptr + index) ;
		total += thisread ;
		if (thisread < readcount)
			break ;
		index += thisread / psf->bytewidth ;
		bytecount -= thisread ;
		} ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_READ ;
	
	return total ;
} /* host_read_d2s */

static sf_count_t
host_read_d2i (SF_PRIVATE *psf, int *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		
		if (psf->float_endswap == SF_TRUE)
			endswap_long_array ((long*) psf->buffer, readcount / sizeof (int)) ;

		d2i_array ((double*) (psf->buffer), thisread / psf->bytewidth, ptr + index) ;
		total += thisread ;
		if (thisread < readcount)
			break ;
		index += thisread / psf->bytewidth ;
		bytecount -= thisread ;
		} ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_READ ;
	
	return total ;
} /* host_read_d2i */

static sf_count_t
host_read_d2f (SF_PRIVATE *psf, float *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int	index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		
		if (psf->float_endswap == SF_TRUE)
			endswap_long_array ((long*) psf->buffer, readcount / sizeof (int)) ;

		d2f_array ((double*) (psf->buffer), thisread / psf->bytewidth, ptr + index) ;
		total += thisread ;
		if (thisread < readcount)
			break ;
		index += thisread / psf->bytewidth ;
		bytecount -= thisread ;
		} ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_READ ;
	
	return total ;
} /* host_read_d2f */

static sf_count_t
host_read_d (SF_PRIVATE *psf, double *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int	index = 0, total = 0 ;
	
	if (psf->float_endswap != SF_TRUE)
		total = psf_fread (ptr, sizeof (double), len, psf->filedes) ; 
	else
	{	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
		bytecount = len * psf->bytewidth ;
		while (bytecount > 0)
		{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
			thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
			
			endswap_long_array ((long*) psf->buffer, readcount / sizeof (int)) ;
	
			memcpy (ptr + index, psf->buffer, thisread) ;
	
			total += thisread ;
			if (thisread < readcount)
				break ;
			index += thisread / psf->bytewidth ;
			bytecount -= thisread ;
			} ;
		total /= psf->bytewidth ;
		} ;

	if (total < len)
		psf->error = SFE_SHORT_READ ;
	
	return total ;
} /* host_read_d */

static sf_count_t
host_write_s2d	(SF_PRIVATE *psf, short *ptr, sf_count_t len)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		s2d_array (ptr + index, (double*) (psf->buffer), writecount / psf->bytewidth) ;
		
		double64_peak_update (psf, (double*) (psf->buffer), writecount / psf->bytewidth, index / psf->sf.channels) ;
		
		if (psf->float_endswap == SF_TRUE)
			endswap_long_array ((long*) psf->buffer, writecount / sizeof (int)) ;
			
		thiswrite = psf_fwrite (psf->buffer, 1, writecount, psf->filedes) ;
		total += thiswrite ;
		if (thiswrite < writecount)
			break ;
		index += thiswrite / psf->bytewidth ;
		bytecount -= thiswrite ;
		} ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_WRITE ;
	
	return total ;
} /* host_write_s2d */

static sf_count_t
host_write_i2d	(SF_PRIVATE *psf, int *ptr, sf_count_t len)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		i2d_array (ptr + index, (double*) (psf->buffer), writecount / psf->bytewidth) ;
		
		double64_peak_update (psf, (double*) (psf->buffer), writecount / psf->bytewidth, index / psf->sf.channels) ;
		
		if (psf->float_endswap == SF_TRUE)
			endswap_long_array ((long*) psf->buffer, writecount / sizeof (int)) ;
			
		thiswrite = psf_fwrite (psf->buffer, 1, writecount, psf->filedes) ;
		total += thiswrite ;
		if (thiswrite < writecount)
			break ;
		index += thiswrite / psf->bytewidth ;
		bytecount -= thiswrite ;
		} ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_WRITE ;
	
	return total ;
} /* host_write_i2d */

static sf_count_t
host_write_f2d	(SF_PRIVATE *psf, float *ptr, sf_count_t len)
{	unsigned int	writecount, thiswrite ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		f2d_array (ptr + index, (double*) (psf->buffer), writecount / psf->bytewidth) ;
		
		double64_peak_update (psf, (double*) (psf->buffer), writecount / psf->bytewidth, index / psf->sf.channels) ;
		
		if (psf->float_endswap == SF_TRUE)
			endswap_long_array ((long*) psf->buffer, writecount / sizeof (int)) ;
			
		thiswrite = psf_fwrite (psf->buffer, 1, writecount, psf->filedes) ;
		total += thiswrite ;
		if (thiswrite < writecount)
			break ;
		index += thiswrite / psf->bytewidth ;
		bytecount -= thiswrite ;
		} ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_WRITE ;
	
	return total ;
} /* host_write_f2d */

static sf_count_t
host_write_d (SF_PRIVATE *psf, double *ptr, sf_count_t len)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	double64_peak_update (psf, ptr, len, 0) ;
			
	if (psf->float_endswap != SF_TRUE)
		total = psf_fwrite (ptr, sizeof (double), len, psf->filedes) ; 
	else
	{	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
		bytecount = len * psf->bytewidth ;
		while (bytecount > 0)
		{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
	
			memcpy (psf->buffer, ptr + index, writecount) ;
			
			if (psf->float_endswap == SF_TRUE)
				endswap_long_array ((long*) psf->buffer, writecount / sizeof (int)) ;
				
			thiswrite = psf_fwrite (psf->buffer, 1, writecount, psf->filedes) ;
			total += thiswrite ;
			if (thiswrite < writecount)
				break ;
			index += thiswrite / psf->bytewidth ;
			bytecount -= thiswrite ;
			} ;
		total /= psf->bytewidth ;
		} ;

	if (total < len)
		psf->error = SFE_SHORT_WRITE ;
	
	return total ;
} /* host_write_d */

/*=======================================================================================
*/

static void	
d2s_array (double *buffer, unsigned int count, short *ptr)
{	while (count)
	{	count -- ;
		ptr [count] = lrint (buffer [count]) ;
		} ;
} /* d2s_array */

static void	
d2i_array (double *buffer, unsigned int count, int *ptr)
{	while (count)
	{	count -- ;
		ptr [count] = lrint (buffer [count]) ;
		} ;
} /* d2i_array */

static void	
d2f_array (double *buffer, unsigned int count, float *ptr)
{	while (count)
	{	count -- ;
		ptr [count] = buffer [count] ;
		} ;
} /* d2f_array */

static  void	
s2d_array (short *ptr, double *buffer, unsigned int count)
{	while (count)
	{	count -- ;
		buffer [count] = ptr [count] ;
		} ;
		
} /* s2d_array */

static void	
i2d_array (int *ptr, double *buffer, unsigned int count)
{	while (count)
	{	count -- ;
		buffer [count] = ptr [count] ;
		} ;
} /* i2d_array */

static void	
f2d_array (float *ptr, double *buffer, unsigned int count)
{	while (count)
	{	count -- ;
		buffer [count] = ptr [count] ;
		} ;
} /* f2d_array */

/*=======================================================================================
*/

