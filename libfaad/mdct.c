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
** $Id: mdct.c,v 1.10 2002/05/24 17:26:12 menno Exp $
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
 * Thanks to the FFTW library this (I)MDCT can now be used for any data
 * size n, where n is divisible by 8.
 *
 */


#include "common.h"

#include <stdlib.h>
#include <assert.h>

/* uses fftw (http://www.fftw.org) for very fast arbitrary-n FFT and IFFT */
#include <fftw.h>


#include "mdct.h"


void faad_mdct_init(mdct_info *mdct, uint16_t N)
{
    uint16_t k;

    assert(N % 8 == 0);

    mdct->N = N;
    mdct->sincos = (faad_sincos*)malloc(N/4*sizeof(faad_sincos));
    mdct->Z1 = (fftw_complex*)malloc(N/4*sizeof(fftw_complex));
    mdct->Z2 = (fftw_complex*)malloc(N/4*sizeof(fftw_complex));

    for (k = 0; k < N/4; k++)
    {
        real_t angle = 2.0 * M_PI * (k + 1.0/8.0)/(real_t)N;
        mdct->sincos[k].sin = -sin(angle);
        mdct->sincos[k].cos = -cos(angle);
    }

    mdct->plan_backward = fftw_create_plan(N/4, FFTW_BACKWARD, FFTW_ESTIMATE);
#ifdef LTP_DEC
    mdct->plan_forward = fftw_create_plan(N/4, FFTW_FORWARD, FFTW_ESTIMATE);
#endif
}

void faad_mdct_end(mdct_info *mdct)
{
    fftw_destroy_plan(mdct->plan_backward);
#ifdef LTP_DEC
    fftw_destroy_plan(mdct->plan_forward);
#endif

    if (mdct->Z2) free(mdct->Z2);
    if (mdct->Z1) free(mdct->Z1);
    if (mdct->sincos) free(mdct->sincos);
}

void faad_imdct(mdct_info *mdct, real_t *X_in, real_t *X_out)
{
    uint16_t k;

    fftw_complex *Z1    = mdct->Z1;
    fftw_complex *Z2    = mdct->Z2;
    faad_sincos *sincos = mdct->sincos;

    uint16_t N  = mdct->N;
    uint16_t N2 = N >> 1;
    uint16_t N4 = N >> 2;
    uint16_t N8 = N >> 3;

    real_t fac = 2.0/(real_t)N;

    /* pre-IFFT complex multiplication */
    for (k = 0; k < N4; k++)
    {
        uint16_t n = k << 1;
        real_t x0 = X_in[         n];
        real_t x1 = X_in[N2 - 1 - n];
        Z1[k].re  = MUL(fac, MUL(x1, sincos[k].cos) - MUL(x0, sincos[k].sin));
        Z1[k].im  = MUL(fac, MUL(x0, sincos[k].cos) + MUL(x1, sincos[k].sin));
    }

    /* complex IFFT */
    fftw_one(mdct->plan_backward, Z1, Z2);

    /* post-IFFT complex multiplication */
    for (k = 0; k < N4; k++)
    {
        real_t zr = Z2[k].re;
        real_t zi = Z2[k].im;
        Z2[k].re  = MUL(zr, sincos[k].cos) - MUL(zi, sincos[k].sin);
        Z2[k].im  = MUL(zi, sincos[k].cos) + MUL(zr, sincos[k].sin);
    }

    /* reordering */
    for (k = 0; k < N8; k++)
    {
        uint16_t n = k << 1;
        X_out[              n] = -Z2[N8 +     k].im;
        X_out[          1 + n] =  Z2[N8 - 1 - k].re;
        X_out[N4 +          n] = -Z2[         k].re;
        X_out[N4 +      1 + n] =  Z2[N4 - 1 - k].im;
        X_out[N2 +          n] = -Z2[N8 +     k].re;
        X_out[N2 +      1 + n] =  Z2[N8 - 1 - k].im;
        X_out[N2 + N4 +     n] =  Z2[         k].im;
        X_out[N2 + N4 + 1 + n] = -Z2[N4 - 1 - k].re;
    }
}

#ifdef LTP_DEC
void faad_mdct(mdct_info *mdct, real_t *X_in, real_t *X_out)
{
    uint16_t k;

    fftw_complex *Z1    = mdct->Z1;
    fftw_complex *Z2    = mdct->Z2;
    faad_sincos *sincos = mdct->sincos;

    uint16_t N  = mdct->N;
    uint16_t N2 = N >> 1;
    uint16_t N4 = N >> 2;
    uint16_t N8 = N >> 3;


    /* pre-FFT complex multiplication */
    for (k = 0; k < N8; k++)
    {
        uint16_t n = k << 1;
        real_t zr     =  X_in[N - N4 - 1 - n] + X_in[N - N4 +     n];
        real_t zi     =  X_in[    N4 +     n] - X_in[    N4 - 1 - n];

        Z1[k     ].re = -MUL(zr, sincos[k     ].cos) - MUL(zi, sincos[k     ].sin);
        Z1[k     ].im = -MUL(zi, sincos[k     ].cos) + MUL(zr, sincos[k     ].sin);

        zr            =  X_in[    N2 - 1 - n] - X_in[             n];
        zi            =  X_in[    N2 +     n] + X_in[N -      1 - n];

        Z1[k + N8].re = -MUL(zr, sincos[k + N8].cos) - MUL(zi, sincos[k + N8].sin);
        Z1[k + N8].im = -MUL(zi, sincos[k + N8].cos) + MUL(zr, sincos[k + N8].sin);
    }

    /* complex FFT */
    fftw_one(mdct->plan_forward, Z1, Z2);

    /* post-FFT complex multiplication */
    for (k = 0; k < N4; k++)
    {
        uint16_t n = k << 1;
        real_t zr = MUL(2.0, MUL(Z2[k].re, sincos[k].cos) + MUL(Z2[k].im, sincos[k].sin));
        real_t zi = MUL(2.0, MUL(Z2[k].im, sincos[k].cos) - MUL(Z2[k].re, sincos[k].sin));

        X_out[         n] = -zr;
        X_out[N2 - 1 - n] =  zi;
        X_out[N2 +     n] = -zi;
        X_out[N  - 1 - n] =  zr;
    }
}
#endif
