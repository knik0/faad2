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
** $Id: mdct.c,v 1.23 2002/11/28 18:48:30 menno Exp $
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
#include "structs.h"

#include <stdlib.h>
#ifdef _WIN32_WCE
#define assert(x)
#else
#include <assert.h>
#endif

#include "cfft.h"
#include "mdct.h"

/* const_tab[]:
    0: sqrt(2 / N)
    1: cos(2 * PI / N)
    2: sin(2 * PI / N)
    3: cos(2 * PI * (1/8) / N)
    4: sin(2 * PI * (1/8) / N)
 */
#ifdef FIXED_POINT
real_t const_tab[][5] =
{
    { 0x800000, 0xFFFFB10, 0xC90FC, 0xFFFFFF0, 0x1921F }, /* 2048 */
    { 0x8432A5, 0xFFFFA60, 0xD6773, 0xFFFFFF0, 0x1ACEE }, /* 1920 */
    { 0xB504F3, 0xFFFEC40, 0x1921F1, 0xFFFFFB0, 0x3243F }, /* 1024 */
    { 0xBAF4BA, 0xFFFE990, 0x1ACEDD, 0xFFFFFA0, 0x359DD }, /* 960 */
    { 0x16A09E6, 0xFFEC430, 0x648558, 0xFFFFB10, 0xC90FC }, /* 256 */
    { 0x175E974, 0xFFE98B0, 0x6B3885, 0xFFFFA60, 0xD6773 }  /* 240 */
#ifdef SSR_DEC
   ,{ 0, 0, 0, 0, 0 }, /* 512 */
    { 0, 0, 0, 0, 0 }  /* 64 */
#endif
};
#else
#ifdef _MSC_VER
#pragma warning(disable:4305)
#pragma warning(disable:4244)
#endif
real_t const_tab[][5] =
{
    { 0.0312500000, 0.9999952938, 0.0030679568, 0.9999999265, 0.0003834952 }, /* 2048 */
    { 0.0322748612, 0.9999946356, 0.0032724866, 0.9999999404, 0.0004090615 }, /* 1920 */
    { 0.0441941738, 0.9999811649, 0.0061358847, 0.9999997020, 0.0007669903 }, /* 1024 */
    { 0.0456435465, 0.9999786019, 0.0065449383, 0.9999996424, 0.0008181230 }, /* 960 */
    { 0.0883883476, 0.9996988177, 0.0245412290, 0.9999952912, 0.0030679568 }, /* 256 */
    { 0.0912870929, 0.9996573329, 0.0261769500, 0.9999946356, 0.0032724866 }  /* 240 */
#ifdef SSR_DEC
   ,{ 0.062500000, 0.999924702, 0.012271538, 0.999998823, 0.00153398 }, /* 512 */
    { 0.176776695, 0.995184727, 0.09801714, 0.999924702, 0.012271538 }  /* 64 */
#endif
};
#endif

uint8_t map_N_to_idx(uint16_t N)
{
    switch(N)
    {
    case 2048: return 0;
    case 1920: return 1;
    case 1024: return 2;
    case 960:  return 3;
    case 256:  return 4;
    case 240:  return 5;
#ifdef SSR_DEC
    case 512:  return 6;
    case 64:   return 7;
#endif
    }
    return 0;
}

mdct_info *faad_mdct_init(uint16_t N)
{
    uint16_t k, N_idx;
    real_t cangle, sangle, c, s, cold;
	real_t scale;

    mdct_info *mdct = (mdct_info*)malloc(sizeof(mdct_info));

    assert(N % 8 == 0);

    mdct->N = N;
    mdct->sincos = (complex_t*)malloc(N/4*sizeof(complex_t));
    mdct->Z1 = (complex_t*)malloc(N/4*sizeof(complex_t));

    N_idx = map_N_to_idx(N);

    scale = const_tab[N_idx][0];
    cangle = const_tab[N_idx][1];
    sangle = const_tab[N_idx][2];
    c = const_tab[N_idx][3];
    s = const_tab[N_idx][4];

    for (k = 0; k < N/4; k++)
    {
        RE(mdct->sincos[k]) = -1*MUL_C_C(c,scale);
        IM(mdct->sincos[k]) = -1*MUL_C_C(s,scale);

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

    if (mdct->Z1) free(mdct->Z1);
    if (mdct->sincos) free(mdct->sincos);

    if (mdct) free(mdct);
}

void faad_imdct(mdct_info *mdct, real_t *X_in, real_t *X_out)
{
    uint16_t k;

    complex_t x;
    complex_t *Z1 = mdct->Z1;
    complex_t *sincos = mdct->sincos;

    uint16_t N  = mdct->N;
    uint16_t N2 = N >> 1;
    uint16_t N4 = N >> 2;
    uint16_t N8 = N >> 3;

    /* pre-IFFT complex multiplication */
    for (k = 0; k < N4; k++)
    {
        uint16_t n = k << 1;
        RE(x) = X_in[         n];
        IM(x) = X_in[N2 - 1 - n];
        RE(Z1[k]) = MUL_R_C(IM(x), RE(sincos[k])) - MUL_R_C(RE(x), IM(sincos[k]));
        IM(Z1[k]) = MUL_R_C(RE(x), RE(sincos[k])) + MUL_R_C(IM(x), IM(sincos[k]));
    }

    /* complex IFFT */
    cfftb(mdct->cfft, Z1);

    /* post-IFFT complex multiplication */
    for (k = 0; k < N4; k++)
    {
        uint16_t n = k << 1;
        RE(x) = RE(Z1[k]);
        IM(x) = IM(Z1[k]);

        RE(Z1[k]) = MUL_R_C(RE(x), RE(sincos[k])) - MUL_R_C(IM(x), IM(sincos[k]));
        IM(Z1[k]) = MUL_R_C(IM(x), RE(sincos[k])) + MUL_R_C(RE(x), IM(sincos[k]));
    }

    /* reordering */
    for (k = 0; k < N8; k++)
    {
        uint16_t n = k << 1;
        X_out[              n] =  IM(Z1[N8 +     k]);
        X_out[          1 + n] = -RE(Z1[N8 - 1 - k]);
        X_out[N4 +          n] =  RE(Z1[         k]);
        X_out[N4 +      1 + n] = -IM(Z1[N4 - 1 - k]);
        X_out[N2 +          n] =  RE(Z1[N8 +     k]);
        X_out[N2 +      1 + n] = -IM(Z1[N8 - 1 - k]);
        X_out[N2 + N4 +     n] = -IM(Z1[         k]);
        X_out[N2 + N4 + 1 + n] =  RE(Z1[N4 - 1 - k]);
    }
}

#ifdef LTP_DEC
void faad_mdct(mdct_info *mdct, real_t *X_in, real_t *X_out)
{
    uint16_t k;

    complex_t x;
    complex_t *Z1 = mdct->Z1;
    complex_t *sincos = mdct->sincos;

    uint16_t N  = mdct->N;
    uint16_t N2 = N >> 1;
    uint16_t N4 = N >> 2;
    uint16_t N8 = N >> 3;

	real_t scale = REAL_CONST(N);

    /* pre-FFT complex multiplication */
    for (k = 0; k < N8; k++)
    {
        uint16_t n = k << 1;
        RE(x) = X_in[N - N4 - 1 - n] + X_in[N - N4 +     n];
        IM(x) = X_in[    N4 +     n] - X_in[    N4 - 1 - n];

        RE(Z1[k]) = -MUL_R_C(RE(x), RE(sincos[k])) - MUL_R_C(IM(x), IM(sincos[k]));
        IM(Z1[k]) = -MUL_R_C(IM(x), RE(sincos[k])) + MUL_R_C(RE(x), IM(sincos[k]));

        RE(x) =  X_in[N2 - 1 - n] - X_in[        n];
        IM(x) =  X_in[N2 +     n] + X_in[N - 1 - n];

        RE(Z1[k + N8]) = -MUL_R_C(RE(x), RE(sincos[k + N8])) - MUL_R_C(IM(x), IM(sincos[k + N8]));
        IM(Z1[k + N8]) = -MUL_R_C(IM(x), RE(sincos[k + N8])) + MUL_R_C(RE(x), IM(sincos[k + N8]));
    }

    /* complex FFT */
    cfftf(mdct->cfft, Z1);

    /* post-FFT complex multiplication */
    for (k = 0; k < N4; k++)
    {
        uint16_t n = k << 1;
        RE(x) = MUL(MUL_R_C(RE(Z1[k]), RE(sincos[k])) + MUL_R_C(IM(Z1[k]), IM(sincos[k])), scale);
        IM(x) = MUL(MUL_R_C(IM(Z1[k]), RE(sincos[k])) - MUL_R_C(RE(Z1[k]), IM(sincos[k])), scale);

        X_out[         n] =  RE(x);
        X_out[N2 - 1 - n] = -IM(x);
        X_out[N2 +     n] =  IM(x);
        X_out[N  - 1 - n] = -RE(x);
    }
}
#endif
