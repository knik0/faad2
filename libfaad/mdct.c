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
** $Id: mdct.c,v 1.18 2002/08/27 18:16:12 menno Exp $
**/

/*
 * Fast (I)MDCT Implementation using (I)FFT ((Inverse) Fast Fourier Transform)
 * and consists of three steps: pre-(I)FFT complex multiplication, complex
 * (I)FFT, post-(I)FFT complex multiplication,
 * 
 * As described in:
 *  P. Duhamel, Y. Mahieux, and J.P. Petit, "A Fast Algorithm for the
 *  Implementation of Filter Banks Based on 'Time Domain Aliasing
 *  Cancellation’," IEEE Proc. on ICASSP‘91, 1991, pp. 2209-2212.
 *
 *
 * As of April 6th 2002 completely rewritten.
 * This (I)MDCT can now be used for any data size n, where n is divisible by 8.
 *
 */


#include "common.h"

#include <stdlib.h>
#include <assert.h>

#ifdef USE_FFTW
/* uses fftw (http://www.fftw.org) for very fast arbitrary-n FFT and IFFT */
#include <fftw.h>
#else
#include "cfft.h"
#endif


#include "mdct.h"

mdct_info *faad_mdct_init(uint16_t N)
{
    uint16_t k;
	float32_t scale = sqrt(2.0/(float32_t)N);

    mdct_info *mdct = (mdct_info*)malloc(sizeof(mdct_info));

    assert(N % 8 == 0);

    mdct->N = N;
    mdct->sincos = (faad_sincos*)malloc(N/4*sizeof(faad_sincos));
#ifdef USE_FFTW
    mdct->Z1 = (fftw_complex*)malloc(N/4*sizeof(fftw_complex));
    mdct->Z2 = (fftw_complex*)malloc(N/4*sizeof(fftw_complex));
#else
    mdct->Z1 = (real_t*)malloc(N/2*sizeof(real_t));
    mdct->Z2 = (faad_complex*)malloc(N/4*sizeof(faad_complex));
#endif

    for (k = 0; k < N/4; k++)
    {
        float32_t angle = 2.0 * M_PI * ((float32_t)k + 0.125)/(float32_t)N;
        mdct->sincos[k].sin = COEF_CONST(-sin(angle)*scale);
        mdct->sincos[k].cos = COEF_CONST(-cos(angle)*scale);
    }

#ifdef USE_FFTW
    mdct->plan_backward = fftw_create_plan(N/4, FFTW_BACKWARD, FFTW_ESTIMATE);
#ifdef LTP_DEC
    mdct->plan_forward = fftw_create_plan(N/4, FFTW_FORWARD, FFTW_ESTIMATE);
#endif
#else
    /* own implementation */
    mdct->cfft = cffti(N/4);
#endif

    return mdct;
}

void faad_mdct_end(mdct_info *mdct)
{
#ifdef USE_FFTW
    fftw_destroy_plan(mdct->plan_backward);
#ifdef LTP_DEC
    fftw_destroy_plan(mdct->plan_forward);
#endif
#else
    cfftu(mdct->cfft);
#endif

    if (mdct->Z2) free(mdct->Z2);
    if (mdct->Z1) free(mdct->Z1);
    if (mdct->sincos) free(mdct->sincos);

    if (mdct) free(mdct);
}

void faad_imdct(mdct_info *mdct, real_t *X_in, real_t *X_out)
{
    uint16_t k;

#ifdef USE_FFTW
    fftw_complex *Z1    = mdct->Z1;
    fftw_complex *Z2    = mdct->Z2;
#else
    real_t *Z1          = mdct->Z1;
    faad_complex *Z2    = mdct->Z2;
#endif
    faad_sincos *sincos = mdct->sincos;

    uint16_t N  = mdct->N;
    uint16_t N2 = N >> 1;
    uint16_t N4 = N >> 2;
    uint16_t N8 = N >> 3;

    /* pre-IFFT complex multiplication */
    for (k = 0; k < N4; k++)
    {
        uint16_t n = k << 1;
        real_t x0 = X_in[         n];
        real_t x1 = X_in[N2 - 1 - n];
#ifdef USE_FFTW
        Z1[k].re  = MUL_R_C(x1, sincos[k].cos) - MUL_R_C(x0, sincos[k].sin);
        Z1[k].im  = MUL_R_C(x0, sincos[k].cos) + MUL_R_C(x1, sincos[k].sin);
#else
        Z1[n]   = MUL_R_C(x1, sincos[k].cos) - MUL_R_C(x0, sincos[k].sin);
        Z1[n+1] = MUL_R_C(x0, sincos[k].cos) + MUL_R_C(x1, sincos[k].sin);
#endif
    }

    /* complex IFFT */
#ifdef USE_FFTW
    fftw_one(mdct->plan_backward, Z1, Z2);
#else
    cfftb(mdct->cfft, Z1);
#endif

    /* post-IFFT complex multiplication */
    for (k = 0; k < N4; k++)
    {
#ifdef USE_FFTW
        real_t zr = Z2[k].re;
        real_t zi = Z2[k].im;
#else
        uint16_t n = k << 1;
        real_t zr = Z1[n];
        real_t zi = Z1[n+1];
#endif
        Z2[k].re  = MUL_R_C(zr, sincos[k].cos) - MUL_R_C(zi, sincos[k].sin);
        Z2[k].im  = MUL_R_C(zi, sincos[k].cos) + MUL_R_C(zr, sincos[k].sin);
    }

    /* reordering */
    for (k = 0; k < N8; k++)
    {
        uint16_t n = k << 1;
        X_out[              n] =  Z2[N8 +     k].im;
        X_out[          1 + n] = -Z2[N8 - 1 - k].re;
        X_out[N4 +          n] =  Z2[         k].re;
        X_out[N4 +      1 + n] = -Z2[N4 - 1 - k].im;
        X_out[N2 +          n] =  Z2[N8 +     k].re;
        X_out[N2 +      1 + n] = -Z2[N8 - 1 - k].im;
        X_out[N2 + N4 +     n] = -Z2[         k].im;
        X_out[N2 + N4 + 1 + n] =  Z2[N4 - 1 - k].re;
    }
}

#ifdef LTP_DEC
void faad_mdct(mdct_info *mdct, real_t *X_in, real_t *X_out)
{
    uint16_t k;

#ifdef USE_FFTW
    fftw_complex *Z1 = mdct->Z1;
    fftw_complex *Z2 = mdct->Z2;
#else
    real_t *Z1 = mdct->Z1;
#endif
    faad_sincos *sincos = mdct->sincos;

    uint16_t N  = mdct->N;
    uint16_t N2 = N >> 1;
    uint16_t N4 = N >> 2;
    uint16_t N8 = N >> 3;

	real_t scale = REAL_CONST(N);

    /* pre-FFT complex multiplication */
    for (k = 0; k < N8; k++)
    {
        uint16_t n = k << 1;
        real_t zr =  X_in[N - N4 - 1 - n] + X_in[N - N4 +     n];
        real_t zi =  X_in[    N4 +     n] - X_in[    N4 - 1 - n];

#ifdef USE_FFTW
        Z1[k].re = -MUL_R_C(zr, sincos[k].cos) - MUL_R_C(zi, sincos[k].sin);
        Z1[k].im = -MUL_R_C(zi, sincos[k].cos) + MUL_R_C(zr, sincos[k].sin);
#else
        Z1[n]   = -MUL_R_C(zr, sincos[k].cos) - MUL_R_C(zi, sincos[k].sin);
        Z1[n+1] = -MUL_R_C(zi, sincos[k].cos) + MUL_R_C(zr, sincos[k].sin);
#endif

        zr =  X_in[N2 - 1 - n] - X_in[        n];
        zi =  X_in[N2 +     n] + X_in[N - 1 - n];

#ifdef USE_FFTW
        Z1[k + N8].re = -MUL_R_C(zr, sincos[k + N8].cos) - MUL_R_C(zi, sincos[k + N8].sin);
        Z1[k + N8].im = -MUL_R_C(zi, sincos[k + N8].cos) + MUL_R_C(zr, sincos[k + N8].sin);
#else
        Z1[n   + N4] = -MUL_R_C(zr, sincos[k + N8].cos) - MUL_R_C(zi, sincos[k + N8].sin);
        Z1[n+1 + N4] = -MUL_R_C(zi, sincos[k + N8].cos) + MUL_R_C(zr, sincos[k + N8].sin);
#endif
    }

    /* complex FFT */
#ifdef USE_FFTW
    fftw_one(mdct->plan_forward, Z1, Z2);
#else
    cfftf(mdct->cfft, Z1);
#endif

    /* post-FFT complex multiplication */
    for (k = 0; k < N4; k++)
    {
        uint16_t n = k << 1;
#ifdef USE_FFTW
        real_t zr = MUL(MUL_R_C(Z2[k].re, sincos[k].cos) + MUL_R_C(Z2[k].im, sincos[k].sin), scale);
        real_t zi = MUL(MUL_R_C(Z2[k].im, sincos[k].cos) - MUL_R_C(Z2[k].re, sincos[k].sin), scale);
#else
        real_t zr = MUL(MUL_R_C(Z1[n], sincos[k].cos) + MUL_R_C(Z1[n+1], sincos[k].sin), scale);
        real_t zi = MUL(MUL_R_C(Z1[n+1], sincos[k].cos) - MUL_R_C(Z1[n], sincos[k].sin), scale);
#endif

        X_out[         n] =  zr;
        X_out[N2 - 1 - n] = -zi;
        X_out[N2 +     n] =  zi;
        X_out[N  - 1 - n] = -zr;
    }
}
#endif
