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
** $Id: mdct.h,v 1.10 2002/08/17 10:03:16 menno Exp $
**/

#ifndef __MDCT_H__
#define __MDCT_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_FFTW
#include <fftw.h>
#else
#include "cfft.h"
#endif

typedef struct {
    real_t sin;
    real_t cos;
} faad_sincos;

#ifndef USE_FFTW
typedef struct {
    real_t re;
    real_t im;
} faad_complex;
#endif

typedef struct {
    faad_sincos *sincos;
#ifdef USE_FFTW
    fftw_complex *Z1;
    fftw_complex *Z2;
    fftw_plan plan_backward;
#ifdef LTP_DEC
    fftw_plan plan_forward;
#endif
#else
    real_t *Z1;
    faad_complex *Z2;
    cfft_info *cfft;
#endif
    uint16_t N;
} mdct_info;

mdct_info *faad_mdct_init(uint16_t N);
void faad_mdct_end(mdct_info *mdct);
void faad_imdct(mdct_info *mdct, real_t *X_in, real_t *X_out);
void faad_mdct(mdct_info *mdct, real_t *X_in, real_t *X_out);


#ifdef __cplusplus
}
#endif
#endif
