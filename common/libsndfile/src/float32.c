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
#include	"common.h"

/*--------------------------------------------------------------------------------------------
*/

/*--------------------------------------------------------------------------------------------
**	Prototypes for private functions.
*/

static int		host_read_f2s   (SF_PRIVATE *psf, short *ptr, int len) ;
static int		host_read_f2i   (SF_PRIVATE *psf, int *ptr, int len) ;
static int		host_read_f     (SF_PRIVATE *psf, float *ptr, int len) ;
static int		host_read_f2d   (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;

static int		host_write_s2f   (SF_PRIVATE *psf, short *ptr, int len) ;
static int		host_write_i2f   (SF_PRIVATE *psf, int *ptr, int len) ;
static int		host_write_f     (SF_PRIVATE *psf, float *ptr, int len) ;
static int		host_write_d2f   (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;

static	void	f2s_array 	(float *buffer, unsigned int count, short *ptr, int index) ;
static	void	f2i_array 	(float *buffer, unsigned int count, int *ptr, int index) ;
static	void	f2d_array 	(float *buffer, unsigned int count, double *ptr, int index) ;

static 	void	s2f_array 	(short *ptr, int index, float *buffer, unsigned int count) ;
static 	void	i2f_array 	(int *ptr, int index, float *buffer, unsigned int count) ;
static 	void	d2f_array 	(double *ptr, int index, float *buffer, unsigned int count) ;

static void		float32_peak_update (SF_PRIVATE *psf, float *buffer, int count, int index) ;

static int		broken_read_f2s (SF_PRIVATE *psf, short *ptr, int len) ;
static int		broken_read_f2i (SF_PRIVATE *psf, int *ptr, int len) ;
static int		broken_read_f   (SF_PRIVATE *psf, float *ptr, int len) ;
static int		broken_read_f2d (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;

static int		broken_write_s2f (SF_PRIVATE *psf, short *ptr, int len) ;
static int		broken_write_i2f (SF_PRIVATE *psf, int *ptr, int len) ;
static int		broken_write_f   (SF_PRIVATE *psf, float *ptr, int len) ;
static int		broken_write_d2f (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;

static	void	bf2f_array (float *buffer, unsigned int count) ;
static	void	f2bf_array (float *buffer, unsigned int count) ;

/*--------------------------------------------------------------------------------------------
**	Exported functions.
*/

int
float32_read_init (SF_PRIVATE *psf)
{	static int float_caps = FLOAT_UNKNOWN ;

	if (float_caps == FLOAT_UNKNOWN)
		float_caps = float32_get_capability (psf->endian) ;
	
	switch (psf->endian + 0x1000 * float_caps)
	{	case (SF_ENDIAN_BIG + 0x1000 * FLOAT_CAN_RW_BE) :
				psf->fl32_endswap = SF_FALSE ;
				psf->read_short  = (func_short)  host_read_f2s ;
				psf->read_int    = (func_int)    host_read_f2i ;
				psf->read_float  = (func_float)  host_read_f ;
				psf->read_double = (func_double) host_read_f2d ;
				break ;
				
		case (SF_ENDIAN_LITTLE + 0x1000 * FLOAT_CAN_RW_LE) :
				psf->fl32_endswap = SF_FALSE ;
				psf->read_short  = (func_short)  host_read_f2s ;
				psf->read_int    = (func_int)    host_read_f2i ;
				psf->read_float  = (func_float)  host_read_f ;
				psf->read_double = (func_double) host_read_f2d ;
				break ;

		case (SF_ENDIAN_BIG + 0x1000 * FLOAT_CAN_RW_LE) :
				psf->fl32_endswap = SF_TRUE ;
				psf->read_short  = (func_short)  host_read_f2s ;
				psf->read_int    = (func_int)    host_read_f2i ;
				psf->read_float  = (func_float)  host_read_f ;
				psf->read_double = (func_double) host_read_f2d ;
				break ;
				
		case (SF_ENDIAN_LITTLE + 0x1000 * FLOAT_CAN_RW_BE) :
				psf->fl32_endswap = SF_TRUE ;
				psf->read_short  = (func_short)  host_read_f2s ;
				psf->read_int    = (func_int)    host_read_f2i ;
				psf->read_float  = (func_float)  host_read_f ;
				psf->read_double = (func_double) host_read_f2d ;
				break ;
				
		case (SF_ENDIAN_BIG + 0x1000 * FLOAT_BROKEN_LE) :
				psf->fl32_endswap = SF_TRUE ;
				psf->read_short  = (func_short)  broken_read_f2s ;
				psf->read_int    = (func_int)    broken_read_f2i ;
				psf->read_float  = (func_float)  broken_read_f ;
				psf->read_double = (func_double) broken_read_f2d ;
				break ;
				
		case (SF_ENDIAN_LITTLE + 0x1000 * FLOAT_BROKEN_LE) :
				psf->fl32_endswap = SF_FALSE ;
				psf->read_short  = (func_short)  broken_read_f2s ;
				psf->read_int    = (func_int)    broken_read_f2i ;
				psf->read_float  = (func_float)  broken_read_f ;
				psf->read_double = (func_double) broken_read_f2d ;
				break ;
				
		case (SF_ENDIAN_BIG + 0x1000 * FLOAT_BROKEN_BE) :
				psf->fl32_endswap = SF_FALSE ;
				psf->read_short  = (func_short)  broken_read_f2s ;
				psf->read_int    = (func_int)    broken_read_f2i ;
				psf->read_float  = (func_float)  broken_read_f ;
				psf->read_double = (func_double) broken_read_f2d ;
				break ;
				
		case (SF_ENDIAN_LITTLE + 0x1000 * FLOAT_BROKEN_BE) :
				psf->fl32_endswap = SF_TRUE ;
				psf->read_short  = (func_short)  broken_read_f2s ;
				psf->read_int    = (func_int)    broken_read_f2i ;
				psf->read_float  = (func_float)  broken_read_f ;
				psf->read_double = (func_double) broken_read_f2d ;
				break ;
				
		default : break ;
		} ;

	return 0 ;
} /* float32_read_init */

int
float32_write_init (SF_PRIVATE *psf)
{	static int float_caps = FLOAT_UNKNOWN ;

	if (float_caps == FLOAT_UNKNOWN)
		float_caps = float32_get_capability (psf->endian) ;
	
	switch (psf->endian + 0x1000 * float_caps)
	{	case (SF_ENDIAN_LITTLE + 0x1000 * FLOAT_CAN_RW_LE) :
				psf->fl32_endswap = SF_FALSE ;
				psf->write_short  = (func_short)  host_write_s2f ;
				psf->write_int    = (func_int)    host_write_i2f ;
				psf->write_float  = (func_float)  host_write_f ;
				psf->write_double = (func_double) host_write_d2f ;
				break ;

		case (SF_ENDIAN_BIG + 0x1000 * FLOAT_CAN_RW_BE) :
				psf->fl32_endswap = SF_FALSE ;
				psf->write_short  = (func_short)  host_write_s2f ;
				psf->write_int    = (func_int)    host_write_i2f ;
				psf->write_float  = (func_float)  host_write_f ;
				psf->write_double = (func_double) host_write_d2f ;
				break ;
				
		case (SF_ENDIAN_BIG + 0x1000 * FLOAT_CAN_RW_LE) :
				psf->fl32_endswap = SF_TRUE ;
				psf->write_short  = (func_short)  host_write_s2f ;
				psf->write_int    = (func_int)    host_write_i2f ;
				psf->write_float  = (func_float)  host_write_f ;
				psf->write_double = (func_double) host_write_d2f ;
				break ;
				
		case (SF_ENDIAN_LITTLE + 0x1000 * FLOAT_CAN_RW_BE) :
				psf->fl32_endswap = SF_TRUE ;
				psf->write_short  = (func_short)  host_write_s2f ;
				psf->write_int    = (func_int)    host_write_i2f ;
				psf->write_float  = (func_float)  host_write_f ;
				psf->write_double = (func_double) host_write_d2f ;
				break ;
				
		case (SF_ENDIAN_BIG + 0x1000 * FLOAT_BROKEN_LE) :
				psf->fl32_endswap = SF_TRUE ;
				psf->write_short  = (func_short)  broken_write_s2f ;
				psf->write_int    = (func_int)    broken_write_i2f ;
				psf->write_float  = (func_float)  broken_write_f ;
				psf->write_double = (func_double) broken_write_d2f ;
				break ;
				
		case (SF_ENDIAN_LITTLE + 0x1000 * FLOAT_BROKEN_LE) :
				psf->fl32_endswap = SF_FALSE ;
				psf->write_short  = (func_short)  broken_write_s2f ;
				psf->write_int    = (func_int)    broken_write_i2f ;
				psf->write_float  = (func_float)  broken_write_f ;
				psf->write_double = (func_double) broken_write_d2f ;
				break ;
				
		case (SF_ENDIAN_BIG + 0x1000 * FLOAT_BROKEN_BE) :
				psf->fl32_endswap = SF_FALSE ;
				psf->write_short  = (func_short)  broken_write_s2f ;
				psf->write_int    = (func_int)    broken_write_i2f ;
				psf->write_float  = (func_float)  broken_write_f ;
				psf->write_double = (func_double) broken_write_d2f ;
				break ;
				
		case (SF_ENDIAN_LITTLE + 0x1000 * FLOAT_BROKEN_BE) :
				psf->fl32_endswap = SF_TRUE ;
				psf->write_short  = (func_short)  broken_write_s2f ;
				psf->write_int    = (func_int)    broken_write_i2f ;
				psf->write_float  = (func_float)  broken_write_f ;
				psf->write_double = (func_double) broken_write_d2f ;
				break ;
				
		default : break ;
		} ;

	return 0 ;
} /* float32_write_init */	

float	
float32_read (unsigned char *cptr)
{	int		exponent, mantissa, negative ;
	float	fvalue ;

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
} /* float32_read */

void	
float32_write (float in, unsigned char *out)
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
} /* float32_write */

/*==============================================================================================
**	Private functions.
*/

static void
float32_peak_update (SF_PRIVATE *psf, float *buffer, int count, int index)
{	int		k, chan, position ;
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
			psf->peak.peak[chan].position = psf->current + index + (position /psf->sf.channels) ;
			} ;
		} ;

	return ;	
} /* float32_peak_update */

int
float32_get_capability (int endianness)
{	union 
	{	float			f ;
		int				i ;
		unsigned char	c [4] ;
	} data ;

	data.f = 1.23456789 ; /* Some abitrary value. */
	
	if (FORCE_BROKEN_FLOAT || data.i != 0x3f9e0652)
		return (CPU_IS_LITTLE_ENDIAN) ? FLOAT_BROKEN_LE : FLOAT_BROKEN_BE ;

	/* If this test is true ints and floats are compatible and little endian. */
	if (data.c [0] == 0x52 && data.c [1] == 0x06 && data.c [2] == 0x9e && data.c [3] == 0x3f)
		return FLOAT_CAN_RW_LE ;

	/* If this test is true ints and floats are compatible and big endian. */
	if (data.c [3] == 0x52 && data.c [2] == 0x06 && data.c [1] == 0x9e && data.c [0] == 0x3f)
		return FLOAT_CAN_RW_BE ;
		
	/* Floats are broken. Don't expect reading or writing to be fast. */
	return 0 ;
} /* float32_get_capability */

/*----------------------------------------------------------------------------------------------
*/


static int
host_read_f2s (SF_PRIVATE *psf, short *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		
		if (psf->fl32_endswap == SF_TRUE)
			endswap_int_array ((int*) psf->buffer, readcount / sizeof (int)) ;

		f2s_array ((float*) (psf->buffer), thisread / psf->bytewidth, ptr, index) ;
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
} /* host_read_f2s */

static int
host_read_f2i (SF_PRIVATE *psf, int *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		
		if (psf->fl32_endswap == SF_TRUE)
			endswap_int_array ((int*) psf->buffer, readcount / sizeof (int)) ;

		f2i_array ((float*) (psf->buffer), thisread / psf->bytewidth, ptr, index) ;
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
} /* host_read_f2i */

static int
host_read_f (SF_PRIVATE *psf, float *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int	index = 0, total = 0 ;
	
	if (psf->fl32_endswap != SF_TRUE)
		return fread (ptr, sizeof (float), len, psf->file) ; 
	
	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		
		endswap_int_array ((int*) psf->buffer, readcount / sizeof (int)) ;

		memcpy (ptr + index, psf->buffer, thisread) ;

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
} /* host_read_f */

static int
host_read_f2d (SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int	index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		
		if (psf->fl32_endswap == SF_TRUE)
			endswap_int_array ((int*) psf->buffer, readcount / sizeof (int)) ;

		f2d_array ((float*) (psf->buffer), thisread / psf->bytewidth, ptr, index) ;
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
} /* host_read_f2d */

static int
host_write_s2f	(SF_PRIVATE *psf, short *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		s2f_array (ptr, index, (float*) (psf->buffer), writecount / psf->bytewidth) ;
		
		float32_peak_update (psf, (float *) (psf->buffer), writecount / psf->bytewidth, index / psf->sf.channels) ;
		
		if (psf->fl32_endswap == SF_TRUE)
			endswap_int_array ((int*) psf->buffer, writecount / sizeof (int)) ;
			
		thiswrite = fwrite (psf->buffer, 1, writecount, psf->file) ;
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
} /* host_write_s2f */

static int
host_write_i2f	(SF_PRIVATE *psf, int *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		i2f_array (ptr, index, (float*) (psf->buffer), writecount / psf->bytewidth) ;
		
		float32_peak_update (psf, (float *) (psf->buffer), writecount / psf->bytewidth, index / psf->sf.channels) ;
		
		if (psf->fl32_endswap == SF_TRUE)
			endswap_int_array ((int*) psf->buffer, writecount / sizeof (int)) ;
			
		thiswrite = fwrite (psf->buffer, 1, writecount, psf->file) ;
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
} /* host_write_i2f */

static int
host_write_f (SF_PRIVATE *psf, float *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	float32_peak_update (psf, ptr, len, 0) ;
			
	if (psf->fl32_endswap != SF_TRUE)
		return fwrite (ptr, sizeof (float), len, psf->file) ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;

		memcpy (psf->buffer, ptr + index, writecount) ;
		
		if (psf->fl32_endswap == SF_TRUE)
			endswap_int_array ((int*) psf->buffer, writecount / sizeof (int)) ;
			
		thiswrite = fwrite (psf->buffer, 1, writecount, psf->file) ;
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
} /* host_write_f */

static int
host_write_d2f	(SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		d2f_array (ptr, index, (float*) (psf->buffer), writecount / psf->bytewidth) ;
		
		float32_peak_update (psf, (float *) (psf->buffer), writecount / psf->bytewidth, index / psf->sf.channels) ;
		
		if (psf->fl32_endswap == SF_TRUE)
			endswap_int_array ((int*) psf->buffer, writecount / sizeof (int)) ;
			
		thiswrite = fwrite (psf->buffer, 1, writecount, psf->file) ;
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
} /* host_write_d2f */

/*=======================================================================================
*/

static void	
f2s_array (float *buffer, unsigned int count, short *ptr, int index)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	ptr [index] = ((double) buffer [k]) ;
		index ++ ;
		} ;
} /* f2s_array */

static void	
f2i_array (float *buffer, unsigned int count, int *ptr, int index)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	ptr [index] = (int) ((double) buffer [k]) ;
		index ++ ;
		} ;
} /* f2i_array */

static void	
f2d_array (float *buffer, unsigned int count, double *ptr, int index)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	ptr [index] = ((double) buffer [k]) ;
		index ++ ;
		} ;
} /* f2d_array */

static  void	
s2f_array (short *ptr, int index, float *buffer, unsigned int count)
{	int		k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = (float) (ptr [index]) ;
		index ++ ;
		} ;
		
} /* s2f_array */

static void	
i2f_array (int *ptr, int index, float *buffer, unsigned int count)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = (float) (ptr [index]) ;
		index ++ ;
		} ;
} /* i2f_array */

static void	
d2f_array (double *ptr, int index, float *buffer, unsigned int count)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = (float) (ptr [index]) ;
		index ++ ;
		} ;
} /* d2f_array */

/*=======================================================================================
*/

static int		
broken_read_f2s (SF_PRIVATE *psf, short *ptr, int len)
{	unsigned int	bytecount, readcount, bufferlen, thisread ;
	int				index = 0, total = 0 ;
		
	bufferlen = (SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;

		if (psf->fl32_endswap == SF_TRUE)
			endswap_int_array ((int*) psf->buffer, readcount / sizeof (int)) ;

		bf2f_array ((float *) (psf->buffer), readcount / psf->bytewidth) ;

		f2s_array ((float*) (psf->buffer), thisread / psf->bytewidth, ptr, index) ;
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
} /* broken_read_f2s */

static int		
broken_read_f2i (SF_PRIVATE *psf, int *ptr, int len)
{	unsigned int	bytecount, readcount, bufferlen, thisread ;
	int				index = 0, total = 0 ;

	bufferlen = (SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;

		if (psf->fl32_endswap == SF_TRUE)
			endswap_int_array ((int*) psf->buffer, readcount / sizeof (int)) ;

		bf2f_array ((float *) (psf->buffer), readcount / psf->bytewidth) ;

		f2i_array ((float*) (psf->buffer), thisread / psf->bytewidth, ptr, index) ;
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
} /* broken_read_f2i */

static int		
broken_read_f (SF_PRIVATE *psf, float *ptr, int len)
{	unsigned int	bytecount, readcount, bufferlen, thisread ;
	int				index = 0, total = 0 ;
	
	/* FIX THIS */

	bufferlen = (SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;

		if (psf->fl32_endswap == SF_TRUE)
			endswap_int_array ((int*) psf->buffer, readcount / sizeof (int)) ;

		bf2f_array ((float *) (psf->buffer), readcount / psf->bytewidth) ;

		memcpy (ptr + index, psf->buffer, readcount) ;
		
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
} /* broken_read_f */

static int		
broken_read_f2d (SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	unsigned int	bytecount, readcount, bufferlen, thisread ;
	int				index = 0, total = 0 ;

	bufferlen = (SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;

		if (psf->fl32_endswap == SF_TRUE)
			endswap_int_array ((int*) psf->buffer, readcount / sizeof (int)) ;

		bf2f_array ((float *) (psf->buffer), readcount / psf->bytewidth) ;

		f2d_array ((float*) (psf->buffer), thisread / psf->bytewidth, ptr, index) ;
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
} /* broken_read_f2d */

static int	
broken_write_s2f (SF_PRIVATE *psf, short *ptr, int len)
{	unsigned int	bytecount, writecount, bufferlen, thiswrite ;
	int				index = 0, total = 0 ;

	bufferlen = (SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		s2f_array (ptr, index, (float*) (psf->buffer), writecount / psf->bytewidth) ;

		float32_peak_update (psf, (float *) (psf->buffer), writecount / psf->bytewidth, index / psf->sf.channels) ;

		f2bf_array ((float *) (psf->buffer), writecount / psf->bytewidth) ;

		if (psf->fl32_endswap == SF_TRUE)
			endswap_int_array ((int*) psf->buffer, writecount / sizeof (int)) ;

		thiswrite = fwrite (psf->buffer, 1, writecount, psf->file) ;
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
} /* broken_write_s2f */

static int	
broken_write_i2f (SF_PRIVATE *psf, int *ptr, int len) 
{	unsigned int	bytecount, writecount, bufferlen, thiswrite ;
	int				index = 0, total = 0 ;

	bufferlen = (SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		i2f_array (ptr, index, (float*) (psf->buffer), writecount / psf->bytewidth) ;

		float32_peak_update (psf, (float *) (psf->buffer), writecount / psf->bytewidth, index / psf->sf.channels) ;

		f2bf_array ((float *) (psf->buffer), writecount / psf->bytewidth) ;

		if (psf->fl32_endswap == SF_TRUE)
			endswap_int_array ((int*) psf->buffer, writecount / sizeof (int)) ;

		thiswrite = fwrite (psf->buffer, 1, writecount, psf->file) ;
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
} /* broken_write_i2f */

static int	
broken_write_f (SF_PRIVATE *psf, float *ptr, int len)
{	unsigned int	bytecount, writecount, bufferlen, thiswrite ;
	int				index = 0, total = 0 ;
	
	/* FIX THIS */
	float32_peak_update (psf, ptr, len, 0) ;

	bufferlen = (SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;

		memcpy (psf->buffer, ptr + index, writecount) ;

		f2bf_array ((float *) (psf->buffer), writecount / psf->bytewidth) ;
		
		if (psf->fl32_endswap == SF_TRUE)
			endswap_int_array ((int*) psf->buffer, writecount / sizeof (int)) ;

		thiswrite = fwrite (psf->buffer, 1, writecount, psf->file) ;
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
} /* broken_write_f */

static int	
broken_write_d2f (SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	unsigned int	bytecount, writecount, bufferlen, thiswrite ;
	int				index = 0, total = 0 ;

	bufferlen = (SF_BUFFER_LEN / psf->blockwidth) * psf->blockwidth ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		d2f_array (ptr, index, (float*) (psf->buffer), writecount / psf->bytewidth) ;

		float32_peak_update (psf, (float *) (psf->buffer), writecount / psf->bytewidth, index / psf->sf.channels) ;

		f2bf_array ((float *) (psf->buffer), writecount / psf->bytewidth) ;

		if (psf->fl32_endswap == SF_TRUE)
			endswap_int_array ((int*) psf->buffer, writecount / sizeof (int)) ;

		thiswrite = fwrite (psf->buffer, 1, writecount, psf->file) ;
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
} /* broken_write_d2f */

/*==============================================================================
*/

/*----------------------------------------------------------------------------------------------
*/

static void	
bf2f_array (float *buffer, unsigned int count)
{	int	k ;
	
	for (k = 0 ; k < count ; k++)
		buffer [k] = float32_read ((unsigned char *) (buffer + k)) ;
} /* bf2f_array */

static void	
f2bf_array (float *buffer, unsigned int count)
{	int	k ;
	
	for (k = 0 ; k < count ; k++)
		float32_write (buffer [k], (unsigned char*) (buffer + k)) ;
} /* f2bf_array */

