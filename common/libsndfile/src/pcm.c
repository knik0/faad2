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


#include	<unistd.h>

#include	"config.h"
#include	"sndfile.h"
#include	"sfendian.h"
#include	"floatcast.h"
#include	"common.h"

/* Important!!! Do not assume that sizeof (tribyte) == 3. Some compilers 
** (Metrowerks CodeWarrior for Mac is one) pad the struct with an extra byte.
*/

typedef	struct
{	char	bytes [3] ;
} tribyte ;

static int	pcm_read_sc2s  (SF_PRIVATE *psf, short *ptr, int len) ;
static int	pcm_read_uc2s  (SF_PRIVATE *psf, short *ptr, int len) ;
static int	pcm_read_bes2s (SF_PRIVATE *psf, short *ptr, int len) ;
static int	pcm_read_les2s (SF_PRIVATE *psf, short *ptr, int len) ;
static int	pcm_read_bet2s (SF_PRIVATE *psf, short *ptr, int len) ;
static int	pcm_read_let2s (SF_PRIVATE *psf, short *ptr, int len) ;
static int	pcm_read_bei2s (SF_PRIVATE *psf, short *ptr, int len) ;
static int	pcm_read_lei2s (SF_PRIVATE *psf, short *ptr, int len) ;

static int	pcm_read_sc2i  (SF_PRIVATE *psf, int *ptr, int len) ;
static int	pcm_read_uc2i  (SF_PRIVATE *psf, int *ptr, int len) ;
static int	pcm_read_bes2i (SF_PRIVATE *psf, int *ptr, int len) ;
static int	pcm_read_les2i (SF_PRIVATE *psf, int *ptr, int len) ;
static int	pcm_read_bet2i (SF_PRIVATE *psf, int *ptr, int len) ;
static int	pcm_read_let2i (SF_PRIVATE *psf, int *ptr, int len) ;
static int	pcm_read_bei2i (SF_PRIVATE *psf, int *ptr, int len) ;
static int	pcm_read_lei2i (SF_PRIVATE *psf, int *ptr, int len) ;

static int	pcm_read_sc2f  (SF_PRIVATE *psf, float *ptr, int len) ;
static int	pcm_read_uc2f  (SF_PRIVATE *psf, float *ptr, int len) ;
static int	pcm_read_bes2f (SF_PRIVATE *psf, float *ptr, int len) ;
static int	pcm_read_les2f (SF_PRIVATE *psf, float *ptr, int len) ;
static int	pcm_read_bet2f (SF_PRIVATE *psf, float *ptr, int len) ;
static int	pcm_read_let2f (SF_PRIVATE *psf, float *ptr, int len) ;
static int	pcm_read_bei2f (SF_PRIVATE *psf, float *ptr, int len) ;
static int	pcm_read_lei2f (SF_PRIVATE *psf, float *ptr, int len) ;

static int	pcm_read_sc2d  (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;
static int	pcm_read_uc2d  (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;
static int	pcm_read_bes2d (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;
static int	pcm_read_les2d (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;
static int	pcm_read_bet2d (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;
static int	pcm_read_let2d (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;
static int	pcm_read_bei2d (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;
static int	pcm_read_lei2d (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;


static int	pcm_write_s2sc  (SF_PRIVATE *psf, short *ptr, int len) ;
static int	pcm_write_s2uc  (SF_PRIVATE *psf, short *ptr, int len) ;
static int	pcm_write_s2bes (SF_PRIVATE *psf, short *ptr, int len) ;
static int	pcm_write_s2les (SF_PRIVATE *psf, short *ptr, int len) ;
static int	pcm_write_s2bet (SF_PRIVATE *psf, short *ptr, int len) ;
static int	pcm_write_s2let (SF_PRIVATE *psf, short *ptr, int len) ;
static int	pcm_write_s2bei (SF_PRIVATE *psf, short *ptr, int len) ;
static int	pcm_write_s2lei (SF_PRIVATE *psf, short *ptr, int len) ;

static int	pcm_write_i2sc  (SF_PRIVATE *psf, int *ptr, int len) ;
static int	pcm_write_i2uc  (SF_PRIVATE *psf, int *ptr, int len) ;
static int	pcm_write_i2bes (SF_PRIVATE *psf, int *ptr, int len) ;
static int	pcm_write_i2les (SF_PRIVATE *psf, int *ptr, int len) ;
static int	pcm_write_i2bet (SF_PRIVATE *psf, int *ptr, int len) ;
static int	pcm_write_i2let (SF_PRIVATE *psf, int *ptr, int len) ;
static int	pcm_write_i2bei (SF_PRIVATE *psf, int *ptr, int len) ;
static int	pcm_write_i2lei (SF_PRIVATE *psf, int *ptr, int len) ;

static int	pcm_write_f2sc  (SF_PRIVATE *psf, float *ptr, int len) ;
static int	pcm_write_f2uc  (SF_PRIVATE *psf, float *ptr, int len) ;
static int	pcm_write_f2bes (SF_PRIVATE *psf, float *ptr, int len) ;
static int	pcm_write_f2les (SF_PRIVATE *psf, float *ptr, int len) ;
static int	pcm_write_f2bet (SF_PRIVATE *psf, float *ptr, int len) ;
static int	pcm_write_f2let (SF_PRIVATE *psf, float *ptr, int len) ;
static int	pcm_write_f2bei (SF_PRIVATE *psf, float *ptr, int len) ;
static int	pcm_write_f2lei (SF_PRIVATE *psf, float *ptr, int len) ;

static int	pcm_write_d2sc  (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;
static int	pcm_write_d2uc  (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;
static int	pcm_write_d2bes (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;
static int	pcm_write_d2les (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;
static int	pcm_write_d2bet (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;
static int	pcm_write_d2let (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;
static int	pcm_write_d2bei (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;
static int	pcm_write_d2lei (SF_PRIVATE *psf, double *ptr, int len, int normalize) ;

static	void	sc2s_array	(signed char *buffer, unsigned int count, short *ptr, int index) ;
static	void	uc2s_array	(unsigned char *buffer, unsigned int count, short *ptr, int index) ;
static	void	bet2s_array (tribyte *buffer, unsigned int count, short *ptr, int index) ;
static	void	let2s_array (tribyte *buffer, unsigned int count, short *ptr, int index) ;
static	void	bei2s_array (int *buffer, unsigned int count, short *ptr, int index) ;
static	void	lei2s_array (int *buffer, unsigned int count, short *ptr, int index) ;

static	void	sc2i_array	(signed char *buffer, unsigned int count, int *ptr, int index) ;
static	void	uc2i_array	(unsigned char *buffer, unsigned int count, int *ptr, int index) ;
static	void	bes2i_array (short *buffer, unsigned int count, int *ptr, int index) ;
static	void	les2i_array (short *buffer, unsigned int count, int *ptr, int index) ;
static	void	bet2i_array (tribyte *buffer, unsigned int count, int *ptr, int index) ;
static	void	let2i_array (tribyte *buffer, unsigned int count, int *ptr, int index) ;

static	void	sc2f_array	(signed char *buffer, unsigned int count, float *ptr, int index, float normfact) ;
static	void	uc2f_array	(unsigned char *buffer, unsigned int count, float *ptr, int index, float normfact) ;
static	void	bes2f_array (short *buffer, unsigned int count, float *ptr, int index, float normfact) ;
static	void	les2f_array (short *buffer, unsigned int count, float *ptr, int index, float normfact) ;
static	void	bet2f_array (tribyte *buffer, unsigned int count, float *ptr, int index, float normfact) ;
static	void	let2f_array (tribyte *buffer, unsigned int count, float *ptr, int index, float normfact) ;
static	void	bei2f_array (int *buffer, unsigned int count, float *ptr, int index, float normfact) ;
static	void	lei2f_array (int *buffer, unsigned int count, float *ptr, int index, float normfact) ;

static	void	sc2d_array	(signed char *buffer, unsigned int count, double *ptr, int index, double normfact) ;
static	void	uc2d_array	(unsigned char *buffer, unsigned int count, double *ptr, int index, double normfact) ;
static	void	bes2d_array (short *buffer, unsigned int count, double *ptr, int index, double normfact) ;
static	void	les2d_array (short *buffer, unsigned int count, double *ptr, int index, double normfact) ;
static	void	bet2d_array (tribyte *buffer, unsigned int count, double *ptr, int index, double normfact) ;
static	void	let2d_array (tribyte *buffer, unsigned int count, double *ptr, int index, double normfact) ;
static	void	bei2d_array (int *buffer, unsigned int count, double *ptr, int index, double normfact) ;
static	void	lei2d_array (int *buffer, unsigned int count, double *ptr, int index, double normfact) ;


static	void	s2sc_array	(short *ptr, int index, signed char *buffer, unsigned int count) ;
static	void	s2uc_array	(short *ptr, int index, unsigned char *buffer, unsigned int count) ;
static	void	s2bet_array (short *ptr, int index, tribyte *buffer, unsigned int count) ;
static	void	s2let_array (short *ptr, int index, tribyte *buffer, unsigned int count) ;
static	void	s2bei_array (short *ptr, int index, int *buffer, unsigned int count) ;
static	void	s2lei_array (short *ptr, int index, int *buffer, unsigned int count) ;

static	void	i2sc_array	(int *ptr, int index, signed char *buffer, unsigned int count) ;
static	void	i2uc_array	(int *ptr, int index, unsigned char *buffer, unsigned int count) ;
static	void	i2bes_array (int *ptr, int index, short *buffer, unsigned int count) ;
static	void	i2les_array (int *ptr, int index, short *buffer, unsigned int count) ;
static	void	i2bet_array (int *ptr, int index, tribyte *buffer, unsigned int count) ;
static	void	i2let_array (int *ptr, int index, tribyte *buffer, unsigned int count) ;

static	void	f2sc_array	(float *ptr, int index, signed char *buffer, unsigned int count, float normfact) ;
static	void	f2uc_array	(float *ptr, int index, unsigned char *buffer, unsigned int count, float normfact) ;
static	void	f2bes_array (float *ptr, int index, short *buffer, unsigned int count, float normfact) ;
static	void	f2les_array (float *ptr, int index, short *buffer, unsigned int count, float normfact) ;
static	void	f2bet_array (float *ptr, int index, tribyte *buffer, unsigned int count, float normfact) ;
static	void	f2let_array (float *ptr, int index, tribyte *buffer, unsigned int count, float normfact) ;
static 	void	f2bei_array (float *ptr, int index, int *buffer, unsigned int count, float normfact) ;
static 	void	f2lei_array (float *ptr, int index, int *buffer, unsigned int count, float normfact) ;

static	void	d2sc_array	(double *ptr, int index, signed char *buffer, unsigned int count, double normfact) ;
static	void	d2uc_array	(double *ptr, int index, unsigned char *buffer, unsigned int count, double normfact) ;
static	void	d2bes_array (double *ptr, int index, short *buffer, unsigned int count, double normfact) ;
static	void	d2les_array (double *ptr, int index, short *buffer, unsigned int count, double normfact) ;
static	void	d2bet_array (double *ptr, int index, tribyte *buffer, unsigned int count, double normfact) ;
static	void	d2let_array (double *ptr, int index, tribyte *buffer, unsigned int count, double normfact) ;
static 	void	d2bei_array (double *ptr, int index, int *buffer, unsigned int count, double normfact) ;
static 	void	d2lei_array (double *ptr, int index, int *buffer, unsigned int count, double normfact) ;

/*-----------------------------------------------------------------------------------------------
*/

int
pcm_read_init (SF_PRIVATE *psf)
{
	if (psf->bytewidth == 1 && psf->chars == SF_CHARS_SIGNED)
	{	psf->read_short  = (func_short)  pcm_read_sc2s ;
		psf->read_int    = (func_int)    pcm_read_sc2i ;
		psf->read_float  = (func_float)  pcm_read_sc2f ;
		psf->read_double = (func_double) pcm_read_sc2d ;
		return 0 ;
		} 
	else if (psf->bytewidth == 1 && psf->chars == SF_CHARS_UNSIGNED)
	{	psf->read_short  = (func_short)  pcm_read_uc2s ;
		psf->read_int    = (func_int)    pcm_read_uc2i ;
		psf->read_float  = (func_float)  pcm_read_uc2f ;
		psf->read_double = (func_double) pcm_read_uc2d ;
		return 0 ;
		} ;
		
	switch (psf->bytewidth * 0x10000 + psf->endian)
	{	case  (2 * 0x10000 + SF_ENDIAN_BIG) :
				psf->read_short  = (func_short)  pcm_read_bes2s ;
				psf->read_int    = (func_int)    pcm_read_bes2i ;
				psf->read_float  = (func_float)  pcm_read_bes2f ;
				psf->read_double = (func_double) pcm_read_bes2d ;
				break ;
		case  (3 * 0x10000 + SF_ENDIAN_BIG) :
				psf->read_short  = (func_short)  pcm_read_bet2s ;
				psf->read_int    = (func_int)    pcm_read_bet2i ;
				psf->read_float  = (func_float)  pcm_read_bet2f ;
				psf->read_double = (func_double) pcm_read_bet2d ;
				break ;
		case  (4 * 0x10000 + SF_ENDIAN_BIG) :
				psf->read_short  = (func_short)  pcm_read_bei2s ;
				psf->read_int    = (func_int)    pcm_read_bei2i ;
				psf->read_float  = (func_float)  pcm_read_bei2f ;
				psf->read_double = (func_double) pcm_read_bei2d ;
				break ;
				
		case  (2 * 0x10000 + SF_ENDIAN_LITTLE) :
				psf->read_short  = (func_short)  pcm_read_les2s ;
				psf->read_int    = (func_int)    pcm_read_les2i ;
				psf->read_float  = (func_float)  pcm_read_les2f ;
				psf->read_double = (func_double) pcm_read_les2d ;
				break ;
		case  (3 * 0x10000 + SF_ENDIAN_LITTLE) :
				psf->read_short  = (func_short)  pcm_read_let2s ;
				psf->read_int    = (func_int)    pcm_read_let2i ;
				psf->read_float  = (func_float)  pcm_read_let2f ;
				psf->read_double = (func_double) pcm_read_let2d ;
				break ;
		case  (4 * 0x10000 + SF_ENDIAN_LITTLE) :
				psf->read_short  = (func_short)  pcm_read_lei2s ;
				psf->read_int    = (func_int)    pcm_read_lei2i ;
				psf->read_float  = (func_float)  pcm_read_lei2f ;
				psf->read_double = (func_double) pcm_read_lei2d ;
				break ;
		default : return SFE_UNIMPLEMENTED ;
		} ;

	return 0 ;
} /* pcm_read_init */

int
pcm_write_init (SF_PRIVATE *psf)
{
	if (psf->bytewidth == 1 && psf->chars == SF_CHARS_SIGNED)
	{	psf->write_short  = (func_short)  pcm_write_s2sc ;
		psf->write_int    = (func_int)    pcm_write_i2sc ;
		psf->write_float  = (func_float)  pcm_write_f2sc ;
		psf->write_double = (func_double) pcm_write_d2sc ;
		return 0 ;
		} 
	else if (psf->bytewidth == 1 && psf->chars == SF_CHARS_UNSIGNED)
	{	psf->write_short  = (func_short)  pcm_write_s2uc ;
		psf->write_int    = (func_int)    pcm_write_i2uc ;
		psf->write_float  = (func_float)  pcm_write_f2uc ;
		psf->write_double = (func_double) pcm_write_d2uc ;
		return 0 ;
		} ;
		
	switch (psf->bytewidth * 0x10000 + psf->endian)
	{	case  (2 * 0x10000 + SF_ENDIAN_BIG) :
				psf->write_short  = (func_short)  pcm_write_s2bes ;
				psf->write_int    = (func_int)    pcm_write_i2bes ;
				psf->write_float  = (func_float)  pcm_write_f2bes ;
				psf->write_double = (func_double) pcm_write_d2bes ;
				break ;
				
		case  (3 * 0x10000 + SF_ENDIAN_BIG) :
				psf->write_short  = (func_short)  pcm_write_s2bet ;
				psf->write_int    = (func_int)    pcm_write_i2bet ;
				psf->write_float  = (func_float)  pcm_write_f2bet ;
				psf->write_double = (func_double) pcm_write_d2bet ;
				break ;
				
		case  (4 * 0x10000 + SF_ENDIAN_BIG) :
				psf->write_short  = (func_short)  pcm_write_s2bei ;
				psf->write_int    = (func_int)    pcm_write_i2bei ;
				psf->write_float  = (func_float)  pcm_write_f2bei ;
				psf->write_double = (func_double) pcm_write_d2bei ;
				break ;
				
		case  (2 * 0x10000 + SF_ENDIAN_LITTLE) :
				psf->write_short  = (func_short)  pcm_write_s2les ;
				psf->write_int    = (func_int)    pcm_write_i2les ;
				psf->write_float  = (func_float)  pcm_write_f2les ;
				psf->write_double = (func_double) pcm_write_d2les ;
				break ;
				
		case  (3 * 0x10000 + SF_ENDIAN_LITTLE) :
				psf->write_short  = (func_short)  pcm_write_s2let ;
				psf->write_int    = (func_int)    pcm_write_i2let ;
				psf->write_float  = (func_float)  pcm_write_f2let ;
				psf->write_double = (func_double) pcm_write_d2let ;
				break ;
				
		case  (4 * 0x10000 + SF_ENDIAN_LITTLE) :
				psf->write_short  = (func_short)  pcm_write_s2lei ;
				psf->write_int    = (func_int)    pcm_write_i2lei ;
				psf->write_float  = (func_float)  pcm_write_f2lei ;
				psf->write_double = (func_double) pcm_write_d2lei ;
				break ;
				
		default : return SFE_UNIMPLEMENTED ;
		} ;

	return 0 ;
} /* pcm_read_init */


/*-----------------------------------------------------------------------------------------------
*/

static int		
pcm_read_sc2s (SF_PRIVATE *psf, short *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		sc2s_array ((signed char*) (psf->buffer), thisread / psf->bytewidth, ptr, index) ;
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
} /* pcm_read_sc2s */

static int
pcm_read_uc2s (SF_PRIVATE *psf, short *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		uc2s_array ((unsigned char*) (psf->buffer), thisread / psf->bytewidth, ptr, index) ;
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
} /* pcm_read_uc2s */

static int
pcm_read_bes2s (SF_PRIVATE *psf, short *ptr, int len)
{	int		total ;

	total = fread (ptr, 1, len * sizeof (short), psf->file) ;
	if (CPU_IS_LITTLE_ENDIAN)
		endswap_short_array	(ptr, len) ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_READ ;
	
	return total ;
} /* pcm_read_bes2s */

static int
pcm_read_les2s (SF_PRIVATE *psf, short *ptr, int len)
{	int		total ;

	total = fread (ptr, 1, len * sizeof (short), psf->file) ;
	if (CPU_IS_BIG_ENDIAN)
		endswap_short_array	(ptr, len) ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_READ ;
	
	return total ;
} /* pcm_read_les2s */

static int
pcm_read_bet2s (SF_PRIVATE *psf, short *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		bet2s_array ((tribyte*) (psf->buffer), thisread / psf->bytewidth, ptr, index) ;
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
} /* pcm_read_bet2s */

static int
pcm_read_let2s (SF_PRIVATE *psf, short *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		let2s_array ((tribyte*) (psf->buffer), thisread / psf->bytewidth, ptr, index) ;
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
} /* pcm_read_let2s */

static int
pcm_read_bei2s (SF_PRIVATE *psf, short *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		bei2s_array ((int*) (psf->buffer), thisread / psf->bytewidth, ptr, index) ;
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
} /* pcm_read_bei2s */

static int
pcm_read_lei2s (SF_PRIVATE *psf, short *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		lei2s_array ((int*) (psf->buffer), thisread / psf->bytewidth, ptr, index) ;
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
} /* pcm_read_lei2s */

/*-----------------------------------------------------------------------------------------------
*/

static int
pcm_read_sc2i (SF_PRIVATE *psf, int *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		sc2i_array ((signed char*) (psf->buffer), thisread / psf->bytewidth, ptr, index) ;
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
} /* pcm_read_sc2i */

static int
pcm_read_uc2i (SF_PRIVATE *psf, int *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		uc2i_array ((unsigned char*) (psf->buffer), thisread / psf->bytewidth, ptr, index) ;
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
} /* pcm_read_uc2i */

static int
pcm_read_bes2i (SF_PRIVATE *psf, int *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		bes2i_array ((short*) (psf->buffer), thisread / psf->bytewidth, ptr, index) ;
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
} /* pcm_read_bes2i */

static int
pcm_read_les2i (SF_PRIVATE *psf, int *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		les2i_array ((short*) (psf->buffer), thisread / psf->bytewidth, ptr, index) ;
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
} /* pcm_read_les2i */

static int
pcm_read_bet2i (SF_PRIVATE *psf, int *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		bet2i_array ((tribyte*) (psf->buffer), thisread / psf->bytewidth, ptr, index) ;
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
} /* pcm_read_bet2i */

static int
pcm_read_let2i (SF_PRIVATE *psf, int *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		let2i_array ((tribyte*) (psf->buffer), thisread / psf->bytewidth, ptr, index) ;
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
} /* pcm_read_let2i */

static int
pcm_read_bei2i (SF_PRIVATE *psf, int *ptr, int len)
{	int		total ;

	total = fread (ptr, 1, len * sizeof (int), psf->file) ;
	if (CPU_IS_LITTLE_ENDIAN)
		endswap_int_array	(ptr, len) ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_READ ;
	
	return total ;
} /* pcm_read_bei2i */

static int
pcm_read_lei2i (SF_PRIVATE *psf, int *ptr, int len)
{	int		total ;

	total = fread (ptr, 1, len * sizeof (int), psf->file) ;
	if (CPU_IS_BIG_ENDIAN)
		endswap_int_array	(ptr, len) ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_READ ;
	
	return total ;
} /* pcm_read_lei2i */

/*-----------------------------------------------------------------------------------------------
*/

static int
pcm_read_sc2f (SF_PRIVATE *psf, float *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;

	normfact = (psf->norm_float == SF_TRUE) ? 1.0 / ((float) 0x80) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		sc2f_array ((signed char*) (psf->buffer), thisread / psf->bytewidth, ptr, index, normfact) ;
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
} /* pcm_read_sc2f */

static int
pcm_read_uc2f (SF_PRIVATE *psf, float *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;

	normfact = (psf->norm_float == SF_TRUE) ? 1.0 / ((float) 0x80) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		uc2f_array ((unsigned char*) (psf->buffer), thisread / psf->bytewidth, ptr, index, normfact) ;
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
} /* pcm_read_uc2f */

static int
pcm_read_bes2f (SF_PRIVATE *psf, float *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;

	normfact = (psf->norm_float == SF_TRUE) ? 1.0 / ((float) 0x8000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		bes2f_array ((short*) (psf->buffer), thisread / psf->bytewidth, ptr, index, normfact) ;
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
} /* pcm_read_bes2f */

static int
pcm_read_les2f (SF_PRIVATE *psf, float *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;

	normfact = (psf->norm_float == SF_TRUE) ? 1.0 / ((float) 0x8000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		les2f_array ((short*) (psf->buffer), thisread / psf->bytewidth, ptr, index, normfact) ;
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
} /* pcm_read_les2f */

static int
pcm_read_bet2f (SF_PRIVATE *psf, float *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;

	normfact = (psf->norm_float == SF_TRUE) ? 1.0 / ((float) 0x800000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		bet2f_array ((tribyte*) (psf->buffer), thisread / psf->bytewidth, ptr, index, normfact) ;
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
} /* pcm_read_bet2f */

static int
pcm_read_let2f (SF_PRIVATE *psf, float *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;

	normfact = (psf->norm_float == SF_TRUE) ? 1.0 / ((float) 0x800000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		let2f_array ((tribyte*) (psf->buffer), thisread / psf->bytewidth, ptr, index, normfact) ;
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
} /* pcm_read_let2f */

static int
pcm_read_bei2f (SF_PRIVATE *psf, float *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;

	normfact = (psf->norm_float == SF_TRUE) ? 1.0 / ((float) 0x80000000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		bei2f_array ((int*) (psf->buffer), thisread / psf->bytewidth, ptr, index, normfact) ;
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
} /* pcm_read_bei2f */

static int
pcm_read_lei2f (SF_PRIVATE *psf, float *ptr, int len)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;

	normfact = (psf->norm_float == SF_TRUE) ? 1.0 / ((float) 0x80000000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		lei2f_array ((int*) (psf->buffer), thisread / psf->bytewidth, ptr, index, normfact) ;
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
} /* pcm_read_lei2f */

/*-----------------------------------------------------------------------------------------------
*/

static int
pcm_read_sc2d (SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double		normfact ;

	normfact = (normalize ? 1.0 / ((double) 0x80) : 1.0) ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		sc2d_array ((signed char*) (psf->buffer), thisread / psf->bytewidth, ptr, index, normfact) ;
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
} /* pcm_read_sc2d */

static int
pcm_read_uc2d (SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double		normfact ;

	normfact = (normalize ? 1.0 / ((double) 0x80) : 1.0) ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		uc2d_array ((unsigned char*) (psf->buffer), thisread / psf->bytewidth, ptr, index, normfact) ;
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
} /* pcm_read_uc2d */

static int
pcm_read_bes2d (SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double		normfact ;

	normfact = (normalize ? 1.0 / ((double) 0x8000) : 1.0) ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		bes2d_array ((short*) (psf->buffer), thisread / psf->bytewidth, ptr, index, normfact) ;
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
} /* pcm_read_bes2d */

static int
pcm_read_les2d (SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double		normfact ;

	normfact = (normalize ? 1.0 / ((double) 0x8000) : 1.0) ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		les2d_array ((short*) (psf->buffer), thisread / psf->bytewidth, ptr, index, normfact) ;
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
} /* pcm_read_les2d */

static int
pcm_read_bet2d (SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double		normfact ;

	normfact = (normalize ? 1.0 / ((double) 0x800000) : 1.0) ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		bet2d_array ((tribyte*) (psf->buffer), thisread / psf->bytewidth, ptr, index, normfact) ;
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
} /* pcm_read_bet2d */

static int
pcm_read_let2d (SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double		normfact ;

	normfact = (normalize ? 1.0 / ((double) 0x800000) : 1.0) ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		let2d_array ((tribyte*) (psf->buffer), thisread / psf->bytewidth, ptr, index, normfact) ;
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
} /* pcm_read_let2d */

static int
pcm_read_bei2d (SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double		normfact ;

	normfact = (normalize ? 1.0 / ((double) 0x80000000) : 1.0) ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		bei2d_array ((int*) (psf->buffer), thisread / psf->bytewidth, ptr, index, normfact) ;
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
} /* pcm_read_bei2d */

static int
pcm_read_lei2d (SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	unsigned int readcount, thisread ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double		normfact ;

	normfact = (normalize ? 1.0 / ((double) 0x80000000) : 1.0) ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = fread (psf->buffer, 1, readcount, psf->file) ;
		lei2d_array ((int*) (psf->buffer), thisread / psf->bytewidth, ptr, index, normfact) ;
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
} /* pcm_read_lei2d */

/*===============================================================================================
**-----------------------------------------------------------------------------------------------
**===============================================================================================
*/

static int
pcm_write_s2sc	(SF_PRIVATE *psf, short *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		s2sc_array (ptr, index, (signed char*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_s2sc */

static int
pcm_write_s2uc	(SF_PRIVATE *psf, short *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		s2uc_array (ptr, index, (unsigned char*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_s2uc */

static int
pcm_write_s2bes	(SF_PRIVATE *psf, short *ptr, int len)
{	int		total ;

	if (CPU_IS_LITTLE_ENDIAN)
		endswap_short_array (ptr, len) ;
	total = fwrite (ptr, 1, len * sizeof (short), psf->file) ;
	if (CPU_IS_LITTLE_ENDIAN)
		endswap_short_array (ptr, len) ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_WRITE ;
	
	return total ;
} /* pcm_write_s2bes */

static int
pcm_write_s2les	(SF_PRIVATE *psf, short *ptr, int len)
{	int		total ;

	if (CPU_IS_BIG_ENDIAN)
		endswap_short_array (ptr, len) ;
	total = fwrite (ptr, 1, len * sizeof (short), psf->file) ;
	if (CPU_IS_BIG_ENDIAN)
		endswap_short_array (ptr, len) ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_WRITE ;
	
	return total ;
} /* pcm_write_s2les */

static int
pcm_write_s2bet	(SF_PRIVATE *psf, short *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		s2bet_array (ptr, index, (tribyte*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_s2bet */

static int
pcm_write_s2let	(SF_PRIVATE *psf, short *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		s2let_array (ptr, index, (tribyte*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_s2let */

static int
pcm_write_s2bei	(SF_PRIVATE *psf, short *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		s2bei_array (ptr, index, (int*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_s2bei */

static int
pcm_write_s2lei	(SF_PRIVATE *psf, short *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		s2lei_array (ptr, index, (int*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_s2lei */

/*-----------------------------------------------------------------------------------------------
*/

static int
pcm_write_i2sc	(SF_PRIVATE *psf, int *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		i2sc_array (ptr, index, (signed char*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_i2sc */

static int
pcm_write_i2uc	(SF_PRIVATE *psf, int *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		i2uc_array (ptr, index, (unsigned char*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_i2uc */

static int
pcm_write_i2bes	(SF_PRIVATE *psf, int *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		i2bes_array (ptr, index, (short*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_i2bes */

static int
pcm_write_i2les	(SF_PRIVATE *psf, int *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		i2les_array (ptr, index, (short*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_i2les */

static int
pcm_write_i2bet	(SF_PRIVATE *psf, int *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		i2bet_array (ptr, index, (tribyte*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_i2bet */

static int
pcm_write_i2let	(SF_PRIVATE *psf, int *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		i2let_array (ptr, index, (tribyte*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_i2les */

static int
pcm_write_i2bei	(SF_PRIVATE *psf, int *ptr, int len)
{	int		total ;

	if (CPU_IS_LITTLE_ENDIAN)
		endswap_int_array (ptr, len) ;
	total = fwrite (ptr, 1, len * sizeof (int), psf->file) ;
	if (CPU_IS_LITTLE_ENDIAN)
		endswap_int_array (ptr, len) ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_WRITE ;
	
	return total ;
} /* pcm_write_i2bei */

static int
pcm_write_i2lei	(SF_PRIVATE *psf, int *ptr, int len)
{	int		total ;

	if (CPU_IS_BIG_ENDIAN)
		endswap_int_array (ptr, len) ;
	total = fwrite (ptr, 1, len * sizeof (int), psf->file) ;
	if (CPU_IS_BIG_ENDIAN)
		endswap_int_array (ptr, len) ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_WRITE ;
	
	return total ;
} /* pcm_write_i2lei */

/*-----------------------------------------------------------------------------------------------
*/

static int
pcm_write_f2sc	(SF_PRIVATE *psf, float *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;
	
	normfact = (psf->norm_float == SF_TRUE) ? ((float) 0x80) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		f2sc_array (ptr, index, (signed char*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_f2sc */

static int
pcm_write_f2uc	(SF_PRIVATE *psf, float *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;
	
	normfact = (psf->norm_float == SF_TRUE) ? ((float) 0x80) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		f2uc_array (ptr, index, (unsigned char*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_f2uc */

static int
pcm_write_f2bes	(SF_PRIVATE *psf, float *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;
	
	normfact = (psf->norm_float == SF_TRUE) ? ((float) 0x8000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		f2bes_array (ptr, index, (short*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_f2bes */

static int
pcm_write_f2les	(SF_PRIVATE *psf, float *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;
	
	normfact = (psf->norm_float == SF_TRUE) ? ((float) 0x8000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		f2les_array (ptr, index, (short*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_f2les */

static int
pcm_write_f2let	(SF_PRIVATE *psf, float *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;
	
	normfact = (psf->norm_float == SF_TRUE) ? ((float) 0x800000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		f2let_array (ptr, index, (tribyte*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_f2les */

static int
pcm_write_f2bet	(SF_PRIVATE *psf, float *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;
	
	normfact = (psf->norm_float == SF_TRUE) ? ((float) 0x800000) : 1.0 ;


	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		f2bet_array (ptr, index, (tribyte*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_f2bes */

static int
pcm_write_f2bei	(SF_PRIVATE *psf, float *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;
	
	normfact = (psf->norm_float == SF_TRUE) ? ((float) 0x80000000) : 1.0 ;


	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		f2bei_array (ptr, index, (int*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_f2bei */

static int
pcm_write_f2lei	(SF_PRIVATE *psf, float *ptr, int len)
{	unsigned int	writecount, thiswrite ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;
	
	normfact = (psf->norm_float == SF_TRUE) ? ((float) 0x80000000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		f2lei_array (ptr, index, (int*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_f2lei */

/*-----------------------------------------------------------------------------------------------
*/

static int
pcm_write_d2sc	(SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	unsigned int	writecount, thiswrite ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double	normfact ;
	
	normfact = (normalize ? ((double) 0x80) : 1.0) ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		d2sc_array (ptr, index, (signed char*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_d2sc */

static int
pcm_write_d2uc	(SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	unsigned int	writecount, thiswrite ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double	normfact ;
	
	normfact = (normalize ? ((double) 0x80) : 1.0) ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		d2uc_array (ptr, index, (unsigned char*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_d2uc */

static int
pcm_write_d2bes	(SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	unsigned int	writecount, thiswrite ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double	normfact ;
	
	normfact = (normalize ? ((double) 0x8000) : 1.0) ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		d2bes_array (ptr, index, (short*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_d2bes */

static int
pcm_write_d2les	(SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	unsigned int	writecount, thiswrite ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double	normfact ;
	
	normfact = (normalize ? ((double) 0x8000) : 1.0) ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		d2les_array (ptr, index, (short*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_d2les */

static int
pcm_write_d2let	(SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	unsigned int	writecount, thiswrite ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double	normfact ;
	
	normfact = (normalize ? ((double) 0x800000) : 1.0) ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		d2let_array (ptr, index, (tribyte*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_d2les */

static int
pcm_write_d2bet	(SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	unsigned int	writecount, thiswrite ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double	normfact ;
	
	normfact = (normalize ? ((double) 0x800000) : 1.0) ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		d2bet_array (ptr, index, (tribyte*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_d2bes */

static int
pcm_write_d2bei	(SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	unsigned int	writecount, thiswrite ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double	normfact ;
	
	normfact = (normalize ? ((double) 0x80000000) : 1.0) ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		d2bei_array (ptr, index, (int*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_d2bei */

static int
pcm_write_d2lei	(SF_PRIVATE *psf, double *ptr, int len, int normalize)
{	unsigned int	writecount, thiswrite ;
	int		bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double	normfact ;
	
	normfact = (normalize ? ((double) 0x80000000) : 1.0) ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		d2lei_array (ptr, index, (int*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_d2lei */

/*-----------------------------------------------------------------------------------------------
*/

static	void	
sc2s_array	(signed char *buffer, unsigned int count, short *ptr, int index)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	ptr [index] = ((short) buffer [k]) ;
		index ++ ;
		} ;
} /* sc2s_array */

static	void	
uc2s_array	(unsigned char *buffer, unsigned int count, short *ptr, int index)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	ptr [index] = ((((short) buffer [k]) - 128) % 256) ;
		index ++ ;
		} ;
} /* uc2s_array */

static void	
bet2s_array (tribyte *buffer, unsigned int count, short *ptr, int index)
{	unsigned char	*cptr ;
	int		k ;
	int 	value;
	cptr = (unsigned char*) buffer ;
	for (k = 0 ; k < count ; k++)
	{	value = (cptr [0] << 24) + (cptr [1] << 16) + (cptr [2] << 8) ;
		value = BE2H_INT (value) ;
		ptr [index] = (short) (value >> 16) ;
		index ++ ;
		cptr += 3 ;	
		} ;
} /* bet2s_array */

static void	
let2s_array (tribyte *buffer, unsigned int count, short *ptr, int index)
{	unsigned char	*cptr ;
	int		k ;
	int 	value;
	cptr = (unsigned char*) buffer ;
	for (k = 0 ; k < count ; k++)
	{	value = (cptr [0] << 8) + (cptr [1] << 16) + (cptr [2] << 24) ;
		value = LE2H_INT (value) ;
		ptr [index] = (short) (value >> 16) ;
		index ++ ;
		cptr += 3 ;
		} ;
} /* let2s_array */

static void	
bei2s_array (int *buffer, unsigned int count, short *ptr, int index)
{	int	k ;
	int		value ;
	for (k = 0 ; k < count ; k++)
	{	value = BE2H_INT (buffer [k]) ;
		ptr [index] = (short) (value >> 16) ;
		index ++ ;
		} ;
} /* bei2s_array */

static void	
lei2s_array (int *buffer, unsigned int count, short *ptr, int index)
{	int	k ;
	int		value ;
	for (k = 0 ; k < count ; k++)
	{	value = LE2H_INT (buffer [k]) ;
		ptr [index] = (short) (value >> 16) ;
		index ++ ;
		} ;
} /* lei2s_array */


/*-----------------------------------------------------------------------------------------------
*/

static	void	
sc2i_array	(signed char *buffer, unsigned int count, int *ptr, int index)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	ptr [index] = ((int) buffer [k]) ;
		index ++ ;
		} ;
} /* sc2i_array */

static	void	
uc2i_array	(unsigned char *buffer, unsigned int count, int *ptr, int index)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	ptr [index] = ((((int) buffer [k]) - 128) % 256) ;
		index ++ ;
		} ;
} /* uc2i_array */

static void	
bes2i_array (short *buffer, unsigned int count, int *ptr, int index)
{	int	k ;
	short	value ;
	for (k = 0 ; k < count ; k++)
	{	value = BE2H_SHORT (buffer [k]) ;
		ptr [index] = ((int) value) ;
		index ++ ;
		} ;
} /* bes2i_array */

static void	
les2i_array (short *buffer, unsigned int count, int *ptr, int index)
{	int	k ;
	short	value ;
	for (k = 0 ; k < count ; k++)
	{	value = LE2H_SHORT (buffer [k]) ;
		ptr [index] = ((int) value) ;
		index ++ ;
		} ;
} /* les2i_array */

static void	
bet2i_array (tribyte *buffer, unsigned int count, int *ptr, int index)
{	unsigned char	*cptr ;
	int		k ;
	int 	value;
	cptr = (unsigned char*) buffer ;
	for (k = 0 ; k < count ; k++)
	{	value = (cptr [0] << 24) + (cptr [1] << 16) + (cptr [2] << 8) ;
		ptr [index] = value / 256 ;
		index ++ ;
		cptr += 3 ;
		} ;
} /* bet2i_array */

static void	
let2i_array (tribyte *buffer, unsigned int count, int *ptr, int index)
{	unsigned char	*cptr ;
	int	k ;
	int 	value;
	cptr = (unsigned char*) buffer ;
	for (k = 0 ; k < count ; k++)
	{	value = (cptr [0] << 8) + (cptr [1] << 16) + (cptr [2] << 24) ;
		ptr [index] = value / 256 ;
		index ++ ;
		cptr += 3 ;
		} ;
} /* let2i_array */

/*-----------------------------------------------------------------------------------------------
*/


static	void	
sc2f_array	(signed char *buffer, unsigned int count, float *ptr, int index, float normfact)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	ptr [index] = ((float) buffer [k]) * normfact ;
		index ++ ;
		} ;
} /* sc2f_array */

static	void	
uc2f_array	(unsigned char *buffer, unsigned int count, float *ptr, int index, float normfact)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	ptr [index] = ((((int) buffer [k]) - 128) % 256) * normfact ;
		index ++ ;
		} ;
} /* uc2f_array */

static void	
bes2f_array (short *buffer, unsigned int count, float *ptr, int index, float normfact)
{	int	k ;
	short	value ;
	for (k = 0 ; k < count ; k++)
	{	value = BE2H_SHORT (buffer [k]) ;
		ptr [index] = ((float) value) * normfact ;
		index ++ ;
		} ;
} /* bes2f_array */

static void	
les2f_array (short *buffer, unsigned int count, float *ptr, int index, float normfact)
{	int	k ;
	short	value ;
	for (k = 0 ; k < count ; k++)
	{	value = LE2H_SHORT (buffer [k]) ;
		ptr [index] = ((float) value) * normfact ;
		index ++ ;
		} ;
} /* les2f_array */

static void	
bet2f_array (tribyte *buffer, unsigned int count, float *ptr, int index, float normfact)
{	unsigned char	*cptr ;
	int		k ;
	int 	value;
	cptr = (unsigned char*) buffer ;
	for (k = 0 ; k < count ; k++)
	{	value = (cptr [0] << 24) + (cptr [1] << 16) + (cptr [2] << 8) ;
		ptr [index] = ((float) (value / 256)) * normfact ;
		index ++ ;
		cptr += 3 ;
		} ;
} /* bet2f_array */

static void	
let2f_array (tribyte *buffer, unsigned int count, float *ptr, int index, float normfact)
{	unsigned char	*cptr ;
	int		k ;
	int 	value;
	cptr = (unsigned char*) buffer ;
	for (k = 0 ; k < count ; k++)
	{	value = (cptr [0] << 8) + (cptr [1] << 16) + (cptr [2] << 24) ;
		ptr [index] = ((float) (value / 256)) * normfact ;
		index ++ ;
		cptr += 3 ; 
		} ;
} /* let2f_array */

static void	
lei2f_array (int *buffer, unsigned int count, float *ptr, int index, float normfact)
{	int	k ;
	int		value ;
	for (k = 0 ; k < count ; k++)
	{	value = LE2H_INT (buffer [k]) ;
		ptr [index] = ((float) value) * normfact ;
		index ++ ;
		} ;
} /* lei2f_array */

static void	
bei2f_array (int *buffer, unsigned int count, float *ptr, int index, float normfact)
{	int	k ;
	int		value ;
	for (k = 0 ; k < count ; k++)
	{	value = BE2H_INT (buffer [k]) ;
		ptr [index] = ((float) value) * normfact ;
		index ++ ;
		} ;
} /* bei2f_array */


/*-----------------------------------------------------------------------------------------------
*/

static	void	
sc2d_array	(signed char *buffer, unsigned int count, double *ptr, int index, double normfact)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	ptr [index] = ((double) buffer [k]) * normfact ;
		index ++ ;
		} ;
} /* sc2d_array */

static	void	
uc2d_array	(unsigned char *buffer, unsigned int count, double *ptr, int index, double normfact)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	ptr [index] = ((((int) buffer [k]) - 128) % 256) * normfact ;
		index ++ ;
		} ;
} /* uc2d_array */

static void	
bes2d_array (short *buffer, unsigned int count, double *ptr, int index, double normfact)
{	int	k ;
	short	value ;
	for (k = 0 ; k < count ; k++)
	{	value = BE2H_SHORT (buffer [k]) ;
		ptr [index] = ((double) value) * normfact ;
		index ++ ;
		} ;
} /* bes2d_array */

static void	
les2d_array (short *buffer, unsigned int count, double *ptr, int index, double normfact)
{	int	k ;
	short	value ;
	for (k = 0 ; k < count ; k++)
	{	value = LE2H_SHORT (buffer [k]) ;
		ptr [index] = ((double) value) * normfact ;
		index ++ ;
		} ;
} /* les2d_array */

static void	
bet2d_array (tribyte *buffer, unsigned int count, double *ptr, int index, double normfact)
{	unsigned char	*cptr ;
	int		k ;
	int 	value;
	cptr = (unsigned char*) buffer ;
	for (k = 0 ; k < count ; k++)
	{	value = (cptr [0] << 24) + (cptr [1] << 16) + (cptr [2] << 8) ;
		ptr [index] = ((double) (value / 256)) * normfact ;
		index ++ ;
		cptr += 3 ;
		} ;
} /* bet2d_array */

static void	
let2d_array (tribyte *buffer, unsigned int count, double *ptr, int index, double normfact)
{	unsigned char	*cptr ;
	int		k ;
	int 	value;
	cptr = (unsigned char*) buffer ;
	for (k = 0 ; k < count ; k++)
	{	value = (cptr [0] << 8) + (cptr [1] << 16) + (cptr [2] << 24) ;
		ptr [index] = ((double) (value / 256)) * normfact ;
		index ++ ;
		cptr += 3 ; 
		} ;
} /* let2d_array */

static void	
bei2d_array (int *buffer, unsigned int count, double *ptr, int index, double normfact)
{	int	k ;
	int		value ;
	for (k = 0 ; k < count ; k++)
	{	value = BE2H_INT (buffer [k]) ;
		ptr [index] = ((double) value) * normfact ;
		index ++ ;
		} ;
} /* bei2d_array */

static void	
lei2d_array (int *buffer, unsigned int count, double *ptr, int index, double normfact)
{	int	k ;
	int		value ;
	for (k = 0 ; k < count ; k++)
	{	value = LE2H_INT (buffer [k]) ;
		ptr [index] = ((double) value) * normfact ;
		index ++ ;
		} ;
} /* lei2d_array */

/*-----------------------------------------------------------------------------------------------
*/

static	void	
s2sc_array	(short *ptr, int index, signed char *buffer, unsigned int count)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = (signed char) (ptr [index]) ;
		index ++ ;
		} ;
} /* s2sc_array */

static	void	
s2uc_array	(short *ptr, int index, unsigned char *buffer, unsigned int count)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = (unsigned char) (ptr [index] + 128) ;
		index ++ ;
		} ;
} /* s2uc_array */

static void	
s2bet_array (short *ptr, int index, tribyte *buffer, unsigned int count)
{	unsigned char	*cptr ;
	int		k, value ;
	cptr = (unsigned char*) buffer ;
	for (k = 0 ; k < count ; k++)
	{	value = ptr [index] << 8 ;
		cptr [2] = (unsigned char) (value & 0xFF) ;
		cptr [1] = (unsigned char) ((value >> 8) & 0xFF) ;
		cptr [0] = (unsigned char) ((value >> 16) & 0xFF) ;
		index ++ ;
		cptr += 3 ;
		} ;
} /* s2bet_array */

static void	
s2let_array (short *ptr, int index, tribyte *buffer, unsigned int count)
{	unsigned char	*cptr ;
	int		k, value ;
	cptr = (unsigned char*) buffer ;
	for (k = 0 ; k < count ; k++)
	{	value = ptr [index] << 8 ;
		cptr [0] = (unsigned char) (value & 0xFF) ;
		cptr [1] = (unsigned char) ((value >> 8) & 0xFF) ;
		cptr [2] = (unsigned char) ((value >> 16) & 0xFF) ;
		index ++ ;
		cptr += 3 ;
		} ;
} /* s2let_array */

static void	
s2bei_array (short *ptr, int index, int *buffer, unsigned int count)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = H2BE_INT (ptr [index] << 16) ;
		index ++ ;
		} ;
} /* s2lei_array */

static void	
s2lei_array (short *ptr, int index, int *buffer, unsigned int count)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = H2LE_INT (ptr [index] << 16) ;
		index ++ ;
		} ;
} /* s2lei_array */

/*-----------------------------------------------------------------------------------------------
*/

static	void	
i2sc_array	(int *ptr, int index, signed char *buffer, unsigned int count)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = (signed char) (ptr [index]) ;
		index ++ ;
		} ;
} /* i2sc_array */

static	void	
i2uc_array	(int *ptr, int index, unsigned char *buffer, unsigned int count)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = (unsigned char) (ptr [index] + 128) ;
		index ++ ;
		} ;
} /* i2uc_array */

static void	
i2bes_array (int *ptr, int index, short *buffer, unsigned int count)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = H2BE_SHORT (ptr [index]) ;
		index ++ ;
		} ;
} /* i2bes_array */

static void	
i2les_array (int *ptr, int index, short *buffer, unsigned int count)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = H2LE_SHORT (ptr [index]) ;
		index ++ ;
		} ;
} /* i2les_array */

static void	
i2bet_array (int *ptr, int index, tribyte *buffer, unsigned int count)
{	unsigned char	*cptr ;
	int		k, value ;
	cptr = (unsigned char*) buffer ;
	for (k = 0 ; k < count ; k++)
	{	value = ptr [index] ;
		cptr [0] = (unsigned char) ((value & 0xFF0000) >> 16) ;
		cptr [1] = (unsigned char) ((value >> 8) & 0xFF) ;
		cptr [2] = (unsigned char) (value & 0xFF) ;
		index ++ ;
		cptr += 3 ;
		} ;
} /* i2bet_array */

static void	
i2let_array (int *ptr, int index, tribyte *buffer, unsigned int count)
{	unsigned char	*cptr ;
	int		k, value ;
	cptr = (unsigned char*) buffer ;
	for (k = 0 ; k < count ; k++)
	{	value = ptr [index] ;
		cptr [0] = (unsigned char) (value & 0xFF) ;
		cptr [1] = (unsigned char) ((value >> 8) & 0xFF) ;
		cptr [2] = (unsigned char) ((value >> 16) & 0xFF) ;
		index ++ ;
		cptr += 3 ;
		} ;
} /* i2let_array */

/*-----------------------------------------------------------------------------------------------
*/

static	void	
f2sc_array	(float *ptr, int index, signed char *buffer, unsigned int count, float normfact)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = FLOAT_TO_SHORT (ptr [index] * normfact) ;
		index ++ ;
		} ;
} /* f2sc_array */

static	void	
f2uc_array	(float *ptr, int index, unsigned char *buffer, unsigned int count, float normfact)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = FLOAT_TO_SHORT (ptr [index] * normfact) + 128 ;
		index ++ ;
		} ;
} /* f2uc_array */

static void	
f2bes_array (float *ptr, int index, short *buffer, unsigned int count, float normfact)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = H2BE_SHORT (FLOAT_TO_SHORT (ptr [index] * normfact)) ;
		index ++ ;
		} ;
} /* f2bes_array */

static void	
f2les_array (float *ptr, int index, short *buffer, unsigned int count, float normfact)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = H2LE_SHORT (FLOAT_TO_SHORT (ptr [index] * normfact)) ;
		index ++ ;
		} ;
} /* f2les_array */

static void	
f2bet_array (float *ptr, int index, tribyte *buffer, unsigned int count, float normfact)
{	unsigned char	*cptr ;
	int		k, value ;
	cptr = (unsigned char*) buffer ;
	for (k = 0 ; k < count ; k++)
	{	value = FLOAT_TO_INT (ptr [index] * normfact) ;
		cptr [2] = (unsigned char) (value & 0xFF) ;
		cptr [1] = (unsigned char) ((value >> 8) & 0xFF) ;
		cptr [0] = (unsigned char) ((value >> 16) & 0xFF) ;
		index ++ ;
		cptr += 3 ;
		} ;
} /* f2bet_array */

static void	
f2let_array (float *ptr, int index, tribyte *buffer, unsigned int count, float normfact)
{	unsigned char	*cptr ;
	int		k, value ;
	cptr = (unsigned char*) buffer ;	
	for (k = 0 ; k < count ; k++)
	{	value = FLOAT_TO_INT (ptr [index] * normfact) ;
		cptr [0] = (unsigned char) (value & 0xFF) ;
		cptr [1] = (unsigned char) ((value >> 8) & 0xFF) ;
		cptr [2] = (unsigned char) ((value >> 16) & 0xFF) ;
		index ++ ;
		cptr += 3 ;
		} ;
} /* f2let_array */

static void	
f2bei_array (float *ptr, int index, int *buffer, unsigned int count, float normfact)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = H2BE_INT (FLOAT_TO_INT (ptr [index] * normfact)) ;
		index ++ ;
		} ;
} /* f2bei_array */

static 
void	f2lei_array (float *ptr, int index, int *buffer, unsigned int count, float normfact)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = H2LE_INT (FLOAT_TO_INT (ptr [index] * normfact)) ;
		index ++ ;
		} ;
} /* f2lei_array */

/*-----------------------------------------------------------------------------------------------
*/

static	void	
d2sc_array	(double *ptr, int index, signed char *buffer, unsigned int count, double normfact)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = DOUBLE_TO_SHORT (ptr [index] * normfact) ;
		index ++ ;
		} ;
} /* d2sc_array */

static	void	
d2uc_array	(double *ptr, int index, unsigned char *buffer, unsigned int count, double normfact)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = DOUBLE_TO_SHORT (ptr [index] * normfact) + 128 ;
		index ++ ;
		} ;
} /* d2uc_array */

static void	
d2bes_array (double *ptr, int index, short *buffer, unsigned int count, double normfact)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = H2BE_SHORT (DOUBLE_TO_SHORT (ptr [index] * normfact)) ;
		index ++ ;
		} ;
} /* d2bes_array */

static void	
d2les_array (double *ptr, int index, short *buffer, unsigned int count, double normfact)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = H2LE_SHORT (DOUBLE_TO_SHORT (ptr [index] * normfact)) ;
		index ++ ;
		} ;
} /* d2les_array */

static void	
d2bet_array (double *ptr, int index, tribyte *buffer, unsigned int count, double normfact)
{	unsigned char	*cptr ;
	int		k, value ;
	cptr = (unsigned char*) buffer ;
	for (k = 0 ; k < count ; k++)
	{	value = DOUBLE_TO_INT (ptr [index] * normfact) ;
		cptr [2] = (unsigned char) (value & 0xFF) ;
		cptr [1] = (unsigned char) ((value >> 8) & 0xFF) ;
		cptr [0] = (unsigned char) ((value >> 16) & 0xFF) ;
		index ++ ;
		cptr += 3 ;
		} ;
} /* d2bet_array */

static void	
d2let_array (double *ptr, int index, tribyte *buffer, unsigned int count, double normfact)
{	unsigned char	*cptr ;
	int		k, value ;
	cptr = (unsigned char*) buffer ;	
	for (k = 0 ; k < count ; k++)
	{	value = DOUBLE_TO_INT (ptr [index] * normfact) ;
		cptr [0] = (unsigned char) (value & 0xFF) ;
		cptr [1] = (unsigned char) ((value >> 8) & 0xFF) ;
		cptr [2] = (unsigned char) ((value >> 16) & 0xFF) ;
		index ++ ;
		cptr += 3 ;
		} ;
} /* d2let_array */

static void	
d2bei_array (double *ptr, int index, int *buffer, unsigned int count, double normfact)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = H2BE_INT (DOUBLE_TO_INT (ptr [index] * normfact)) ;
		index ++ ;
		} ;
} /* d2bei_array */

static 
void	d2lei_array (double *ptr, int index, int *buffer, unsigned int count, double normfact)
{	int	k ;
	for (k = 0 ; k < count ; k++)
	{	buffer [k] = H2LE_INT (DOUBLE_TO_INT (ptr [index] * normfact)) ;
		index ++ ;
		} ;
} /* d2lei_array */

