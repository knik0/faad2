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
** $Id: mdct.c,v 1.19 2002/09/08 18:14:37 menno Exp $
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

#include "cfft.h"
#include "mdct.h"


mdct_info *faad_mdct_init(uint16_t N)
{
    uint16_t k;
    real_t cangle, sangle, c, s, cold;
	real_t scale = COEF_CONST(sqrt(2.0/(float32_t)N));

    mdct_info *mdct = (mdct_info*)malloc(sizeof(mdct_info));

    assert(N % 8 == 0);

    mdct->N = N;
    mdct->sincos = (faad_sincos*)malloc(N/4*sizeof(faad_sincos));
    mdct->Z1 = (real_t*)malloc(N/2*sizeof(real_t));
    mdct->Z2 = (complex_t*)malloc(N/4*sizeof(complex_t));

    cangle = COEF_CONST(cos(2.0 * M_PI / (float32_t)N));
    sangle = COEF_CONST(sin(2.0 * M_PI / (float32_t)N));
    c = COEF_CONST(cos(2.0 * M_PI * 0.125 / (float32_t)N));
    s = COEF_CONST(sin(2.0 * M_PI * 0.125 / (float32_t)N));

    for (k = 0; k < N/4; k++)
    {
        mdct->sincos[k].sin = -1*MUL_C_C(s,scale);
        mdct->sincos[k].cos = -1*MUL_C_C(c,scale);

        cold = c;
        c = MUL_C_C(c,cangle) - MUL_C_C(s,sangle);
        s = MUL_C_C(s,cangle) + MUL_C_C(cold,sangle);
    }

    /* initialise fft */
    mdct->cfft = cffti(N/4);

    return mdct;
}

void faad_mdct_end(mdct_info *mdct)
{
    cfftu(mdct->cfft);

    if (mdct->Z2) free(mdct->Z2);
    if (mdct->Z1) free(mdct->Z1);
    if (mdct->sincos) free(mdct->sincos);

    if (mdct) free(mdct);
}

void faad_imdct(mdct_info *mdct, real_t *X_in, real_t *X_out)
{
    uint16_t k;

    real_t *Z1    = mdct->Z1;
    complex_t *Z2 = mdct->Z2;
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
        Z1[n]   = MUL_R_C(x1, sincos[k].cos) - MUL_R_C(x0, sincos[k].sin);
        Z1[n+1] = MUL_R_C(x0, sincos[k].cos) + MUL_R_C(x1, sincos[k].sin);
    }

    /* complex IFFT */
    cfftb(mdct->cfft, Z1);

    /* post-IFFT complex multiplication */
    for (k = 0; k < N4; k++)
    {
        uint16_t n = k << 1;
        real_t zr = Z1[n];
        real_t zi = Z1[n+1];

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

    real_t *Z1 = mdct->Z1;
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

        Z1[n]   = -MUL_R_C(zr, sincos[k].cos) - MUL_R_C(zi, sincos[k].sin);
        Z1[n+1] = -MUL_R_C(zi, sincos[k].cos) + MUL_R_C(zr, sincos[k].sin);

        zr =  X_in[N2 - 1 - n] - X_in[        n];
        zi =  X_in[N2 +     n] + X_in[N - 1 - n];

        Z1[n   + N4] = -MUL_R_C(zr, sincos[k + N8].cos) - MUL_R_C(zi, sincos[k + N8].sin);
        Z1[n+1 + N4] = -MUL_R_C(zi, sincos[k + N8].cos) + MUL_R_C(zr, sincos[k + N8].sin);
    }

    /* complex FFT */
    cfftf(mdct->cfft, Z1);

    /* post-FFT complex multiplication */
    for (k = 0; k < N4; k++)
    {
        uint16_t n = k << 1;
        real_t zr = MUL(MUL_R_C(Z1[n], sincos[k].cos) + MUL_R_C(Z1[n+1], sincos[k].sin), scale);
        real_t zi = MUL(MUL_R_C(Z1[n+1], sincos[k].cos) - MUL_R_C(Z1[n], sincos[k].sin), scale);

        X_out[         n] =  zr;
        X_out[N2 - 1 - n] = -zi;
        X_out[N2 +     n] =  zi;
        X_out[N  - 1 - n] = -zr;
    }
}
#endif
