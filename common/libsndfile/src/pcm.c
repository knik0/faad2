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


#include	<unistd.h>

#include	"sndfile.h"
#include	"config.h"
#include	"sfendian.h"
#include	"float_cast.h"
#include	"common.h"

/* Important!!! Do not assume that sizeof (tribyte) == 3. Some compilers 
** (Metrowerks CodeWarrior for Mac is one) pad the struct with an extra byte.
*/

typedef	struct
{	char	bytes [3] ;
} tribyte ;

static sf_count_t	pcm_read_sc2s  (SF_PRIVATE *psf, short *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_uc2s  (SF_PRIVATE *psf, short *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_bes2s (SF_PRIVATE *psf, short *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_les2s (SF_PRIVATE *psf, short *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_bet2s (SF_PRIVATE *psf, short *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_let2s (SF_PRIVATE *psf, short *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_bei2s (SF_PRIVATE *psf, short *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_lei2s (SF_PRIVATE *psf, short *ptr, sf_count_t len) ;

static sf_count_t	pcm_read_sc2i  (SF_PRIVATE *psf, int *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_uc2i  (SF_PRIVATE *psf, int *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_bes2i (SF_PRIVATE *psf, int *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_les2i (SF_PRIVATE *psf, int *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_bet2i (SF_PRIVATE *psf, int *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_let2i (SF_PRIVATE *psf, int *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_bei2i (SF_PRIVATE *psf, int *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_lei2i (SF_PRIVATE *psf, int *ptr, sf_count_t len) ;

static sf_count_t	pcm_read_sc2f  (SF_PRIVATE *psf, float *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_uc2f  (SF_PRIVATE *psf, float *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_bes2f (SF_PRIVATE *psf, float *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_les2f (SF_PRIVATE *psf, float *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_bet2f (SF_PRIVATE *psf, float *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_let2f (SF_PRIVATE *psf, float *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_bei2f (SF_PRIVATE *psf, float *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_lei2f (SF_PRIVATE *psf, float *ptr, sf_count_t len) ;

static sf_count_t	pcm_read_sc2d  (SF_PRIVATE *psf, double *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_uc2d  (SF_PRIVATE *psf, double *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_bes2d (SF_PRIVATE *psf, double *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_les2d (SF_PRIVATE *psf, double *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_bet2d (SF_PRIVATE *psf, double *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_let2d (SF_PRIVATE *psf, double *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_bei2d (SF_PRIVATE *psf, double *ptr, sf_count_t len) ;
static sf_count_t	pcm_read_lei2d (SF_PRIVATE *psf, double *ptr, sf_count_t len) ;


static sf_count_t	pcm_write_s2sc  (SF_PRIVATE *psf, short *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_s2uc  (SF_PRIVATE *psf, short *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_s2bes (SF_PRIVATE *psf, short *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_s2les (SF_PRIVATE *psf, short *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_s2bet (SF_PRIVATE *psf, short *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_s2let (SF_PRIVATE *psf, short *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_s2bei (SF_PRIVATE *psf, short *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_s2lei (SF_PRIVATE *psf, short *ptr, sf_count_t len) ;

static sf_count_t	pcm_write_i2sc  (SF_PRIVATE *psf, int *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_i2uc  (SF_PRIVATE *psf, int *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_i2bes (SF_PRIVATE *psf, int *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_i2les (SF_PRIVATE *psf, int *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_i2bet (SF_PRIVATE *psf, int *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_i2let (SF_PRIVATE *psf, int *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_i2bei (SF_PRIVATE *psf, int *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_i2lei (SF_PRIVATE *psf, int *ptr, sf_count_t len) ;

static sf_count_t	pcm_write_f2sc  (SF_PRIVATE *psf, float *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_f2uc  (SF_PRIVATE *psf, float *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_f2bes (SF_PRIVATE *psf, float *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_f2les (SF_PRIVATE *psf, float *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_f2bet (SF_PRIVATE *psf, float *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_f2let (SF_PRIVATE *psf, float *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_f2bei (SF_PRIVATE *psf, float *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_f2lei (SF_PRIVATE *psf, float *ptr, sf_count_t len) ;

static sf_count_t	pcm_write_d2sc  (SF_PRIVATE *psf, double *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_d2uc  (SF_PRIVATE *psf, double *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_d2bes (SF_PRIVATE *psf, double *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_d2les (SF_PRIVATE *psf, double *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_d2bet (SF_PRIVATE *psf, double *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_d2let (SF_PRIVATE *psf, double *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_d2bei (SF_PRIVATE *psf, double *ptr, sf_count_t len) ;
static sf_count_t	pcm_write_d2lei (SF_PRIVATE *psf, double *ptr, sf_count_t len) ;

static	void	sc2s_array	(signed char *buffer, unsigned int count, short *ptr) ;
static	void	uc2s_array	(unsigned char *buffer, unsigned int count, short *ptr) ;

static	void	bet2s_array (tribyte *buffer, unsigned int count, short *ptr) ;
static	void	let2s_array (tribyte *buffer, unsigned int count, short *ptr) ;
static	void	bei2s_array (int *buffer, unsigned int count, short *ptr) ;
static	void	lei2s_array (int *buffer, unsigned int count, short *ptr) ;

static	void	sc2i_array	(signed char *buffer, unsigned int count, int *ptr) ;
static	void	uc2i_array	(unsigned char *buffer, unsigned int count, int *ptr) ;
static	void	bes2i_array (short *buffer, unsigned int count, int *ptr) ;
static	void	les2i_array (short *buffer, unsigned int count, int *ptr) ;
static	void	bet2i_array (tribyte *buffer, unsigned int count, int *ptr) ;
static	void	let2i_array (tribyte *buffer, unsigned int count, int *ptr) ;

static	void	sc2f_array	(signed char *buffer, unsigned int count, float *ptr, float normfact) ;
static	void	uc2f_array	(unsigned char *buffer, unsigned int count, float *ptr, float normfact) ;
static	void	bes2f_array (short *buffer, unsigned int count, float *ptr, float normfact) ;
static	void	les2f_array (short *buffer, unsigned int count, float *ptr, float normfact) ;
static	void	bet2f_array (tribyte *buffer, unsigned int count, float *ptr, float normfact) ;
static	void	let2f_array (tribyte *buffer, unsigned int count, float *ptr, float normfact) ;
static	void	bei2f_array (int *buffer, unsigned int count, float *ptr, float normfact) ;
static	void	lei2f_array (int *buffer, unsigned int count, float *ptr, float normfact) ;

static	void	sc2d_array	(signed char *buffer, unsigned int count, double *ptr, double normfact) ;
static	void	uc2d_array	(unsigned char *buffer, unsigned int count, double *ptr, double normfact) ;
static	void	bes2d_array (short *buffer, unsigned int count, double *ptr, double normfact) ;
static	void	les2d_array (short *buffer, unsigned int count, double *ptr, double normfact) ;
static	void	bet2d_array (tribyte *buffer, unsigned int count, double *ptr, double normfact) ;
static	void	let2d_array (tribyte *buffer, unsigned int count, double *ptr, double normfact) ;
static	void	bei2d_array (int *buffer, unsigned int count, double *ptr, double normfact) ;
static	void	lei2d_array (int *buffer, unsigned int count, double *ptr, double normfact) ;


static	void	s2sc_array	(short *ptr, signed char *buffer, unsigned int count) ;
static	void	s2uc_array	(short *ptr, unsigned char *buffer, unsigned int count) ;
static	void	s2bet_array (short *ptr, tribyte *buffer, unsigned int count) ;
static	void	s2let_array (short *ptr, tribyte *buffer, unsigned int count) ;
static	void	s2bei_array (short *ptr, int *buffer, unsigned int count) ;
static	void	s2lei_array (short *ptr, int *buffer, unsigned int count) ;

static	void	i2sc_array	(int *ptr, signed char *buffer, unsigned int count) ;
static	void	i2uc_array	(int *ptr, unsigned char *buffer, unsigned int count) ;
static	void	i2bes_array (int *ptr, short *buffer, unsigned int count) ;
static	void	i2les_array (int *ptr, short *buffer, unsigned int count) ;
static	void	i2bet_array (int *ptr, tribyte *buffer, unsigned int count) ;
static	void	i2let_array (int *ptr, tribyte *buffer, unsigned int count) ;

static	void	f2sc_array	(float *ptr, signed char *buffer, unsigned int count, float normfact) ;
static	void	f2uc_array	(float *ptr, unsigned char *buffer, unsigned int count, float normfact) ;
static	void	f2bes_array (float *ptr, short *buffer, unsigned int count, float normfact) ;
static	void	f2les_array (float *ptr, short *buffer, unsigned int count, float normfact) ;
static	void	f2bet_array (float *ptr, tribyte *buffer, unsigned int count, float normfact) ;
static	void	f2let_array (float *ptr, tribyte *buffer, unsigned int count, float normfact) ;
static 	void	f2bei_array (float *ptr, int *buffer, unsigned int count, float normfact) ;
static 	void	f2lei_array (float *ptr, int *buffer, unsigned int count, float normfact) ;

static	void	d2sc_array	(double *ptr, signed char *buffer, unsigned int count, double normfact) ;
static	void	d2uc_array	(double *ptr, unsigned char *buffer, unsigned int count, double normfact) ;
static	void	d2bes_array (double *ptr, short *buffer, unsigned int count, double normfact) ;
static	void	d2les_array (double *ptr, short *buffer, unsigned int count, double normfact) ;
static	void	d2bet_array (double *ptr, tribyte *buffer, unsigned int count, double normfact) ;
static	void	d2let_array (double *ptr, tribyte *buffer, unsigned int count, double normfact) ;
static 	void	d2bei_array (double *ptr, int *buffer, unsigned int count, double normfact) ;
static 	void	d2lei_array (double *ptr, int *buffer, unsigned int count, double normfact) ;

/*-----------------------------------------------------------------------------------------------
*/

int
pcm_init (SF_PRIVATE *psf)
{
	psf->blockwidth = psf->bytewidth * psf->sf.channels ;

	if (psf->mode == SFM_READ || psf->mode == SFM_RDWR)
	{	switch (psf->bytewidth * 0x10000 + psf->endian + psf->chars)
		{	case (0x10000 + SF_ENDIAN_BIG + SF_CHARS_SIGNED) :
			case (0x10000 + SF_ENDIAN_LITTLE + SF_CHARS_SIGNED) :
					psf->read_short  = pcm_read_sc2s ;
					psf->read_int    = pcm_read_sc2i ;
					psf->read_float  = pcm_read_sc2f ;
					psf->read_double = pcm_read_sc2d ;
					break ;
			case (0x10000 + SF_ENDIAN_BIG + SF_CHARS_UNSIGNED) :
			case (0x10000 + SF_ENDIAN_LITTLE + SF_CHARS_UNSIGNED) :
					psf->read_short  = pcm_read_uc2s ;
					psf->read_int    = pcm_read_uc2i ;
					psf->read_float  = pcm_read_uc2f ;
					psf->read_double = pcm_read_uc2d ;
					break ;
	
			case  (2 * 0x10000 + SF_ENDIAN_BIG) :
					psf->read_short  = pcm_read_bes2s ;
					psf->read_int    = pcm_read_bes2i ;
					psf->read_float  = pcm_read_bes2f ;
					psf->read_double = pcm_read_bes2d ;
					break ;
			case  (3 * 0x10000 + SF_ENDIAN_BIG) :
					psf->read_short  = pcm_read_bet2s ;
					psf->read_int    = pcm_read_bet2i ;
					psf->read_float  = pcm_read_bet2f ;
					psf->read_double = pcm_read_bet2d ;
					break ;
			case  (4 * 0x10000 + SF_ENDIAN_BIG) :
					psf->read_short  = pcm_read_bei2s ;
					psf->read_int    = pcm_read_bei2i ;
					psf->read_float  = pcm_read_bei2f ;
					psf->read_double = pcm_read_bei2d ;
					break ;
					
			case  (2 * 0x10000 + SF_ENDIAN_LITTLE) :
					psf->read_short  = pcm_read_les2s ;
					psf->read_int    = pcm_read_les2i ;
					psf->read_float  = pcm_read_les2f ;
					psf->read_double = pcm_read_les2d ;
					break ;
			case  (3 * 0x10000 + SF_ENDIAN_LITTLE) :
					psf->read_short  = pcm_read_let2s ;
					psf->read_int    = pcm_read_let2i ;
					psf->read_float  = pcm_read_let2f ;
					psf->read_double = pcm_read_let2d ;
					break ;
			case  (4 * 0x10000 + SF_ENDIAN_LITTLE) :
					psf->read_short  = pcm_read_lei2s ;
					psf->read_int    = pcm_read_lei2i ;
					psf->read_float  = pcm_read_lei2f ;
					psf->read_double = pcm_read_lei2d ;
					break ;
			default : return SFE_UNIMPLEMENTED ;
			} ;
		} ;

	if (psf->mode == SFM_WRITE || psf->mode == SFM_RDWR)
	{	switch (psf->bytewidth * 0x10000 + psf->endian + psf->chars)
		{	case (0x10000 + SF_ENDIAN_BIG + SF_CHARS_SIGNED) :
			case (0x10000 + SF_ENDIAN_LITTLE + SF_CHARS_SIGNED) :
					psf->write_short  = pcm_write_s2sc ;
					psf->write_int    = pcm_write_i2sc ;
					psf->write_float  = pcm_write_f2sc ;
					psf->write_double = pcm_write_d2sc ;
					break ;
			case (0x10000 + SF_ENDIAN_BIG + SF_CHARS_UNSIGNED) :
			case (0x10000 + SF_ENDIAN_LITTLE + SF_CHARS_UNSIGNED) :
					psf->write_short  = pcm_write_s2uc ;
					psf->write_int    = pcm_write_i2uc ;
					psf->write_float  = pcm_write_f2uc ;
					psf->write_double = pcm_write_d2uc ;
					break ;
	
			case  (2 * 0x10000 + SF_ENDIAN_BIG) :
					psf->write_short  = pcm_write_s2bes ;
					psf->write_int    = pcm_write_i2bes ;
					psf->write_float  = pcm_write_f2bes ;
					psf->write_double = pcm_write_d2bes ;
					break ;
					
			case  (3 * 0x10000 + SF_ENDIAN_BIG) :
					psf->write_short  = pcm_write_s2bet ;
					psf->write_int    = pcm_write_i2bet ;
					psf->write_float  = pcm_write_f2bet ;
					psf->write_double = pcm_write_d2bet ;
					break ;
					
			case  (4 * 0x10000 + SF_ENDIAN_BIG) :
					psf->write_short  = pcm_write_s2bei ;
					psf->write_int    = pcm_write_i2bei ;
					psf->write_float  = pcm_write_f2bei ;
					psf->write_double = pcm_write_d2bei ;
					break ;
					
			case  (2 * 0x10000 + SF_ENDIAN_LITTLE) :
					psf->write_short  = pcm_write_s2les ;
					psf->write_int    = pcm_write_i2les ;
					psf->write_float  = pcm_write_f2les ;
					psf->write_double = pcm_write_d2les ;
					break ;
					
			case  (3 * 0x10000 + SF_ENDIAN_LITTLE) :
					psf->write_short  = pcm_write_s2let ;
					psf->write_int    = pcm_write_i2let ;
					psf->write_float  = pcm_write_f2let ;
					psf->write_double = pcm_write_d2let ;
					break ;
					
			case  (4 * 0x10000 + SF_ENDIAN_LITTLE) :
					psf->write_short  = pcm_write_s2lei ;
					psf->write_int    = pcm_write_i2lei ;
					psf->write_float  = pcm_write_f2lei ;
					psf->write_double = pcm_write_d2lei ;
					break ;
					
			default : return SFE_UNIMPLEMENTED ;
			} ;
			
		} ;

	psf->filelength = psf_get_filelen (psf->filedes) ;
	psf->datalength = (psf->dataend) ? psf->dataend - psf->dataoffset : 
							psf->filelength - psf->dataoffset ;
	psf->sf.samples = psf->datalength / psf->blockwidth ;

	return 0 ;
} /* pcm_init */

/*-----------------------------------------------------------------------------------------------
*/

static sf_count_t		
pcm_read_sc2s (SF_PRIVATE *psf, short *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		sc2s_array ((signed char*) (psf->buffer), thisread / psf->bytewidth, ptr + index) ;
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

static sf_count_t
pcm_read_uc2s (SF_PRIVATE *psf, short *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		uc2s_array ((unsigned char*) (psf->buffer), thisread / psf->bytewidth, ptr + index) ;
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

static sf_count_t
pcm_read_bes2s (SF_PRIVATE *psf, short *ptr, sf_count_t len)
{	int		total ;

	total = psf_fread (ptr, 1, len * sizeof (short), psf->filedes) ;
	if (CPU_IS_LITTLE_ENDIAN)
		endswap_short_array (ptr, len) ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_READ ;
	
	return total ;
} /* pcm_read_bes2s */

static sf_count_t
pcm_read_les2s (SF_PRIVATE *psf, short *ptr, sf_count_t len)
{	int		total ;

	total = psf_fread (ptr, 1, len * sizeof (short), psf->filedes) ;
	if (CPU_IS_BIG_ENDIAN)
		endswap_short_array (ptr, len) ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_READ ;
	
	return total ;
} /* pcm_read_les2s */

static sf_count_t
pcm_read_bet2s (SF_PRIVATE *psf, short *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		bet2s_array ((tribyte*) (psf->buffer), thisread / psf->bytewidth, ptr + index) ;
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

static sf_count_t
pcm_read_let2s (SF_PRIVATE *psf, short *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		let2s_array ((tribyte*) (psf->buffer), thisread / psf->bytewidth, ptr + index) ;
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

static sf_count_t
pcm_read_bei2s (SF_PRIVATE *psf, short *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		bei2s_array ((int*) (psf->buffer), thisread / psf->bytewidth, ptr + index) ;
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

static sf_count_t
pcm_read_lei2s (SF_PRIVATE *psf, short *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		lei2s_array ((int*) (psf->buffer), thisread / psf->bytewidth, ptr + index) ;
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

static sf_count_t
pcm_read_sc2i (SF_PRIVATE *psf, int *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		sc2i_array ((signed char*) (psf->buffer), thisread / psf->bytewidth, ptr + index) ;
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

static sf_count_t
pcm_read_uc2i (SF_PRIVATE *psf, int *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		uc2i_array ((unsigned char*) (psf->buffer), thisread / psf->bytewidth, ptr + index) ;
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

static sf_count_t
pcm_read_bes2i (SF_PRIVATE *psf, int *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		bes2i_array ((short*) (psf->buffer), thisread / psf->bytewidth, ptr + index) ;
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

static sf_count_t
pcm_read_les2i (SF_PRIVATE *psf, int *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		les2i_array ((short*) (psf->buffer), thisread / psf->bytewidth, ptr + index) ;
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

static sf_count_t
pcm_read_bet2i (SF_PRIVATE *psf, int *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		bet2i_array ((tribyte*) (psf->buffer), thisread / psf->bytewidth, ptr + index) ;
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

static sf_count_t
pcm_read_let2i (SF_PRIVATE *psf, int *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		let2i_array ((tribyte*) (psf->buffer), thisread / psf->bytewidth, ptr + index) ;
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

static sf_count_t
pcm_read_bei2i (SF_PRIVATE *psf, int *ptr, sf_count_t len)
{	int		total ;

	total = psf_fread (ptr, 1, len * sizeof (int), psf->filedes) ;
	if (CPU_IS_LITTLE_ENDIAN)
		endswap_int_array	(ptr, len) ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_READ ;
	
	return total ;
} /* pcm_read_bei2i */

static sf_count_t
pcm_read_lei2i (SF_PRIVATE *psf, int *ptr, sf_count_t len)
{	int		total ;

	total = psf_fread (ptr, 1, len * sizeof (int), psf->filedes) ;
	if (CPU_IS_BIG_ENDIAN)
		endswap_int_array	(ptr, len) ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_READ ;
	
	return total ;
} /* pcm_read_lei2i */

/*-----------------------------------------------------------------------------------------------
*/

static sf_count_t
pcm_read_sc2f (SF_PRIVATE *psf, float *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;

	normfact = (psf->norm_float == SF_TRUE) ? 1.0 / ((float) 0x80) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		sc2f_array ((signed char*) (psf->buffer), thisread / psf->bytewidth, ptr + index, normfact) ;
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

static sf_count_t
pcm_read_uc2f (SF_PRIVATE *psf, float *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;

	normfact = (psf->norm_float == SF_TRUE) ? 1.0 / ((float) 0x80) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		uc2f_array ((unsigned char*) (psf->buffer), thisread / psf->bytewidth, ptr + index, normfact) ;
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

static sf_count_t
pcm_read_bes2f (SF_PRIVATE *psf, float *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;

	normfact = (psf->norm_float == SF_TRUE) ? 1.0 / ((float) 0x8000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		bes2f_array ((short*) (psf->buffer), thisread / psf->bytewidth, ptr + index, normfact) ;
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

static sf_count_t
pcm_read_les2f (SF_PRIVATE *psf, float *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;

	normfact = (psf->norm_float == SF_TRUE) ? 1.0 / ((float) 0x8000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		les2f_array ((short*) (psf->buffer), thisread / psf->bytewidth, ptr + index, normfact) ;
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

static sf_count_t
pcm_read_bet2f (SF_PRIVATE *psf, float *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;

	normfact = (psf->norm_float == SF_TRUE) ? 1.0 / ((float) 0x800000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		bet2f_array ((tribyte*) (psf->buffer), thisread / psf->bytewidth, ptr + index, normfact) ;
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

static sf_count_t
pcm_read_let2f (SF_PRIVATE *psf, float *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;

	normfact = (psf->norm_float == SF_TRUE) ? 1.0 / ((float) 0x800000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		let2f_array ((tribyte*) (psf->buffer), thisread / psf->bytewidth, ptr + index, normfact) ;
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

static sf_count_t
pcm_read_bei2f (SF_PRIVATE *psf, float *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;

	normfact = (psf->norm_float == SF_TRUE) ? 1.0 / ((float) 0x80000000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		bei2f_array ((int*) (psf->buffer), thisread / psf->bytewidth, ptr + index, normfact) ;
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

static sf_count_t
pcm_read_lei2f (SF_PRIVATE *psf, float *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;

	normfact = (psf->norm_float == SF_TRUE) ? 1.0 / ((float) 0x80000000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		lei2f_array ((int*) (psf->buffer), thisread / psf->bytewidth, ptr + index, normfact) ;
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

static sf_count_t
pcm_read_sc2d (SF_PRIVATE *psf, double *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double		normfact ;

	normfact = (psf->norm_double == SF_TRUE) ? 1.0 / ((double) 0x80) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		sc2d_array ((signed char*) (psf->buffer), thisread / psf->bytewidth, ptr + index, normfact) ;
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

static sf_count_t
pcm_read_uc2d (SF_PRIVATE *psf, double *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double		normfact ;

	normfact = (psf->norm_double == SF_TRUE) ? 1.0 / ((double) 0x80) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		uc2d_array ((unsigned char*) (psf->buffer), thisread / psf->bytewidth, ptr + index, normfact) ;
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

static sf_count_t
pcm_read_bes2d (SF_PRIVATE *psf, double *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double		normfact ;

	normfact = (psf->norm_double == SF_TRUE) ? 1.0 / ((double) 0x8000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		bes2d_array ((short*) (psf->buffer), thisread / psf->bytewidth, ptr + index, normfact) ;
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

static sf_count_t
pcm_read_les2d (SF_PRIVATE *psf, double *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double		normfact ;

	normfact = (psf->norm_double == SF_TRUE) ? 1.0 / ((double) 0x8000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		les2d_array ((short*) (psf->buffer), thisread / psf->bytewidth, ptr + index, normfact) ;
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

static sf_count_t
pcm_read_bet2d (SF_PRIVATE *psf, double *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double		normfact ;

	normfact = (psf->norm_double == SF_TRUE) ? 1.0 / ((double) 0x800000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		bet2d_array ((tribyte*) (psf->buffer), thisread / psf->bytewidth, ptr + index, normfact) ;
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

static sf_count_t
pcm_read_let2d (SF_PRIVATE *psf, double *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double		normfact ;

	normfact = (psf->norm_double == SF_TRUE) ? 1.0 / ((double) 0x800000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		let2d_array ((tribyte*) (psf->buffer), thisread / psf->bytewidth, ptr + index, normfact) ;
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

static sf_count_t
pcm_read_bei2d (SF_PRIVATE *psf, double *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double		normfact ;

	normfact = (psf->norm_double == SF_TRUE) ? 1.0 / ((double) 0x80000000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		bei2d_array ((int*) (psf->buffer), thisread / psf->bytewidth, ptr + index, normfact) ;
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

static sf_count_t
pcm_read_lei2d (SF_PRIVATE *psf, double *ptr, sf_count_t len)
{	unsigned int readcount, thisread ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double		normfact ;

	normfact = (psf->norm_double == SF_TRUE) ? 1.0 / ((double) 0x80000000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	readcount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		thisread = psf_fread (psf->buffer, 1, readcount, psf->filedes) ;
		lei2d_array ((int*) (psf->buffer), thisread / psf->bytewidth, ptr + index, normfact) ;
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

static sf_count_t
pcm_write_s2sc	(SF_PRIVATE *psf, short *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		s2sc_array (ptr + index, (signed char*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_s2sc */

static sf_count_t
pcm_write_s2uc	(SF_PRIVATE *psf, short *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		s2uc_array (ptr + index, (unsigned char*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_s2uc */

static sf_count_t
pcm_write_s2bes	(SF_PRIVATE *psf, short *ptr, sf_count_t len)
{	int		total ;

	if (CPU_IS_LITTLE_ENDIAN)
		endswap_short_array (ptr, len) ;
	total = psf_fwrite (ptr, 1, len * sizeof (short), psf->filedes) ;
	if (CPU_IS_LITTLE_ENDIAN)
		endswap_short_array (ptr, len) ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_WRITE ;
	
	return total ;
} /* pcm_write_s2bes */

static sf_count_t
pcm_write_s2les	(SF_PRIVATE *psf, short *ptr, sf_count_t len)
{	int		total ;

	if (CPU_IS_BIG_ENDIAN)
		endswap_short_array (ptr, len) ;
	total = psf_fwrite (ptr, 1, len * sizeof (short), psf->filedes) ;
	if (CPU_IS_BIG_ENDIAN)
		endswap_short_array (ptr, len) ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_WRITE ;
	
	return total ;
} /* pcm_write_s2les */

static sf_count_t
pcm_write_s2bet	(SF_PRIVATE *psf, short *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		s2bet_array (ptr + index, (tribyte*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_s2bet */

static sf_count_t
pcm_write_s2let	(SF_PRIVATE *psf, short *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		s2let_array (ptr + index, (tribyte*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_s2let */

static sf_count_t
pcm_write_s2bei	(SF_PRIVATE *psf, short *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		s2bei_array (ptr + index, (int*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_s2bei */

static sf_count_t
pcm_write_s2lei	(SF_PRIVATE *psf, short *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		s2lei_array (ptr + index, (int*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_s2lei */

/*-----------------------------------------------------------------------------------------------
*/

static sf_count_t
pcm_write_i2sc	(SF_PRIVATE *psf, int *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		i2sc_array (ptr + index, (signed char*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_i2sc */

static sf_count_t
pcm_write_i2uc	(SF_PRIVATE *psf, int *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		i2uc_array (ptr + index, (unsigned char*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_i2uc */

static sf_count_t
pcm_write_i2bes	(SF_PRIVATE *psf, int *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		i2bes_array (ptr + index, (short*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_i2bes */

static sf_count_t
pcm_write_i2les	(SF_PRIVATE *psf, int *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		i2les_array (ptr + index, (short*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_i2les */

static sf_count_t
pcm_write_i2bet	(SF_PRIVATE *psf, int *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		i2bet_array (ptr + index, (tribyte*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_i2bet */

static sf_count_t
pcm_write_i2let	(SF_PRIVATE *psf, int *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		i2let_array (ptr + index, (tribyte*) (psf->buffer), writecount / psf->bytewidth) ;
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
} /* pcm_write_i2les */

static sf_count_t
pcm_write_i2bei	(SF_PRIVATE *psf, int *ptr, sf_count_t len)
{	int		total ;

	if (CPU_IS_BIG_ENDIAN)
		total = psf_fwrite (ptr, 1, len * sizeof (int), psf->filedes) ;
	else
	{	endswap_int_array (ptr, len) ;
		total = psf_fwrite (ptr, 1, len * sizeof (int), psf->filedes) ;
		endswap_int_array (ptr, len) ;
		} ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_WRITE ;
	
	return total ;
} /* pcm_write_i2bei */

static sf_count_t
pcm_write_i2lei	(SF_PRIVATE *psf, int *ptr, sf_count_t len)
{	int		total ;

	if (CPU_IS_LITTLE_ENDIAN)
		total = psf_fwrite (ptr, 1, len * sizeof (int), psf->filedes) ;
	else
	{	endswap_int_array (ptr, len) ;
		total = psf_fwrite (ptr, 1, len * sizeof (int), psf->filedes) ;
		endswap_int_array (ptr, len) ;
		} ;

	total /= psf->bytewidth ;
	if (total < len)
		psf->error = SFE_SHORT_WRITE ;
	
	return total ;
} /* pcm_write_i2lei */

/*-----------------------------------------------------------------------------------------------
*/

static sf_count_t
pcm_write_f2sc	(SF_PRIVATE *psf, float *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;
	
	normfact = (psf->norm_float == SF_TRUE) ? ((float) 0x80) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		f2sc_array (ptr + index, (signed char*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_f2sc */

static sf_count_t
pcm_write_f2uc	(SF_PRIVATE *psf, float *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;
	
	normfact = (psf->norm_float == SF_TRUE) ? ((float) 0x80) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		f2uc_array (ptr + index, (unsigned char*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_f2uc */

static sf_count_t
pcm_write_f2bes	(SF_PRIVATE *psf, float *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;
	
	normfact = (psf->norm_float == SF_TRUE) ? ((float) 0x8000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		f2bes_array (ptr + index, (short*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_f2bes */

static sf_count_t
pcm_write_f2les	(SF_PRIVATE *psf, float *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;
	
	normfact = (psf->norm_float == SF_TRUE) ? ((float) 0x8000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		f2les_array (ptr + index, (short*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_f2les */

static sf_count_t
pcm_write_f2let	(SF_PRIVATE *psf, float *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;
	
	normfact = (psf->norm_float == SF_TRUE) ? ((float) 0x800000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		f2let_array (ptr + index, (tribyte*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_f2les */

static sf_count_t
pcm_write_f2bet	(SF_PRIVATE *psf, float *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;
	
	normfact = (psf->norm_float == SF_TRUE) ? ((float) 0x800000) : 1.0 ;


	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		f2bet_array (ptr + index, (tribyte*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_f2bes */

static sf_count_t
pcm_write_f2bei	(SF_PRIVATE *psf, float *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;
	
	normfact = (psf->norm_float == SF_TRUE) ? ((float) 0x80000000) : 1.0 ;


	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		f2bei_array (ptr + index, (int*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_f2bei */

static sf_count_t
pcm_write_f2lei	(SF_PRIVATE *psf, float *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	float	normfact ;
	
	normfact = (psf->norm_float == SF_TRUE) ? ((float) 0x80000000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		f2lei_array (ptr + index, (int*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_f2lei */

/*-----------------------------------------------------------------------------------------------
*/

static sf_count_t
pcm_write_d2sc	(SF_PRIVATE *psf, double *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double	normfact ;
	
	normfact = (psf->norm_double == SF_TRUE) ? ((double) 0x80) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		d2sc_array (ptr + index, (signed char*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_d2sc */

static sf_count_t
pcm_write_d2uc	(SF_PRIVATE *psf, double *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double	normfact ;
	
	normfact = (psf->norm_double == SF_TRUE) ? ((double) 0x80) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		d2uc_array (ptr + index, (unsigned char*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_d2uc */

static sf_count_t
pcm_write_d2bes	(SF_PRIVATE *psf, double *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double	normfact ;
	
	normfact = (psf->norm_double == SF_TRUE) ? ((double) 0x8000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		d2bes_array (ptr + index, (short*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_d2bes */

static sf_count_t
pcm_write_d2les	(SF_PRIVATE *psf, double *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double	normfact ;
	
	normfact = (psf->norm_double == SF_TRUE) ? ((double) 0x8000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		d2les_array (ptr + index, (short*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_d2les */

static sf_count_t
pcm_write_d2let	(SF_PRIVATE *psf, double *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double	normfact ;
	
	normfact = (psf->norm_double == SF_TRUE) ? ((double) 0x800000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		d2let_array (ptr + index, (tribyte*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_d2les */

static sf_count_t
pcm_write_d2bet	(SF_PRIVATE *psf, double *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double	normfact ;
	
	normfact = (psf->norm_double == SF_TRUE) ? ((double) 0x800000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		d2bet_array (ptr + index, (tribyte*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_d2bes */

static sf_count_t
pcm_write_d2bei	(SF_PRIVATE *psf, double *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double	normfact ;
	
	normfact = (psf->norm_double == SF_TRUE) ? ((double) 0x80000000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		d2bei_array (ptr + index, (int*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_d2bei */

static sf_count_t
pcm_write_d2lei	(SF_PRIVATE *psf, double *ptr, sf_count_t len)
{	sf_count_t	writecount, thiswrite ;
	sf_count_t	bytecount, bufferlen ;
	int		index = 0, total = 0 ;
	double	normfact ;
	
	normfact = (psf->norm_double == SF_TRUE) ? ((double) 0x80000000) : 1.0 ;

	bufferlen = SF_BUFFER_LEN - (SF_BUFFER_LEN % psf->blockwidth) ;
	bytecount = len * psf->bytewidth ;
	while (bytecount > 0)
	{	writecount = (bytecount >= bufferlen) ? bufferlen : bytecount ;
		d2lei_array (ptr + index, (int*) (psf->buffer), writecount / psf->bytewidth, normfact) ;
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
} /* pcm_write_d2lei */

/*-----------------------------------------------------------------------------------------------
*/

static	void	
sc2s_array	(signed char *buffer, unsigned int count, short *ptr)
{	while (count)
	{	count -- ;
		ptr [count] = buffer [count] << 8 ;
		} ;
} /* sc2s_array */

static	void	
uc2s_array	(unsigned char *buffer, unsigned int count, short *ptr)
{	while (count)
	{	count -- ;
		ptr [count] = (((short) buffer [count]) - 0x80) << 8 ;
		} ;
} /* uc2s_array */

static void	
bet2s_array (tribyte *buffer, unsigned int count, short *ptr)
{	unsigned char	*cptr ;

	cptr = ((unsigned char*) buffer) + 3 * count ;
	while (count)
	{	count -- ;
		cptr -= 3 ;	
		ptr [count] = (cptr [0] << 8) + cptr [1] ;
		} ;
} /* bet2s_array */

static void	
let2s_array (tribyte *buffer, unsigned int count, short *ptr)
{	unsigned char	*cptr ;

	cptr = ((unsigned char*) buffer) + 3 * count ;
	while (count)
	{	count -- ;
		cptr -= 3 ;	
		ptr [count] = cptr [1] + (cptr [2] << 8) ;
		} ;
} /* let2s_array */

static void	
bei2s_array (int *buffer, unsigned int count, short *ptr)
{	unsigned char	*cptr ;

	cptr = ((unsigned char*) buffer) + 4 * count ;
	while (count)
	{	count -- ;
		cptr -= 4 ;
		ptr [count] = (cptr [0] << 8) + cptr [1] ;
		} ;
} /* bei2s_array */

static void	
lei2s_array (int *buffer, unsigned int count, short *ptr)
{	unsigned char	*cptr ;

	cptr = ((unsigned char*) buffer) + 4 * count ;
	while (count)
	{	count -- ;
		cptr -= 4 ;
		ptr [count] = cptr [2] + (cptr [3] << 8) ;
		} ;
} /* lei2s_array */


/*-----------------------------------------------------------------------------------------------
*/

static	void	
sc2i_array	(signed char *buffer, unsigned int count, int *ptr)
{	while (count)
	{	count -- ;
		ptr [count] = ((int) buffer [count]) << 24 ;
		} ;
} /* sc2i_array */

static	void	
uc2i_array	(unsigned char *buffer, unsigned int count, int *ptr)
{	while (count)
	{	count -- ;
		ptr [count] = (((int) buffer [count]) - 128) << 24 ;
		} ;
} /* uc2i_array */

static void	
bes2i_array (short *buffer, unsigned int count, int *ptr)
{	unsigned char	*cptr ;

	cptr = ((unsigned char*) buffer) + 2 * count ;
	while (count)
	{	count -- ;
		cptr -= 2 ;	
		ptr [count] = (cptr [0] << 24) + (cptr [1] << 16) ;
		} ;
} /* bes2i_array */

static void	
les2i_array (short *buffer, unsigned int count, int *ptr)
{	unsigned char	*cptr ;

	cptr = ((unsigned char*) buffer) + 2 * count ;
	while (count)
	{	count -- ;
		cptr -= 2 ;	
		ptr [count] = (cptr [0] << 16) + (cptr [1] << 24) ;
		} ;
} /* les2i_array */

static void	
bet2i_array (tribyte *buffer, unsigned int count, int *ptr)
{	unsigned char	*cptr ;

	cptr = ((unsigned char*) buffer) + 3 * count ;
	while (count)
	{	count -- ;
		cptr -= 3 ;	
		ptr [count] = (cptr [0] << 24) + (cptr [1] << 16) + (cptr [2] << 8) ;
		} ;
} /* bet2i_array */

static void	
let2i_array (tribyte *buffer, unsigned int count, int *ptr)
{	unsigned char	*cptr ;

	cptr = ((unsigned char*) buffer) + 3 * count ;
	while (count)
	{	count -- ;
		cptr -= 3 ;
		ptr [count] = (cptr [0] << 8) + (cptr [1] << 16) + (cptr [2] << 24) ;
		} ;
} /* let2i_array */

/*-----------------------------------------------------------------------------------------------
*/


static	void	
sc2f_array	(signed char *buffer, unsigned int count, float *ptr, float normfact)
{	while (count)
	{	count -- ;
		ptr [count] = ((float) buffer [count]) * normfact ;
		} ;
} /* sc2f_array */

static	void	
uc2f_array	(unsigned char *buffer, unsigned int count, float *ptr, float normfact)
{	while (count)
	{	count -- ;
		ptr [count] = (((int) buffer [count]) - 128) * normfact ;
		} ;
} /* uc2f_array */

static void	
bes2f_array (short *buffer, unsigned int count, float *ptr, float normfact)
{	unsigned char	*cptr ;
	short			value;

	cptr = ((unsigned char*) buffer) + 2 * count ;
	while (count)
	{	count -- ;
		cptr -= 2 ;
		value = (cptr [0] << 8) + cptr [1] ;
		ptr [count] = ((float) value) * normfact ;
		} ;
} /* bes2f_array */

static void	
les2f_array (short *buffer, unsigned int count, float *ptr, float normfact)
{	unsigned char	*cptr ;
	short			value;

	cptr = ((unsigned char*) buffer) + 2 * count ;
	while (count)
	{	count -- ;
		cptr -= 2 ;
		value = cptr [0] + (cptr [1] << 8) ;
		ptr [count] = ((float) value) * normfact ;
		} ;
} /* les2f_array */

static void	
bet2f_array (tribyte *buffer, unsigned int count, float *ptr, float normfact)
{	unsigned char	*cptr ;
	int 	value;

	cptr = ((unsigned char*) buffer) + 3 * count ;
	while (count)
	{	count -- ;
		cptr -= 3 ;	
		value = (cptr [0] << 24) + (cptr [1] << 16) + (cptr [2] << 8) ;
		ptr [count] = ((float) (value >> 8)) * normfact ;
		} ;
} /* bet2f_array */

static void	
let2f_array (tribyte *buffer, unsigned int count, float *ptr, float normfact)
{	unsigned char	*cptr ;
	int 	value;

	cptr = ((unsigned char*) buffer) + 3 * count ;
	while (count)
	{	count -- ;
		cptr -= 3 ;	
		value = (cptr [0] << 8) + (cptr [1] << 16) + (cptr [2] << 24) ;
		ptr [count] = ((float) (value >> 8)) * normfact ;
		} ;
} /* let2f_array */

static void	
lei2f_array (int *buffer, unsigned int count, float *ptr, float normfact)
{	unsigned char	*cptr ;
	int 			value;

	cptr = ((unsigned char*) buffer) + 4 * count ;
	while (count)
	{	count -- ;
		cptr -= 4 ;
		value = cptr [0] + (cptr [1] << 8) + (cptr [2] << 16) + (cptr [3] << 24) ;
		ptr [count] = ((float) value) * normfact ;
		} ;
} /* lei2f_array */

static void	
bei2f_array (int *buffer, unsigned int count, float *ptr, float normfact)
{	unsigned char	*cptr ;
	int 			value;

	cptr = ((unsigned char*) buffer) + 4 * count ;
	while (count)
	{	count -- ;
		cptr -= 4 ;
		value = (cptr [0] << 24) + (cptr [1] << 16) + (cptr [2] << 8) + cptr [3] ;
		ptr [count] = ((float) value) * normfact ;
		} ;
} /* bei2f_array */

/*-----------------------------------------------------------------------------------------------
*/

static	void	
sc2d_array	(signed char *buffer, unsigned int count, double *ptr, double normfact)
{	while (count)
	{	count -- ;
		ptr [count] = ((double) buffer [count]) * normfact ;
		} ;
} /* sc2d_array */

static	void	
uc2d_array	(unsigned char *buffer, unsigned int count, double *ptr, double normfact)
{	while (count)
	{	count -- ;
		ptr [count] = (((int) buffer [count]) - 128) * normfact ;
		} ;
} /* uc2d_array */

static void	
bes2d_array (short *buffer, unsigned int count, double *ptr, double normfact)
{	unsigned char	*cptr ;
	short			value;

	cptr = ((unsigned char*) buffer) + 2 * count ;
	while (count)
	{	count -- ;
		cptr -= 2 ;
		value = (cptr [0] << 8) + cptr [1] ;
		ptr [count] = ((double) value) * normfact ;
		} ;
} /* bes2d_array */

static void	
les2d_array (short *buffer, unsigned int count, double *ptr, double normfact)
{	unsigned char	*cptr ;
	short			value;

	cptr = ((unsigned char*) buffer) + 2 * count ;
	while (count)
	{	count -- ;
		cptr -= 2 ;
		value = cptr [0] + (cptr [1] << 8) ;
		ptr [count] = ((double) value) * normfact ;
		} ;
} /* les2d_array */

static void	
bet2d_array (tribyte *buffer, unsigned int count, double *ptr, double normfact)
{	unsigned char	*cptr ;
	int 	value;
	
	cptr = ((unsigned char*) buffer) + 3 * count ;
	while (count)
	{	count -- ;
		cptr -= 3 ;	
		value = (cptr [0] << 24) + (cptr [1] << 16) + (cptr [2] << 8) ;
		ptr [count] = ((double) (value >> 8)) * normfact ;
		} ;
} /* bet2d_array */

static void	
let2d_array (tribyte *buffer, unsigned int count, double *ptr, double normfact)
{	unsigned char	*cptr ;
	int 	value;

	cptr = ((unsigned char*) buffer) + 3 * count ;
	while (count)
	{	count -- ;
		cptr -= 3 ;	
		value = (cptr [0] << 8) + (cptr [1] << 16) + (cptr [2] << 24) ;
		ptr [count] = ((double) (value >> 8)) * normfact ;
		} ;
} /* let2d_array */

static void	
bei2d_array (int *buffer, unsigned int count, double *ptr, double normfact)
{	unsigned char	*cptr ;
	int 			value;

	cptr = ((unsigned char*) buffer) + 4 * count ;
	while (count)
	{	count -- ;
		cptr -= 4 ;
		value = (cptr [0] << 24) + (cptr [1] << 16) + (cptr [2] << 8) + cptr [3] ;
		ptr [count] = ((double) value) * normfact ;
		} ;
} /* bei2d_array */

static void	
lei2d_array (int *buffer, unsigned int count, double *ptr, double normfact)
{	unsigned char	*cptr ;
	int 			value;

	cptr = ((unsigned char*) buffer) + 4 * count ;
	while (count)
	{	count -- ;
		cptr -= 4 ;
		value = cptr [0] + (cptr [1] << 8) + (cptr [2] << 16) + (cptr [3] << 24) ;
		ptr [count] = ((double) value) * normfact ;
		} ;
} /* lei2d_array */

/*-----------------------------------------------------------------------------------------------
*/

static	void	
s2sc_array	(short *ptr, signed char *buffer, unsigned int count)
{	while (count)
	{	count -- ;
		buffer [count] = ptr [count] >> 8 ;
		} ;
} /* s2sc_array */

static	void	
s2uc_array	(short *ptr, unsigned char *buffer, unsigned int count)
{	while (count)
	{	count -- ;
		buffer [count] = (ptr [count] >> 8) + 0x80 ;
		} ;
} /* s2uc_array */

static void	
s2bet_array (short *ptr, tribyte *buffer, unsigned int count)
{	unsigned char	*cptr ;

	cptr = ((unsigned char*) buffer) + 3 * count ;
	while (count)
	{	count -- ;
		cptr -= 3 ;	
		cptr [2] = 0 ;
		cptr [1] = ptr [count] ;
		cptr [0] = ptr [count] >> 8 ;
		} ;
} /* s2bet_array */

static void	
s2let_array (short *ptr, tribyte *buffer, unsigned int count)
{	unsigned char	*cptr ;

	cptr = ((unsigned char*) buffer) + 3 * count ;
	while (count)
	{	count -- ;
		cptr -= 3 ;	
		cptr [0] = 0 ;
		cptr [1] = ptr [count] ;
		cptr [2] = ptr [count] >> 8 ;
		} ;
} /* s2let_array */

static void	
s2bei_array (short *ptr, int *buffer, unsigned int count)
{	unsigned char	*cptr ;
	
	cptr = ((unsigned char*) buffer) + 4 * count ;
	while (count)
	{	count -- ;
		cptr -= 4 ;
		cptr [0] = ptr [count] >> 8 ;
		cptr [1] = ptr [count] ;
		cptr [2] = 0 ;
		cptr [3] = 0 ;
		} ;
} /* s2bei_array */

static void	
s2lei_array (short *ptr, int *buffer, unsigned int count)
{	unsigned char	*cptr ;
	
	cptr = ((unsigned char*) buffer) + 4 * count ;
	while (count)
	{	count -- ;
		cptr -= 4 ;
		cptr [0] = 0 ; 
		cptr [1] = 0 ; 
		cptr [2] = ptr [count] ;
		cptr [3] = ptr [count] >> 8 ;
		} ;
} /* s2lei_array */


/*-----------------------------------------------------------------------------------------------
*/

static	void	
i2sc_array	(int *ptr, signed char *buffer, unsigned int count)
{	while (count)
	{	count -- ;
		buffer [count] = (ptr [count] >> 24) ;
		} ;
} /* i2sc_array */

static	void	
i2uc_array	(int *ptr, unsigned char *buffer, unsigned int count)
{	while (count)
	{	count -- ;
		buffer [count] = ((ptr [count] >> 24) + 128) ;
		} ;
} /* i2uc_array */

static void	
i2bes_array (int *ptr, short *buffer, unsigned int count)
{	unsigned char	*cptr ;
	
	cptr = ((unsigned char*) buffer) + 2 * count ;
	while (count)
	{	count -- ;
		cptr -= 2 ;
		cptr [0] = ptr [count] >> 24 ;
		cptr [1] = ptr [count] >> 16 ;
		} ;
} /* i2bes_array */

static void	
i2les_array (int *ptr, short *buffer, unsigned int count)
{	unsigned char	*cptr ;
	
	cptr = ((unsigned char*) buffer) + 2 * count ;
	while (count)
	{	count -- ;
		cptr -= 2 ;
		cptr [0] = ptr [count] >> 16 ;
		cptr [1] = ptr [count] >> 24 ;
		} ;
} /* i2les_array */

static void	
i2bet_array (int *ptr, tribyte *buffer, unsigned int count)
{	unsigned char	*cptr ;
	int				value ;
	
	cptr = ((unsigned char*) buffer) + 3 * count ;
	while (count)
	{	count -- ;
		cptr -= 3 ;	
		value = ptr [count] >> 8 ;
		cptr [2] = value ;
		cptr [1] = value >> 8 ;
		cptr [0] = value >> 16 ;
		} ;
} /* i2bet_array */

static void	
i2let_array (int *ptr, tribyte *buffer, unsigned int count)
{	unsigned char	*cptr ;
	int				value ;

	cptr = ((unsigned char*) buffer) + 3 * count ;
	while (count)
	{	count -- ;
		cptr -= 3 ;	
		value = ptr [count] >> 8 ;
		cptr [0] = value ;
		cptr [1] = value >> 8 ;
		cptr [2] = value >> 16 ;
		} ;
} /* i2let_array */

/*-----------------------------------------------------------------------------------------------
*/

static	void	
f2sc_array	(float *ptr, signed char *buffer, unsigned int count, float normfact)
{	while (count)
	{	count -- ;
		buffer [count] = lrintf (ptr [count] * normfact) ;
		} ;
} /* f2sc_array */

static	void	
f2uc_array	(float *ptr, unsigned char *buffer, unsigned int count, float normfact)
{	while (count)
	{	count -- ;
		buffer [count] = lrintf (ptr [count] * normfact) + 128 ;
		} ;
} /* f2uc_array */

static void	
f2bes_array (float *ptr, short *buffer, unsigned int count, float normfact)
{	unsigned char	*cptr ;
	short			value ;	
	cptr = ((unsigned char*) buffer) + 2 * count ;
	while (count)
	{	count -- ;
		cptr -= 2 ;
		value = lrintf (ptr [count] * normfact) ;
		cptr [0] = value >> 8 ;
		cptr [1] = value ;
		} ;
} /* f2bes_array */

static void	
f2les_array (float *ptr, short *buffer, unsigned int count, float normfact)
{	unsigned char	*cptr ;
	short			value ;	
	cptr = ((unsigned char*) buffer) + 2 * count ;
	while (count)
	{	count -- ;
		cptr -= 2 ;
		value = lrintf (ptr [count] * normfact) ;
		cptr [0] = value ;
		cptr [1] = value >> 8 ;
		} ;
} /* f2les_array */

static void	
f2bet_array (float *ptr, tribyte *buffer, unsigned int count, float normfact)
{	unsigned char	*cptr ;
	int		value ;
	
	cptr = ((unsigned char*) buffer) + 3 * count ;
	while (count)
	{	count -- ;
		cptr -= 3 ;	
		value = lrintf (ptr [count] * normfact) ;
		cptr [0] = value >> 16 ;
		cptr [1] = value >> 8 ;
		cptr [2] = value ;
		} ;
} /* f2bet_array */

static void	
f2let_array (float *ptr, tribyte *buffer, unsigned int count, float normfact)
{	unsigned char	*cptr ;
	int		value ;

	cptr = ((unsigned char*) buffer) + 3 * count ;
	while (count)
	{	count -- ;
		cptr -= 3 ;	
		value = lrintf (ptr [count] * normfact) ;
		cptr [0] = value ;
		cptr [1] = value >> 8 ;
		cptr [2] = value >> 16 ;
		} ;
} /* f2let_array */

static void	
f2bei_array (float *ptr, int *buffer, unsigned int count, float normfact)
{	unsigned char	*cptr ;
	int				value ;	
	cptr = ((unsigned char*) buffer) + 4 * count ;
	while (count)
	{	count -- ;
		cptr -= 4 ;
		value = lrintf (ptr [count] * normfact) ;
		cptr [0] = value >> 24 ;
		cptr [1] = value >> 16 ;
		cptr [2] = value >> 8 ;
		cptr [3] = value ;
		} ;
} /* f2bei_array */

static void	
f2lei_array (float *ptr, int *buffer, unsigned int count, float normfact)
{	unsigned char	*cptr ;
	int				value ;	
	cptr = ((unsigned char*) buffer) + 4 * count ;
	while (count)
	{	count -- ;
		cptr -= 4 ;
		value = lrintf (ptr [count] * normfact) ;
		cptr [0] = value ;
		cptr [1] = value >> 8 ;
		cptr [2] = value >> 16 ;
		cptr [3] = value >> 24 ;
		} ;
} /* f2lei_array */

/*-----------------------------------------------------------------------------------------------
*/

static	void	
d2sc_array	(double *ptr, signed char *buffer, unsigned int count, double normfact)
{	while (count)
	{	count -- ;
		buffer [count] = lrint (ptr [count] * normfact) ;
		} ;
} /* d2sc_array */

static	void	
d2uc_array	(double *ptr, unsigned char *buffer, unsigned int count, double normfact)
{	while (count)
	{	count -- ;
		buffer [count] = lrint (ptr [count] * normfact) + 128 ;
		} ;
} /* d2uc_array */

static void	
d2bes_array (double *ptr, short *buffer, unsigned int count, double normfact)
{	unsigned char	*cptr ;
	short			value ;	
	cptr = ((unsigned char*) buffer) + 2 * count ;
	while (count)
	{	count -- ;
		cptr -= 2 ;
		value = lrint (ptr [count] * normfact) ;
		cptr [0] = value >> 8 ;
		cptr [1] = value ;
		} ;
} /* d2bes_array */

static void	
d2les_array (double *ptr, short *buffer, unsigned int count, double normfact)
{	unsigned char	*cptr ;
	short			value ;	
	cptr = ((unsigned char*) buffer) + 2 * count ;
	while (count)
	{	count -- ;
		cptr -= 2 ;
		value = lrint (ptr [count] * normfact) ;
		cptr [0] = value ;
		cptr [1] = value >> 8 ;
		} ;
} /* d2les_array */

static void	
d2bet_array (double *ptr, tribyte *buffer, unsigned int count, double normfact)
{	unsigned char	*cptr ;
	int		value ;

	cptr = ((unsigned char*) buffer) + 3 * count ;
	while (count)
	{	count -- ;
		cptr -= 3 ;	
		value = lrint (ptr [count] * normfact) ;
		cptr [2] = value ;
		cptr [1] = value >> 8 ;
		cptr [0] = value >> 16 ;
		} ;
} /* d2bet_array */

static void	
d2let_array (double *ptr, tribyte *buffer, unsigned int count, double normfact)
{	unsigned char	*cptr ;
	int		value ;

	cptr = ((unsigned char*) buffer) + 3 * count ;
	while (count)
	{	count -- ;
		cptr -= 3 ;	
		value = lrint (ptr [count] * normfact) ;
		cptr [0] = value ;
		cptr [1] = value >> 8 ;
		cptr [2] = value >> 16 ;
		} ;
} /* d2let_array */

static void	
d2bei_array (double *ptr, int *buffer, unsigned int count, double normfact)
{	unsigned char	*cptr ;
	int				value ;	
	cptr = ((unsigned char*) buffer) + 4 * count ;
	while (count)
	{	count -- ;
		cptr -= 4 ;
		value = lrint (ptr [count] * normfact) ;
		cptr [0] = value >> 24 ;
		cptr [1] = value >> 16 ;
		cptr [2] = value >> 8 ;
		cptr [3] = value ;
		} ;
} /* d2bei_array */

static 
void	d2lei_array (double *ptr, int *buffer, unsigned int count, double normfact)
{	unsigned char	*cptr ;
	int				value ;	
	cptr = ((unsigned char*) buffer) + 4 * count ;
	while (count)
	{	count -- ;
		cptr -= 4 ;
		value = lrint (ptr [count] * normfact) ;
		cptr [0] = value ;
		cptr [1] = value >> 8 ;
		cptr [2] = value >> 16;
		cptr [3] = value >> 24 ;
		} ;
} /* d2lei_array */

/*==============================================================================
*/
