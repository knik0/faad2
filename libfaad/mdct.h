/*
** FAAD - Freeware Advanced Audio Decoder
** Copyright (C) 2002 M. Bakker
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** $Id: mdct.h,v 1.2 2002/02/18 10:01:05 menno Exp $
**/

#ifndef __MDCT_H__
#define __MDCT_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef real_t fftw_real;

typedef struct {
     fftw_real re, im;
} fftw_complex;

#define c_re(c)  ((c).re)
#define c_im(c)  ((c).im)

#define DEFINE_PFFTW(size)          \
 void pfftwi_##size(fftw_complex *input);   \
 void pfftw_##size(fftw_complex *input);    \
 uint16_t pfftw_permutation_##size(uint16_t i);

DEFINE_PFFTW(16)
DEFINE_PFFTW(32)
DEFINE_PFFTW(64)
DEFINE_PFFTW(128)
DEFINE_PFFTW(512)

void make_fft_order(uint16_t *unscrambled64, uint16_t *unscrambled512);
void IMDCT_long(fftw_real *in_data, fftw_real *out_data, uint16_t *unscrambled);
void IMDCT_short(fftw_real *in_data, fftw_real *out_data, uint16_t *unscrambled);

void MDCT_long(fftw_real *in_data, fftw_real *out_data, uint16_t *unscrambled);
void MDCT_short(fftw_real *in_data, fftw_real *out_data, uint16_t *unscrambled);

#define PFFTW(name)  CONCAT(pfftw_, name)
#define PFFTWI(name)  CONCAT(pfftwi_, name)
#define CONCAT_AUX(a, b) a ## b
#define CONCAT(a, b) CONCAT_AUX(a,b)
#define FFTW_KONST(x) ((fftw_real) x)

void PFFTW(twiddle_4)(fftw_complex *A, const fftw_complex *W, uint16_t iostride);
void PFFTWI(twiddle_4)(fftw_complex *A, const fftw_complex *W, uint16_t iostride);

#ifdef __cplusplus
}
#endif
#endif
