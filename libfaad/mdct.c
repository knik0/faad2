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
** $Id: mdct.c,v 1.5 2002/03/16 13:38:36 menno Exp $
**/

/*
 * Fast MDCT Implementation using pre-twiddling and FFT using fftw
 *
 *
 * Optionally uses recurrence relation to find sine and cosine values without
 * lookup table or calling the actual sine or cosine function.
 * Works like this:
 *  cos(2*Pi*(i+1)/N) = cos(2*Pi*i/N)*cos(2*Pi/N) - sin(2*Pi*i/N)*sin(2*Pi/N)
 *  sin(2*Pi*(i+1)/N) = sin(2*Pi*i/N)*cos(2*Pi/N) + cos(2*Pi*i/N)*sin(2*Pi/N)
 *
 * In which cos(2*Pi/N) and sin(2*Pi/N) are constants and cos(2*Pi*i/N) and
 * sin(2*Pi*i/N) are the previous values.
 * (in the method used in this MDCT there is an extra factor 8, but I left that
 * out here to show more clearly the relation)
 *
 * Nice method for low memory usage, but lookup table is faster on most
 * machines.
 *
 */


#include "common.h"

#include <stdlib.h>
#include "mdct.h"

void mdct_init(mdct_info *mdct, uint16_t len)
{
#ifdef USE_TWIDDLE_TABLE
    uint16_t i;
#endif

    mdct->len = len;

    mdct->unscrambled = malloc(len/4*sizeof(uint16_t));
    make_fft_order(mdct->unscrambled, len/4);

#ifdef USE_TWIDDLE_TABLE
    mdct->twiddlers = malloc(len/2*sizeof(real_t));
    for (i = 0; i < len/4; i++)
    {
        real_t angle = 2.0f * M_PI * (i + 1.0f/8.0f) / (real_t)len;
        mdct->twiddlers[i*2]     = (real_t)cos(angle);
        mdct->twiddlers[i*2 + 1] = (real_t)sin(angle);
    }
#endif
}

void mdct_end(mdct_info *mdct)
{
    if (mdct->unscrambled) free(mdct->unscrambled);
#ifdef USE_TWIDDLE_TABLE
    if (mdct->twiddlers) free(mdct->twiddlers);
#endif
}

void MDCT_2048(mdct_info *mdct, fftw_real *in_data, fftw_real *out_data)
{
    fftw_complex FFTarray[512];
    real_t tempr, tempi, fac;

#ifdef USE_TWIDDLE_TABLE
    /* use twiddle factor tables */
    real_t *twiddlers = mdct->twiddlers;
#else
    /* temps for pre and post twiddle */
    real_t cosfreq8, sinfreq8, c, s, cold, cfreq, sfreq;
#endif
    uint16_t i;


    fac = 2.; /* 2 from MDCT inverse to forward */

#ifndef USE_TWIDDLE_TABLE
    /* prepare for recurrence relation in pre-twiddle */
    cfreq = 0.99999529123306274f;
    sfreq = 0.0030679567717015743f;
    cosfreq8 = 0.99999994039535522f;
    sinfreq8 = 0.00038349519824312089f;

    c = cosfreq8;
    s = sinfreq8;
#endif

    for (i = 0; i < 512; i++)
    {
        uint16_t n = 1023 - (i << 1);
        if (i < 256)
            tempr = in_data[512 + n] + in_data[2559 - n];
        else
            tempr = in_data[512 + n] - in_data[511 - n];

        n = (i << 1);
        if (i < 256)
            tempi = in_data[512 + n] - in_data[511 - n];
        else
            tempi = in_data[512 + n] + in_data[2559 - n];

        /* calculate pre-twiddled FFT input */
#ifdef USE_TWIDDLE_TABLE
        FFTarray[i].re = tempr * twiddlers[n] + tempi * twiddlers[n + 1];
        FFTarray[i].im = tempi * twiddlers[n] - tempr * twiddlers[n + 1];
#else
        FFTarray[i].re = tempr * c + tempi * s;
        FFTarray[i].im = tempi * c - tempr * s;

        /* use recurrence to prepare cosine and sine for next value of i */
        cold = c;
        c = c * cfreq - s * sfreq;
        s = s * cfreq + cold * sfreq;
#endif
    }

    /* Perform in-place complex FFT of length N/4 */
    pfftw_512(FFTarray);


#ifndef USE_TWIDDLE_TABLE
    /* prepare for recurrence relations in post-twiddle */
    c = cosfreq8;
    s = sinfreq8;
#endif

    /* post-twiddle FFT output and then get output data */
    for (i = 0; i < 512; i++)
    {
        uint16_t n = i << 1;
        uint16_t unscr = mdct->unscrambled[i];

        /* get post-twiddled FFT output  */
#ifdef USE_TWIDDLE_TABLE
        tempr = fac * (FFTarray[unscr].re * twiddlers[n] + FFTarray[unscr].im * twiddlers[n + 1]);
        tempi = fac * (FFTarray[unscr].im * twiddlers[n] - FFTarray[unscr].re * twiddlers[n + 1]);
#else
        tempr = fac * (FFTarray[unscr].re * c + FFTarray[unscr].im * s);
        tempi = fac * (FFTarray[unscr].im * c - FFTarray[unscr].re * s);
#endif

        /* fill in output values */
        out_data[n]        = -tempr;  /* first half even */
        out_data[1023 - n] =  tempi;  /* first half odd */
        out_data[1024 + n] = -tempi;  /* second half even */
        out_data[2047 - n] =  tempr;  /* second half odd */

#ifndef USE_TWIDDLE_TABLE
        /* use recurrence to prepare cosine and sine for next value of i */
        cold = c;
        c = c * cfreq - s * sfreq;
        s = s * cfreq + cold * sfreq;
#endif
    }
}

#ifdef LD_DEC
void MDCT_1024(mdct_info *mdct, fftw_real *in_data, fftw_real *out_data)
{
    fftw_complex FFTarray[256];
    real_t tempr, tempi, fac;

#ifdef USE_TWIDDLE_TABLE
    /* use twiddle factor tables */
    real_t *twiddlers = mdct->twiddlers;
#else
    /* temps for pre and post twiddle */
    real_t cosfreq8, sinfreq8, c, s, cold, cfreq, sfreq;
#endif
    uint16_t i;


    fac = 2.; /* 2 from MDCT inverse to forward */

#ifndef USE_TWIDDLE_TABLE
    /* prepare for recurrence relation in pre-twiddle */
    cfreq = 0.9999811752826011426f;
    sfreq = 0.0061358846491544753f;
    cosfreq8 = 0.99999970586288221916f;
    sinfreq8 = 0.00076699031874270453f;

    c = cosfreq8;
    s = sinfreq8;
#endif

    for (i = 0; i < 256; i++)
    {
        uint16_t n = 511 - (i << 1);
        if (i < 128)
            tempr = in_data[256 + n] + in_data[1279 - n];
        else
            tempr = in_data[256 + n] - in_data[255 - n];

        n = (i << 1);
        if (i < 128)
            tempi = in_data[256 + n] - in_data[255 - n];
        else
            tempi = in_data[256 + n] + in_data[1279 - n];

        /* calculate pre-twiddled FFT input */
#ifdef USE_TWIDDLE_TABLE
        FFTarray[i].re = tempr * twiddlers[n] + tempi * twiddlers[n + 1];
        FFTarray[i].im = tempi * twiddlers[n] - tempr * twiddlers[n + 1];
#else
        FFTarray[i].re = tempr * c + tempi * s;
        FFTarray[i].im = tempi * c - tempr * s;

        /* use recurrence to prepare cosine and sine for next value of i */
        cold = c;
        c = c * cfreq - s * sfreq;
        s = s * cfreq + cold * sfreq;
#endif
    }

    /* Perform in-place complex FFT of length N/4 */
    pfftw_256(FFTarray);


#ifndef USE_TWIDDLE_TABLE
    /* prepare for recurrence relations in post-twiddle */
    c = cosfreq8;
    s = sinfreq8;
#endif

    /* post-twiddle FFT output and then get output data */
    for (i = 0; i < 256; i++)
    {
        uint16_t n = i << 1;
        uint16_t unscr = mdct->unscrambled[i];

        /* get post-twiddled FFT output  */
#ifdef USE_TWIDDLE_TABLE
        tempr = fac * (FFTarray[unscr].re * twiddlers[n] + FFTarray[unscr].im * twiddlers[n + 1]);
        tempi = fac * (FFTarray[unscr].im * twiddlers[n] - FFTarray[unscr].re * twiddlers[n + 1]);
#else
        tempr = fac * (FFTarray[unscr].re * c + FFTarray[unscr].im * s);
        tempi = fac * (FFTarray[unscr].im * c - FFTarray[unscr].re * s);
#endif

        /* fill in output values */
        out_data[n]        = -tempr;  /* first half even */
        out_data[511 - n]  =  tempi;  /* first half odd */
        out_data[512 + n]  = -tempi;  /* second half even */
        out_data[1023 - n] =  tempr;  /* second half odd */

#ifndef USE_TWIDDLE_TABLE
        /* use recurrence to prepare cosine and sine for next value of i */
        cold = c;
        c = c * cfreq - s * sfreq;
        s = s * cfreq + cold * sfreq;
#endif
    }
}
#endif

void MDCT_256(mdct_info *mdct, fftw_real *in_data, fftw_real *out_data)
{
    fftw_complex FFTarray[64];    /* the array for in-place FFT */
    real_t tempr, tempi, fac;

#ifdef USE_TWIDDLE_TABLE
    /* use twiddle factor tables */
    real_t *twiddlers = mdct->twiddlers;
#else
    /* temps for pre and post twiddle */
    real_t cosfreq8, sinfreq8, c, s, cold, cfreq, sfreq;
#endif
    uint16_t i;


    /* Choosing to allocate 2/N factor to Inverse Xform! */
    fac = 2.; /* 2 from MDCT inverse  to forward */

#ifndef USE_TWIDDLE_TABLE
    /* prepare for recurrence relation in pre-twiddle */
    cfreq = 0.99969881772994995f;
    sfreq = 0.024541229009628296f;
    cosfreq8 = 0.99999529123306274f;
    sinfreq8 = 0.0030679568483393833f;

    c = cosfreq8;
    s = sinfreq8;
#endif

    for (i = 0; i < 64; i++)
    {
        uint16_t n = 127 - (i << 1);
        if (i < 32)
            tempr = in_data[64 + n] + in_data[319 - n];
        else
            tempr = in_data[64 + n] - in_data[63 - n];

        n = i << 1;
        if (i < 32)
            tempi = in_data[64 + n] - in_data[63 - n];
        else
            tempi = in_data[64 + n] + in_data[319 - n];

        /* calculate pre-twiddled FFT input */
#ifdef USE_TWIDDLE_TABLE
        FFTarray[i].re = tempr * twiddlers[n] + tempi * twiddlers[n + 1];
        FFTarray[i].im = tempi * twiddlers[n] - tempr * twiddlers[n + 1];
#else
        FFTarray[i].re = tempr * c + tempi * s;
        FFTarray[i].im = tempi * c - tempr * s;

        /* use recurrence to prepare cosine and sine for next value of i */
        cold = c;
        c = c * cfreq - s * sfreq;
        s = s * cfreq + cold * sfreq;
#endif
    }

    /* Perform in-place complex FFT of length N/4 */
    pfftw_64(FFTarray);

#ifndef USE_TWIDDLE_TABLE
    /* prepare for recurrence relations in post-twiddle */
    c = cosfreq8;
    s = sinfreq8;
#endif

    /* post-twiddle FFT output and then get output data */
    for (i = 0; i < 64; i++)
    {
        uint16_t n = i << 1;
        uint16_t unscr = mdct->unscrambled[i];

#ifdef USE_TWIDDLE_TABLE
        tempr = fac * (FFTarray[unscr].re * twiddlers[n] + FFTarray[unscr].im * twiddlers[n + 1]);
        tempi = fac * (FFTarray[unscr].im * twiddlers[n] - FFTarray[unscr].re * twiddlers[n + 1]);
#else
        tempr = fac * (FFTarray[unscr].re * c + FFTarray[unscr].im * s);
        tempi = fac * (FFTarray[unscr].im * c - FFTarray[unscr].re * s);
#endif

        /* fill in output values */
        out_data[n]           = -tempr;  /* first half even */
        out_data[127 - n] =  tempi;  /* first half odd */
        out_data[128 + n]     = -tempi;  /* second half even */
        out_data[255 - n] =  tempr;  /* second half odd */

#ifndef USE_TWIDDLE_TABLE
        /* use recurrence to prepare cosine and sine for next value of i */
        cold = c;
        c = c * cfreq - s * sfreq;
        s = s * cfreq + cold * sfreq;
#endif
    }
}

void IMDCT_2048(mdct_info *mdct, fftw_real *in_data, fftw_real *out_data)
{
    fftw_complex FFTarray[512];    /* the array for in-place FFT */
    real_t tempr, tempi, fac;

#ifdef USE_TWIDDLE_TABLE
    /* use twiddle factor tables */
    real_t *twiddlers = mdct->twiddlers;
#else
    /* temps for pre and post twiddle */
    real_t cosfreq8, sinfreq8, c, s, cold, cfreq, sfreq;
#endif
    uint16_t i;

    /* Choosing to allocate 2/N factor to Inverse Xform! */
    fac = 0.0009765625f;

#ifndef USE_TWIDDLE_TABLE
    /* prepare for recurrence relation in pre-twiddle */
    cfreq = 0.99999529123306274f;
    sfreq = 0.0030679567717015743f;
    cosfreq8 = 0.99999994039535522f;
    sinfreq8 = 0.00038349519824312089f;

    c = cosfreq8;
    s = sinfreq8;
#endif

    for (i = 0; i < 512; i++)
    {
        uint16_t n = i << 1;
        uint16_t unscr = mdct->unscrambled[i];

        tempr = -in_data[n];
        tempi =  in_data[1023 - n];

        /* calculate pre-twiddled FFT input */
#ifdef USE_TWIDDLE_TABLE
        FFTarray[unscr].re = tempr * twiddlers[n] - tempi * twiddlers[n + 1];
        FFTarray[unscr].im = tempi * twiddlers[n] + tempr * twiddlers[n + 1];
#else
        FFTarray[unscr].re = tempr * c - tempi * s;
        FFTarray[unscr].im = tempi * c + tempr * s;

        /* use recurrence to prepare cosine and sine for next value of i */
        cold = c;
        c = c * cfreq - s * sfreq;
        s = s * cfreq + cold * sfreq;
#endif
    }

    /* Perform in-place complex IFFT of length N/4 */
    pfftwi_512(FFTarray);

#ifndef USE_TWIDDLE_TABLE
    /* prepare for recurrence relations in post-twiddle */
    c = cosfreq8;
    s = sinfreq8;
#endif

    /* post-twiddle FFT output and then get output data */
    for (i = 0; i < 512; i++)
    {
        uint16_t n = i << 1;
        /* get post-twiddled FFT output  */
#ifdef USE_TWIDDLE_TABLE
        tempr = fac * (FFTarray[i].re * twiddlers[n] - FFTarray[i].im * twiddlers[n + 1]);
        tempi = fac * (FFTarray[i].im * twiddlers[n] + FFTarray[i].re * twiddlers[n + 1]);
#else
        tempr = fac * (FFTarray[i].re * c - FFTarray[i].im * s);
        tempi = fac * (FFTarray[i].im * c + FFTarray[i].re * s);
#endif

        /* fill in output values */
        out_data [1535 - n] = tempr;
        if (i < 256)
            out_data[1536 + n] = tempr;
        else
            out_data[n - 512] = -tempr;

        out_data [512 + n] = tempi;
        if (i < 256)
            out_data[511 - n] = -tempi;
        else
            out_data[2559 - n] = tempi;

#ifndef USE_TWIDDLE_TABLE
        /* use recurrence to prepare cosine and sine for next value of i */
        cold = c;
        c = c * cfreq - s * sfreq;
        s = s * cfreq + cold * sfreq;
#endif
    }
}

#ifdef LD_DEC
void IMDCT_1024(mdct_info *mdct, fftw_real *in_data, fftw_real *out_data)
{
    fftw_complex FFTarray[256];    /* the array for in-place FFT */
    real_t tempr, tempi, fac;

#ifdef USE_TWIDDLE_TABLE
    /* use twiddle factor tables */
    real_t *twiddlers = mdct->twiddlers;
#else
    /* temps for pre and post twiddle */
    real_t cosfreq8, sinfreq8, c, s, cold, cfreq, sfreq;
#endif
    uint16_t i;

    /* Choosing to allocate 2/N factor to Inverse Xform! */
    fac = 0.001953125f;

#ifndef USE_TWIDDLE_TABLE
    /* prepare for recurrence relation in pre-twiddle */
    cfreq = 0.9999811752826011426f;
    sfreq = 0.0061358846491544753f;
    cosfreq8 = 0.99999970586288221916f;
    sinfreq8 = 0.00076699031874270453f;

    c = cosfreq8;
    s = sinfreq8;
#endif

    for (i = 0; i < 256; i++)
    {
        uint16_t n = i << 1;
        uint16_t unscr = mdct->unscrambled[i];

        tempr = -in_data[n];
        tempi =  in_data[511 - n];

        /* calculate pre-twiddled FFT input */
#ifdef USE_TWIDDLE_TABLE
        FFTarray[unscr].re = tempr * twiddlers[n] - tempi * twiddlers[n + 1];
        FFTarray[unscr].im = tempi * twiddlers[n] + tempr * twiddlers[n + 1];
#else
        FFTarray[unscr].re = tempr * c - tempi * s;
        FFTarray[unscr].im = tempi * c + tempr * s;

        /* use recurrence to prepare cosine and sine for next value of i */
        cold = c;
        c = c * cfreq - s * sfreq;
        s = s * cfreq + cold * sfreq;
#endif
    }

    /* Perform in-place complex IFFT of length N/4 */
    pfftwi_256(FFTarray);

#ifndef USE_TWIDDLE_TABLE
    /* prepare for recurrence relations in post-twiddle */
    c = cosfreq8;
    s = sinfreq8;
#endif

    /* post-twiddle FFT output and then get output data */
    for (i = 0; i < 256; i++)
    {
        uint16_t n = i << 1;
        /* get post-twiddled FFT output  */
#ifdef USE_TWIDDLE_TABLE
        tempr = fac * (FFTarray[i].re * twiddlers[n] - FFTarray[i].im * twiddlers[n + 1]);
        tempi = fac * (FFTarray[i].im * twiddlers[n] + FFTarray[i].re * twiddlers[n + 1]);
#else
        tempr = fac * (FFTarray[i].re * c - FFTarray[i].im * s);
        tempi = fac * (FFTarray[i].im * c + FFTarray[i].re * s);
#endif

        /* fill in output values */
        out_data [767 - n] = tempr;
        if (i < 128)
            out_data[768 + n] = tempr;
        else
            out_data[n - 256] = -tempr;

        out_data [256 + n] = tempi;
        if (i < 128)
            out_data[255 - n] = -tempi;
        else
            out_data[1279 - n] = tempi;

#ifndef USE_TWIDDLE_TABLE
        /* use recurrence to prepare cosine and sine for next value of i */
        cold = c;
        c = c * cfreq - s * sfreq;
        s = s * cfreq + cold * sfreq;
#endif
    }
}
#endif

void IMDCT_256(mdct_info *mdct, fftw_real *in_data, fftw_real *out_data)
{
    fftw_complex FFTarray[64];    /* the array for in-place FFT */
    real_t tempr, tempi, fac;

#ifdef USE_TWIDDLE_TABLE
    /* use twiddle factor tables */
    real_t *twiddlers = mdct->twiddlers;
#else
    /* temps for pre and post twiddle */
    real_t cosfreq8, sinfreq8, c, s, cold, cfreq, sfreq;
#endif
    uint16_t i;

    /* Choosing to allocate 2/N factor to Inverse Xform! */
    fac = 0.0078125f; /* remaining 2/N from 4/N IFFT factor */

#ifndef USE_TWIDDLE_TABLE
    /* prepare for recurrence relation in pre-twiddle */
    cfreq = 0.99969881772994995f;
    sfreq = 0.024541229009628296f;
    cosfreq8 = 0.99999529123306274f;
    sinfreq8 = 0.0030679568483393833f;

    c = cosfreq8;
    s = sinfreq8;
#endif

    for (i = 0; i < 64; i++)
    {
        uint16_t n = i << 1;
        uint16_t unscr = mdct->unscrambled[i];

        tempr = -in_data[n];
        tempi = in_data[127 - n];

        /* calculate pre-twiddled FFT input */
#ifdef USE_TWIDDLE_TABLE
        FFTarray[unscr].re = tempr * twiddlers[n] - tempi * twiddlers[n + 1];
        FFTarray[unscr].im = tempi * twiddlers[n] + tempr * twiddlers[n + 1];
#else
        FFTarray[unscr].re = tempr * c - tempi * s;
        FFTarray[unscr].im = tempi * c + tempr * s;

        /* use recurrence to prepare cosine and sine for next value of i */
        cold = c;
        c = c * cfreq - s * sfreq;
        s = s * cfreq + cold * sfreq;
#endif
    }

    /* Perform in-place complex IFFT of length N/4 */
    pfftwi_64(FFTarray);

#ifndef USE_TWIDDLE_TABLE
    /* prepare for recurrence relations in post-twiddle */
    c = cosfreq8;
    s = sinfreq8;
#endif

    /* post-twiddle FFT output and then get output data */
    for (i = 0; i < 64; i++)
    {
        uint16_t n = i << 1;

#ifdef USE_TWIDDLE_TABLE
        /* get post-twiddled FFT output  */
        tempr = fac * (FFTarray[i].re * twiddlers[n] - FFTarray[i].im * twiddlers[n + 1]);
        tempi = fac * (FFTarray[i].im * twiddlers[n] + FFTarray[i].re * twiddlers[n + 1]);
#else
        /* get post-twiddled FFT output  */
        tempr = fac * (FFTarray[i].re * c - FFTarray[i].im * s);
        tempi = fac * (FFTarray[i].im * c + FFTarray[i].re * s);
#endif

        /* fill in output values */
        out_data [191 - n] = tempr;
        if (i < 32)
            out_data[192 + n] = tempr;
        else
            out_data[n - 64] = -tempr;

        out_data [64 + n] = tempi;
        if (i < 32)
            out_data[63 - n] = -tempi;
        else
            out_data[319 - n] = tempi;

#ifndef USE_TWIDDLE_TABLE
        /* use recurrence to prepare cosine and sine for next value of i */
        cold = c;
        c = c * cfreq - s * sfreq;
        s = s * cfreq + cold * sfreq;
#endif
    }
}

/* Fast FFT */
#ifdef _MSC_VER
#pragma warning(disable:4305)
#endif

static fftw_real K382683432[1] =
{FFTW_KONST(+0.382683432365089771728459984030398866761344562)};
static fftw_real K923879532[1] =
{FFTW_KONST(+0.923879532511286756128183189396788286822416626)};
static fftw_real K707106781[1] =
{FFTW_KONST(+0.707106781186547524400844362104849039284835938)};
static fftw_real K831469612[1] =
{FFTW_KONST(+0.831469612302545237078788377617905756738560812)};
static fftw_real K555570233[1] =
{FFTW_KONST(+0.555570233019602224742830813948532874374937191)};
static fftw_real K195090322[1] =
{FFTW_KONST(+0.195090322016128267848284868477022240927691618)};
static fftw_real K980785280[1] =
{FFTW_KONST(+0.980785280403230449126182236134239036973933731)};

static fftw_complex PFFTW(W_64)[30] = {
{ 0.995184726672197, 0.0980171403295606 },
{ 0.98078528040323, 0.195090322016128 },
{ 0.98078528040323, 0.195090322016128 },
{ 0.923879532511287, 0.38268343236509 },
{ 0.956940335732209, 0.290284677254462 },
{ 0.831469612302545, 0.555570233019602 },
{ 0.923879532511287, 0.38268343236509 },
{ 0.707106781186548, 0.707106781186547 },
{ 0.881921264348355, 0.471396736825998 },
{ 0.555570233019602, 0.831469612302545 },
{ 0.831469612302545, 0.555570233019602 },
{ 0.38268343236509, 0.923879532511287 },
{ 0.773010453362737, 0.634393284163645 },
{ 0.195090322016128, 0.98078528040323 },
{ 0.707106781186548, 0.707106781186547 },
{ 6.12303176911189e-17, 1 },
{ 0.634393284163645, 0.773010453362737 },
{ -0.195090322016128, 0.98078528040323 },
{ 0.555570233019602, 0.831469612302545 },
{ -0.38268343236509, 0.923879532511287 },
{ 0.471396736825998, 0.881921264348355 },
{ -0.555570233019602, 0.831469612302545 },
{ 0.38268343236509, 0.923879532511287 },
{ -0.707106781186547, 0.707106781186548 },
{ 0.290284677254462, 0.956940335732209 },
{ -0.831469612302545, 0.555570233019602 },
{ 0.195090322016128, 0.98078528040323 },
{ -0.923879532511287, 0.38268343236509 },
{ 0.0980171403295608, 0.995184726672197 },
{ -0.98078528040323, 0.195090322016129 },
};

static fftw_complex PFFTW(W_128)[62] = {
{ 0.998795456205172, 0.049067674327418 },
{ 0.995184726672197, 0.0980171403295606 },
{ 0.995184726672197, 0.0980171403295606 },
{ 0.98078528040323, 0.195090322016128 },
{ 0.989176509964781, 0.146730474455362 },
{ 0.956940335732209, 0.290284677254462 },
{ 0.98078528040323, 0.195090322016128 },
{ 0.923879532511287, 0.38268343236509 },
{ 0.970031253194544, 0.242980179903264 },
{ 0.881921264348355, 0.471396736825998 },
{ 0.956940335732209, 0.290284677254462 },
{ 0.831469612302545, 0.555570233019602 },
{ 0.941544065183021, 0.33688985339222 },
{ 0.773010453362737, 0.634393284163645 },
{ 0.923879532511287, 0.38268343236509 },
{ 0.707106781186548, 0.707106781186547 },
{ 0.903989293123443, 0.427555093430282 },
{ 0.634393284163645, 0.773010453362737 },
{ 0.881921264348355, 0.471396736825998 },
{ 0.555570233019602, 0.831469612302545 },
{ 0.857728610000272, 0.514102744193222 },
{ 0.471396736825998, 0.881921264348355 },
{ 0.831469612302545, 0.555570233019602 },
{ 0.38268343236509, 0.923879532511287 },
{ 0.803207531480645, 0.595699304492433 },
{ 0.290284677254462, 0.956940335732209 },
{ 0.773010453362737, 0.634393284163645 },
{ 0.195090322016128, 0.98078528040323 },
{ 0.740951125354959, 0.671558954847018 },
{ 0.0980171403295608, 0.995184726672197 },
{ 0.707106781186548, 0.707106781186547 },
{ 6.12303176911189e-17, 1 },
{ 0.671558954847018, 0.740951125354959 },
{ -0.0980171403295606, 0.995184726672197 },
{ 0.634393284163645, 0.773010453362737 },
{ -0.195090322016128, 0.98078528040323 },
{ 0.595699304492433, 0.803207531480645 },
{ -0.290284677254462, 0.956940335732209 },
{ 0.555570233019602, 0.831469612302545 },
{ -0.38268343236509, 0.923879532511287 },
{ 0.514102744193222, 0.857728610000272 },
{ -0.471396736825998, 0.881921264348355 },
{ 0.471396736825998, 0.881921264348355 },
{ -0.555570233019602, 0.831469612302545 },
{ 0.427555093430282, 0.903989293123443 },
{ -0.634393284163645, 0.773010453362737 },
{ 0.38268343236509, 0.923879532511287 },
{ -0.707106781186547, 0.707106781186548 },
{ 0.33688985339222, 0.941544065183021 },
{ -0.773010453362737, 0.634393284163645 },
{ 0.290284677254462, 0.956940335732209 },
{ -0.831469612302545, 0.555570233019602 },
{ 0.242980179903264, 0.970031253194544 },
{ -0.881921264348355, 0.471396736825998 },
{ 0.195090322016128, 0.98078528040323 },
{ -0.923879532511287, 0.38268343236509 },
{ 0.146730474455362, 0.989176509964781 },
{ -0.956940335732209, 0.290284677254462 },
{ 0.0980171403295608, 0.995184726672197 },
{ -0.98078528040323, 0.195090322016129 },
{ 0.0490676743274181, 0.998795456205172 },
{ -0.995184726672197, 0.0980171403295608 },
};

static fftw_complex PFFTW(W_256)[126] = {
{ 0.999698818696204, 0.0245412285229123 },
{ 0.998795456205172, 0.049067674327418 },
{ 0.998795456205172, 0.049067674327418 },
{ 0.995184726672197, 0.0980171403295606 },
{ 0.99729045667869, 0.0735645635996674 },
{ 0.989176509964781, 0.146730474455362 },
{ 0.995184726672197, 0.0980171403295606 },
{ 0.98078528040323, 0.195090322016128 },
{ 0.99247953459871, 0.122410675199216 },
{ 0.970031253194544, 0.242980179903264 },
{ 0.989176509964781, 0.146730474455362 },
{ 0.956940335732209, 0.290284677254462 },
{ 0.985277642388941, 0.170961888760301 },
{ 0.941544065183021, 0.33688985339222 },
{ 0.98078528040323, 0.195090322016128 },
{ 0.923879532511287, 0.38268343236509 },
{ 0.975702130038529, 0.21910124015687 },
{ 0.903989293123443, 0.427555093430282 },
{ 0.970031253194544, 0.242980179903264 },
{ 0.881921264348355, 0.471396736825998 },
{ 0.96377606579544, 0.266712757474898 },
{ 0.857728610000272, 0.514102744193222 },
{ 0.956940335732209, 0.290284677254462 },
{ 0.831469612302545, 0.555570233019602 },
{ 0.949528180593037, 0.313681740398892 },
{ 0.803207531480645, 0.595699304492433 },
{ 0.941544065183021, 0.33688985339222 },
{ 0.773010453362737, 0.634393284163645 },
{ 0.932992798834739, 0.359895036534988 },
{ 0.740951125354959, 0.671558954847018 },
{ 0.923879532511287, 0.38268343236509 },
{ 0.707106781186548, 0.707106781186547 },
{ 0.914209755703531, 0.40524131400499 },
{ 0.671558954847018, 0.740951125354959 },
{ 0.903989293123443, 0.427555093430282 },
{ 0.634393284163645, 0.773010453362737 },
{ 0.893224301195515, 0.449611329654607 },
{ 0.595699304492433, 0.803207531480645 },
{ 0.881921264348355, 0.471396736825998 },
{ 0.555570233019602, 0.831469612302545 },
{ 0.870086991108711, 0.492898192229784 },
{ 0.514102744193222, 0.857728610000272 },
{ 0.857728610000272, 0.514102744193222 },
{ 0.471396736825998, 0.881921264348355 },
{ 0.844853565249707, 0.534997619887097 },
{ 0.427555093430282, 0.903989293123443 },
{ 0.831469612302545, 0.555570233019602 },
{ 0.38268343236509, 0.923879532511287 },
{ 0.817584813151584, 0.575808191417845 },
{ 0.33688985339222, 0.941544065183021 },
{ 0.803207531480645, 0.595699304492433 },
{ 0.290284677254462, 0.956940335732209 },
{ 0.788346427626606, 0.615231590580627 },
{ 0.242980179903264, 0.970031253194544 },
{ 0.773010453362737, 0.634393284163645 },
{ 0.195090322016128, 0.98078528040323 },
{ 0.757208846506485, 0.653172842953777 },
{ 0.146730474455362, 0.989176509964781 },
{ 0.740951125354959, 0.671558954847018 },
{ 0.0980171403295608, 0.995184726672197 },
{ 0.724247082951467, 0.689540544737067 },
{ 0.0490676743274181, 0.998795456205172 },
{ 0.707106781186548, 0.707106781186547 },
{ 6.12303176911189e-17, 1 },
{ 0.689540544737067, 0.724247082951467 },
{ -0.049067674327418, 0.998795456205172 },
{ 0.671558954847018, 0.740951125354959 },
{ -0.0980171403295606, 0.995184726672197 },
{ 0.653172842953777, 0.757208846506484 },
{ -0.146730474455362, 0.989176509964781 },
{ 0.634393284163645, 0.773010453362737 },
{ -0.195090322016128, 0.98078528040323 },
{ 0.615231590580627, 0.788346427626606 },
{ -0.242980179903264, 0.970031253194544 },
{ 0.595699304492433, 0.803207531480645 },
{ -0.290284677254462, 0.956940335732209 },
{ 0.575808191417845, 0.817584813151584 },
{ -0.33688985339222, 0.941544065183021 },
{ 0.555570233019602, 0.831469612302545 },
{ -0.38268343236509, 0.923879532511287 },
{ 0.534997619887097, 0.844853565249707 },
{ -0.427555093430282, 0.903989293123443 },
{ 0.514102744193222, 0.857728610000272 },
{ -0.471396736825998, 0.881921264348355 },
{ 0.492898192229784, 0.870086991108711 },
{ -0.514102744193222, 0.857728610000272 },
{ 0.471396736825998, 0.881921264348355 },
{ -0.555570233019602, 0.831469612302545 },
{ 0.449611329654607, 0.893224301195515 },
{ -0.595699304492433, 0.803207531480645 },
{ 0.427555093430282, 0.903989293123443 },
{ -0.634393284163645, 0.773010453362737 },
{ 0.40524131400499, 0.914209755703531 },
{ -0.671558954847018, 0.740951125354959 },
{ 0.38268343236509, 0.923879532511287 },
{ -0.707106781186547, 0.707106781186548 },
{ 0.359895036534988, 0.932992798834739 },
{ -0.740951125354959, 0.671558954847019 },
{ 0.33688985339222, 0.941544065183021 },
{ -0.773010453362737, 0.634393284163645 },
{ 0.313681740398892, 0.949528180593037 },
{ -0.803207531480645, 0.595699304492433 },
{ 0.290284677254462, 0.956940335732209 },
{ -0.831469612302545, 0.555570233019602 },
{ 0.266712757474898, 0.96377606579544 },
{ -0.857728610000272, 0.514102744193222 },
{ 0.242980179903264, 0.970031253194544 },
{ -0.881921264348355, 0.471396736825998 },
{ 0.21910124015687, 0.975702130038529 },
{ -0.903989293123443, 0.427555093430282 },
{ 0.195090322016128, 0.98078528040323 },
{ -0.923879532511287, 0.38268343236509 },
{ 0.170961888760301, 0.985277642388941 },
{ -0.941544065183021, 0.33688985339222 },
{ 0.146730474455362, 0.989176509964781 },
{ -0.956940335732209, 0.290284677254462 },
{ 0.122410675199216, 0.99247953459871 },
{ -0.970031253194544, 0.242980179903264 },
{ 0.0980171403295608, 0.995184726672197 },
{ -0.98078528040323, 0.195090322016129 },
{ 0.0735645635996675, 0.99729045667869 },
{ -0.989176509964781, 0.146730474455362 },
{ 0.0490676743274181, 0.998795456205172 },
{ -0.995184726672197, 0.0980171403295608 },
{ 0.0245412285229123, 0.999698818696204 },
{ -0.998795456205172, 0.049067674327418 },
};

static fftw_complex PFFTW(W_512)[254] = {
{ 0.999924701839145, 0.0122715382857199 },
{ 0.999698818696204, 0.0245412285229123 },
{ 0.999698818696204, 0.0245412285229123 },
{ 0.998795456205172, 0.049067674327418 },
{ 0.99932238458835, 0.0368072229413588 },
{ 0.99729045667869, 0.0735645635996674 },
{ 0.998795456205172, 0.049067674327418 },
{ 0.995184726672197, 0.0980171403295606 },
{ 0.998118112900149, 0.0613207363022086 },
{ 0.99247953459871, 0.122410675199216 },
{ 0.99729045667869, 0.0735645635996674 },
{ 0.989176509964781, 0.146730474455362 },
{ 0.996312612182778, 0.0857973123444399 },
{ 0.985277642388941, 0.170961888760301 },
{ 0.995184726672197, 0.0980171403295606 },
{ 0.98078528040323, 0.195090322016128 },
{ 0.993906970002356, 0.110222207293883 },
{ 0.975702130038529, 0.21910124015687 },
{ 0.99247953459871, 0.122410675199216 },
{ 0.970031253194544, 0.242980179903264 },
{ 0.99090263542778, 0.134580708507126 },
{ 0.96377606579544, 0.266712757474898 },
{ 0.989176509964781, 0.146730474455362 },
{ 0.956940335732209, 0.290284677254462 },
{ 0.987301418157858, 0.158858143333861 },
{ 0.949528180593037, 0.313681740398892 },
{ 0.985277642388941, 0.170961888760301 },
{ 0.941544065183021, 0.33688985339222 },
{ 0.983105487431216, 0.183039887955141 },
{ 0.932992798834739, 0.359895036534988 },
{ 0.98078528040323, 0.195090322016128 },
{ 0.923879532511287, 0.38268343236509 },
{ 0.978317370719628, 0.207111376192219 },
{ 0.914209755703531, 0.40524131400499 },
{ 0.975702130038529, 0.21910124015687 },
{ 0.903989293123443, 0.427555093430282 },
{ 0.97293995220556, 0.231058108280671 },
{ 0.893224301195515, 0.449611329654607 },
{ 0.970031253194544, 0.242980179903264 },
{ 0.881921264348355, 0.471396736825998 },
{ 0.966976471044852, 0.254865659604515 },
{ 0.870086991108711, 0.492898192229784 },
{ 0.96377606579544, 0.266712757474898 },
{ 0.857728610000272, 0.514102744193222 },
{ 0.960430519415566, 0.278519689385053 },
{ 0.844853565249707, 0.534997619887097 },
{ 0.956940335732209, 0.290284677254462 },
{ 0.831469612302545, 0.555570233019602 },
{ 0.953306040354194, 0.302005949319228 },
{ 0.817584813151584, 0.575808191417845 },
{ 0.949528180593037, 0.313681740398892 },
{ 0.803207531480645, 0.595699304492433 },
{ 0.945607325380521, 0.325310292162263 },
{ 0.788346427626606, 0.615231590580627 },
{ 0.941544065183021, 0.33688985339222 },
{ 0.773010453362737, 0.634393284163645 },
{ 0.937339011912575, 0.348418680249435 },
{ 0.757208846506485, 0.653172842953777 },
{ 0.932992798834739, 0.359895036534988 },
{ 0.740951125354959, 0.671558954847018 },
{ 0.928506080473216, 0.371317193951838 },
{ 0.724247082951467, 0.689540544737067 },
{ 0.923879532511287, 0.38268343236509 },
{ 0.707106781186548, 0.707106781186547 },
{ 0.919113851690058, 0.393992040061048 },
{ 0.689540544737067, 0.724247082951467 },
{ 0.914209755703531, 0.40524131400499 },
{ 0.671558954847018, 0.740951125354959 },
{ 0.909167983090522, 0.416429560097637 },
{ 0.653172842953777, 0.757208846506484 },
{ 0.903989293123443, 0.427555093430282 },
{ 0.634393284163645, 0.773010453362737 },
{ 0.898674465693954, 0.438616238538528 },
{ 0.615231590580627, 0.788346427626606 },
{ 0.893224301195515, 0.449611329654607 },
{ 0.595699304492433, 0.803207531480645 },
{ 0.887639620402854, 0.46053871095824 },
{ 0.575808191417845, 0.817584813151584 },
{ 0.881921264348355, 0.471396736825998 },
{ 0.555570233019602, 0.831469612302545 },
{ 0.876070094195407, 0.482183772079123 },
{ 0.534997619887097, 0.844853565249707 },
{ 0.870086991108711, 0.492898192229784 },
{ 0.514102744193222, 0.857728610000272 },
{ 0.863972856121587, 0.503538383725718 },
{ 0.492898192229784, 0.870086991108711 },
{ 0.857728610000272, 0.514102744193222 },
{ 0.471396736825998, 0.881921264348355 },
{ 0.851355193105265, 0.524589682678469 },
{ 0.449611329654607, 0.893224301195515 },
{ 0.844853565249707, 0.534997619887097 },
{ 0.427555093430282, 0.903989293123443 },
{ 0.838224705554838, 0.545324988422046 },
{ 0.40524131400499, 0.914209755703531 },
{ 0.831469612302545, 0.555570233019602 },
{ 0.38268343236509, 0.923879532511287 },
{ 0.824589302785025, 0.565731810783613 },
{ 0.359895036534988, 0.932992798834739 },
{ 0.817584813151584, 0.575808191417845 },
{ 0.33688985339222, 0.941544065183021 },
{ 0.810457198252595, 0.585797857456439 },
{ 0.313681740398892, 0.949528180593037 },
{ 0.803207531480645, 0.595699304492433 },
{ 0.290284677254462, 0.956940335732209 },
{ 0.795836904608884, 0.605511041404326 },
{ 0.266712757474898, 0.96377606579544 },
{ 0.788346427626606, 0.615231590580627 },
{ 0.242980179903264, 0.970031253194544 },
{ 0.780737228572094, 0.624859488142386 },
{ 0.21910124015687, 0.975702130038529 },
{ 0.773010453362737, 0.634393284163645 },
{ 0.195090322016128, 0.98078528040323 },
{ 0.765167265622459, 0.643831542889791 },
{ 0.170961888760301, 0.985277642388941 },
{ 0.757208846506485, 0.653172842953777 },
{ 0.146730474455362, 0.989176509964781 },
{ 0.749136394523459, 0.662415777590172 },
{ 0.122410675199216, 0.99247953459871 },
{ 0.740951125354959, 0.671558954847018 },
{ 0.0980171403295608, 0.995184726672197 },
{ 0.732654271672413, 0.680600997795453 },
{ 0.0735645635996675, 0.99729045667869 },
{ 0.724247082951467, 0.689540544737067 },
{ 0.0490676743274181, 0.998795456205172 },
{ 0.715730825283819, 0.698376249408973 },
{ 0.0245412285229123, 0.999698818696204 },
{ 0.707106781186548, 0.707106781186547 },
{ 6.12303176911189e-17, 1 },
{ 0.698376249408973, 0.715730825283819 },
{ -0.0245412285229121, 0.999698818696204 },
{ 0.689540544737067, 0.724247082951467 },
{ -0.049067674327418, 0.998795456205172 },
{ 0.680600997795453, 0.732654271672413 },
{ -0.0735645635996673, 0.99729045667869 },
{ 0.671558954847018, 0.740951125354959 },
{ -0.0980171403295606, 0.995184726672197 },
{ 0.662415777590172, 0.749136394523459 },
{ -0.122410675199216, 0.99247953459871 },
{ 0.653172842953777, 0.757208846506484 },
{ -0.146730474455362, 0.989176509964781 },
{ 0.643831542889791, 0.765167265622459 },
{ -0.170961888760301, 0.985277642388941 },
{ 0.634393284163645, 0.773010453362737 },
{ -0.195090322016128, 0.98078528040323 },
{ 0.624859488142386, 0.780737228572094 },
{ -0.21910124015687, 0.975702130038529 },
{ 0.615231590580627, 0.788346427626606 },
{ -0.242980179903264, 0.970031253194544 },
{ 0.605511041404326, 0.795836904608883 },
{ -0.266712757474898, 0.96377606579544 },
{ 0.595699304492433, 0.803207531480645 },
{ -0.290284677254462, 0.956940335732209 },
{ 0.585797857456439, 0.810457198252595 },
{ -0.313681740398891, 0.949528180593037 },
{ 0.575808191417845, 0.817584813151584 },
{ -0.33688985339222, 0.941544065183021 },
{ 0.565731810783613, 0.824589302785025 },
{ -0.359895036534988, 0.932992798834739 },
{ 0.555570233019602, 0.831469612302545 },
{ -0.38268343236509, 0.923879532511287 },
{ 0.545324988422046, 0.838224705554838 },
{ -0.40524131400499, 0.914209755703531 },
{ 0.534997619887097, 0.844853565249707 },
{ -0.427555093430282, 0.903989293123443 },
{ 0.524589682678469, 0.851355193105265 },
{ -0.449611329654607, 0.893224301195515 },
{ 0.514102744193222, 0.857728610000272 },
{ -0.471396736825998, 0.881921264348355 },
{ 0.503538383725718, 0.863972856121587 },
{ -0.492898192229784, 0.870086991108711 },
{ 0.492898192229784, 0.870086991108711 },
{ -0.514102744193222, 0.857728610000272 },
{ 0.482183772079123, 0.876070094195407 },
{ -0.534997619887097, 0.844853565249707 },
{ 0.471396736825998, 0.881921264348355 },
{ -0.555570233019602, 0.831469612302545 },
{ 0.46053871095824, 0.887639620402854 },
{ -0.575808191417845, 0.817584813151584 },
{ 0.449611329654607, 0.893224301195515 },
{ -0.595699304492433, 0.803207531480645 },
{ 0.438616238538528, 0.898674465693954 },
{ -0.615231590580627, 0.788346427626606 },
{ 0.427555093430282, 0.903989293123443 },
{ -0.634393284163645, 0.773010453362737 },
{ 0.416429560097637, 0.909167983090522 },
{ -0.653172842953777, 0.757208846506485 },
{ 0.40524131400499, 0.914209755703531 },
{ -0.671558954847018, 0.740951125354959 },
{ 0.393992040061048, 0.919113851690058 },
{ -0.689540544737067, 0.724247082951467 },
{ 0.38268343236509, 0.923879532511287 },
{ -0.707106781186547, 0.707106781186548 },
{ 0.371317193951838, 0.928506080473215 },
{ -0.724247082951467, 0.689540544737067 },
{ 0.359895036534988, 0.932992798834739 },
{ -0.740951125354959, 0.671558954847019 },
{ 0.348418680249435, 0.937339011912575 },
{ -0.757208846506485, 0.653172842953777 },
{ 0.33688985339222, 0.941544065183021 },
{ -0.773010453362737, 0.634393284163645 },
{ 0.325310292162263, 0.945607325380521 },
{ -0.788346427626606, 0.615231590580627 },
{ 0.313681740398892, 0.949528180593037 },
{ -0.803207531480645, 0.595699304492433 },
{ 0.302005949319228, 0.953306040354194 },
{ -0.817584813151584, 0.575808191417845 },
{ 0.290284677254462, 0.956940335732209 },
{ -0.831469612302545, 0.555570233019602 },
{ 0.278519689385053, 0.960430519415566 },
{ -0.844853565249707, 0.534997619887097 },
{ 0.266712757474898, 0.96377606579544 },
{ -0.857728610000272, 0.514102744193222 },
{ 0.254865659604515, 0.966976471044852 },
{ -0.870086991108711, 0.492898192229784 },
{ 0.242980179903264, 0.970031253194544 },
{ -0.881921264348355, 0.471396736825998 },
{ 0.231058108280671, 0.97293995220556 },
{ -0.893224301195515, 0.449611329654607 },
{ 0.21910124015687, 0.975702130038529 },
{ -0.903989293123443, 0.427555093430282 },
{ 0.207111376192219, 0.978317370719628 },
{ -0.914209755703531, 0.40524131400499 },
{ 0.195090322016128, 0.98078528040323 },
{ -0.923879532511287, 0.38268343236509 },
{ 0.183039887955141, 0.983105487431216 },
{ -0.932992798834739, 0.359895036534988 },
{ 0.170961888760301, 0.985277642388941 },
{ -0.941544065183021, 0.33688985339222 },
{ 0.158858143333861, 0.987301418157858 },
{ -0.949528180593037, 0.313681740398891 },
{ 0.146730474455362, 0.989176509964781 },
{ -0.956940335732209, 0.290284677254462 },
{ 0.134580708507126, 0.99090263542778 },
{ -0.96377606579544, 0.266712757474898 },
{ 0.122410675199216, 0.99247953459871 },
{ -0.970031253194544, 0.242980179903264 },
{ 0.110222207293883, 0.993906970002356 },
{ -0.975702130038528, 0.21910124015687 },
{ 0.0980171403295608, 0.995184726672197 },
{ -0.98078528040323, 0.195090322016129 },
{ 0.0857973123444399, 0.996312612182778 },
{ -0.985277642388941, 0.170961888760301 },
{ 0.0735645635996675, 0.99729045667869 },
{ -0.989176509964781, 0.146730474455362 },
{ 0.0613207363022086, 0.998118112900149 },
{ -0.99247953459871, 0.122410675199216 },
{ 0.0490676743274181, 0.998795456205172 },
{ -0.995184726672197, 0.0980171403295608 },
{ 0.036807222941359, 0.99932238458835 },
{ -0.99729045667869, 0.0735645635996677 },
{ 0.0245412285229123, 0.999698818696204 },
{ -0.998795456205172, 0.049067674327418 },
{ 0.0122715382857199, 0.999924701839145 },
{ -0.999698818696204, 0.0245412285229123 },
};

static void PFFTW(16) (fftw_complex * input) {
     fftw_real tmp332;
     fftw_real tmp331;
     fftw_real tmp330;
     fftw_real tmp329;
     fftw_real tmp328;
     fftw_real tmp327;
     fftw_real tmp326;
     fftw_real tmp325;
     fftw_real tmp324;
     fftw_real tmp323;
     fftw_real tmp322;
     fftw_real tmp321;
     fftw_real tmp320;
     fftw_real tmp319;
     fftw_real tmp318;
     fftw_real tmp317;
     fftw_real tmp316;
     fftw_real tmp315;
     fftw_real tmp314;
     fftw_real tmp313;
     fftw_real tmp312;
     fftw_real tmp311;
     fftw_real tmp310;
     fftw_real tmp309;
     fftw_real tmp308;
     fftw_real tmp307;
     fftw_real tmp306;
     fftw_real tmp305;
     fftw_real tmp304;
     fftw_real tmp303;
     fftw_real tmp302;
     fftw_real tmp301;
     fftw_real st1;
     fftw_real st2;
     fftw_real st3;
     fftw_real st4;
     fftw_real st5;
     fftw_real st6;
     fftw_real st7;
     fftw_real st8;
     st8 = c_re(input[0]);
     st8 = st8 - c_re(input[8]);
     st7 = c_im(input[4]);
     st7 = st7 - c_im(input[12]);
     st6 = c_re(input[4]);
     st5 = st8 - st7;
     st8 = st8 + st7;
     st6 = st6 - c_re(input[12]);
     st4 = c_im(input[0]);
     st4 = st4 - c_im(input[8]);
     st3 = c_re(input[0]);
     st2 = st6 + st4;
     st4 = st4 - st6;
     st3 = st3 + c_re(input[8]);
     st1 = c_re(input[4]);
     st1 = st1 + c_re(input[12]);
     st7 = c_im(input[0]);
     st6 = st3 + st1;
     st3 = st3 - st1;
     st7 = st7 + c_im(input[8]);
     st1 = c_im(input[4]);
     st1 = st1 + c_im(input[12]);
     tmp301 = st4;
     st4 = c_re(input[2]);
     tmp302 = st8;
     st8 = st7 + st1;
     st7 = st7 - st1;
     st4 = st4 + c_re(input[10]);
     st1 = c_re(input[6]);
     st1 = st1 + c_re(input[14]);
     tmp303 = st2;
     st2 = c_im(input[2]);
     tmp304 = st5;
     st5 = st4 + st1;
     st4 = st4 - st1;
     st2 = st2 + c_im(input[10]);
     st1 = st6 + st5;
     st6 = st6 - st5;
     st5 = st4 + st7;
     st7 = st7 - st4;
     st4 = c_im(input[6]);
     st4 = st4 + c_im(input[14]);
     tmp305 = st5;
     st5 = c_re(input[6]);
     tmp306 = st7;
     st7 = st2 + st4;
     st2 = st2 - st4;
     st4 = st8 - st7;
     st8 = st8 + st7;
     st7 = st3 - st2;
     st3 = st3 + st2;
     st5 = st5 - c_re(input[14]);
     st2 = c_im(input[2]);
     st2 = st2 - c_im(input[10]);
     tmp307 = st3;
     st3 = c_re(input[2]);
     tmp308 = st6;
     st6 = st5 + st2;
     st2 = st2 - st5;
     st3 = st3 - c_re(input[10]);
     st5 = c_im(input[6]);
     st5 = st5 - c_im(input[14]);
     tmp309 = st7;
     st7 = c_re(input[5]);
     tmp310 = st8;
     st8 = st3 - st5;
     st3 = st3 + st5;
     st5 = st6 - st8;
     st6 = st6 + st8;
     st5 = st5 * K707106781[0];
     st8 = st2 + st3;
     st6 = st6 * K707106781[0];
     st2 = st2 - st3;
     st8 = st8 * K707106781[0];
     st7 = st7 - c_re(input[13]);
     st2 = st2 * K707106781[0];
     st3 = tmp304 + st5;
     tmp311 = st4;
     st4 = tmp303 + st6;
     st6 = tmp303 - st6;
     st5 = tmp304 - st5;
     tmp312 = st1;
     st1 = tmp302 - st8;
     st8 = tmp302 + st8;
     tmp313 = st8;
     st8 = tmp301 + st2;
     st2 = tmp301 - st2;
     tmp314 = st2;
     st2 = c_im(input[1]);
     st2 = st2 - c_im(input[9]);
     tmp315 = st8;
     st8 = c_re(input[1]);
     tmp316 = st1;
     st1 = st7 + st2;
     st2 = st2 - st7;
     st7 = st1 * K923879532[0];
     st8 = st8 - c_re(input[9]);
     st1 = st1 * K382683432[0];
     tmp317 = st5;
     st5 = c_im(input[5]);
     tmp318 = st6;
     st6 = st2 * K923879532[0];
     st5 = st5 - c_im(input[13]);
     st2 = st2 * K382683432[0];
     tmp319 = st4;
     st4 = st8 - st5;
     st8 = st8 + st5;
     st5 = st4 * K382683432[0];
     tmp320 = st3;
     st3 = c_re(input[7]);
     st4 = st4 * K923879532[0];
     st7 = st7 + st5;
     st5 = st8 * K382683432[0];
     st1 = st1 - st4;
     st8 = st8 * K923879532[0];
     st6 = st6 - st5;
     st2 = st2 + st8;
     st3 = st3 - c_re(input[15]);
     st4 = c_im(input[3]);
     st4 = st4 - c_im(input[11]);
     st5 = c_re(input[3]);
     st8 = st3 + st4;
     st4 = st4 - st3;
     st3 = st8 * K382683432[0];
     st5 = st5 - c_re(input[11]);
     st8 = st8 * K923879532[0];
     tmp321 = st2;
     st2 = c_im(input[7]);
     tmp322 = st6;
     st6 = st4 * K382683432[0];
     st2 = st2 - c_im(input[15]);
     st4 = st4 * K923879532[0];
     tmp323 = st1;
     st1 = st5 - st2;
     st5 = st5 + st2;
     st2 = st1 * K923879532[0];
     tmp324 = st7;
     st7 = c_re(input[1]);
     st1 = st1 * K382683432[0];
     st3 = st3 + st2;
     st2 = st5 * K923879532[0];
     st1 = st1 - st8;
     st5 = st5 * K382683432[0];
     st6 = st6 - st2;
     st4 = st4 + st5;
     st7 = st7 + c_re(input[9]);
     st8 = tmp324 - st3;
     st3 = tmp324 + st3;
     st2 = tmp320 - st8;
     st8 = tmp320 + st8;
     st5 = tmp319 - st3;
     st3 = tmp319 + st3;
     tmp325 = st3;
     st3 = tmp323 + st1;
     st1 = tmp323 - st1;
     tmp326 = st5;
     st5 = tmp318 - st3;
     st3 = tmp318 + st3;
     tmp327 = st5;
     st5 = tmp317 - st1;
     st1 = tmp317 + st1;
     tmp328 = st3;
     st3 = tmp322 - st6;
     tmp329 = st5;
     st5 = tmp321 + st4;
     tmp330 = st1;
     st1 = tmp316 - st3;
     st3 = tmp316 + st3;
     tmp331 = st2;
     st2 = tmp313 - st5;
     st5 = tmp313 + st5;
     st6 = tmp322 + st6;
     c_re(input[9]) = st2;
     c_re(input[1]) = st5;
     st2 = tmp315 - st6;
     st6 = tmp315 + st6;
     st4 = tmp321 - st4;
     st5 = c_re(input[5]);
     c_re(input[5]) = st3;
     st5 = st5 + c_re(input[13]);
     c_re(input[13]) = st1;
     st1 = st7 + st5;
     st7 = st7 - st5;
     st3 = tmp314 - st4;
     st4 = tmp314 + st4;
     st5 = c_im(input[1]);
     c_im(input[1]) = st6;
     st5 = st5 + c_im(input[9]);
     c_im(input[9]) = st2;
     st2 = c_im(input[5]);
     c_im(input[5]) = st3;
     st2 = st2 + c_im(input[13]);
     c_im(input[13]) = st4;
     st6 = st5 - st2;
     st5 = st5 + st2;
     st3 = c_re(input[3]);
     c_re(input[3]) = st8;
     st3 = st3 + c_re(input[11]);
     c_re(input[11]) = tmp331;
     st8 = c_re(input[7]);
     c_re(input[7]) = tmp330;
     st8 = st8 + c_re(input[15]);
     c_re(input[15]) = tmp329;
     st4 = st3 + st8;
     st3 = st3 - st8;
     st2 = st1 + st4;
     st1 = st1 - st4;
     st8 = st3 + st6;
     st6 = st6 - st3;
     st4 = tmp312 - st2;
     st2 = tmp312 + st2;
     st3 = tmp311 - st1;
     c_re(input[8]) = st4;
     c_re(input[0]) = st2;
     c_im(input[4]) = st3;
     st1 = st1 + tmp311;
     st4 = c_im(input[3]);
     c_im(input[3]) = tmp328;
     c_im(input[12]) = st1;
     st4 = st4 + c_im(input[11]);
     c_im(input[11]) = tmp327;
     st2 = c_im(input[7]);
     c_im(input[7]) = tmp326;
     st2 = st2 + c_im(input[15]);
     c_im(input[15]) = tmp325;
     st3 = st4 - st2;
     st4 = st4 + st2;
     st1 = st7 - st3;
     st2 = st5 - st4;
     tmp332 = st2;
     st2 = st8 - st1;
     st8 = st8 + st1;
     st2 = st2 * K707106781[0];
     st5 = st5 + st4;
     st8 = st8 * K707106781[0];
     st7 = st7 + st3;
     st3 = tmp310 + st5;
     st4 = st6 - st7;
     st6 = st6 + st7;
     c_im(input[0]) = st3;
     st4 = st4 * K707106781[0];
     st5 = tmp310 - st5;
     st6 = st6 * K707106781[0];
     st1 = tmp309 - st2;
     c_im(input[8]) = st5;
     c_re(input[14]) = st1;
     st2 = tmp309 + st2;
     st7 = tmp308 + tmp332;
     st3 = tmp308 - tmp332;
     c_re(input[6]) = st2;
     c_re(input[4]) = st7;
     c_re(input[12]) = st3;
     st5 = tmp306 - st4;
     st4 = tmp306 + st4;
     st1 = tmp307 - st6;
     c_im(input[10]) = st5;
     c_im(input[2]) = st4;
     c_re(input[10]) = st1;
     st6 = tmp307 + st6;
     st2 = tmp305 - st8;
     st8 = tmp305 + st8;
     c_re(input[2]) = st6;
     c_im(input[6]) = st2;
     c_im(input[14]) = st8;
}

static void PFFTW(32) (fftw_complex * input) {
     fftw_real tmp714;
     fftw_real tmp713;
     fftw_real tmp712;
     fftw_real tmp711;
     fftw_real tmp710;
     fftw_real tmp709;
     fftw_real tmp708;
     fftw_real tmp707;
     fftw_real tmp706;
     fftw_real tmp705;
     fftw_real tmp704;
     fftw_real tmp703;
     fftw_real tmp702;
     fftw_real tmp701;
     fftw_real tmp700;
     fftw_real tmp699;
     fftw_real tmp698;
     fftw_real tmp697;
     fftw_real tmp696;
     fftw_real tmp695;
     fftw_real tmp694;
     fftw_real tmp693;
     fftw_real tmp692;
     fftw_real tmp691;
     fftw_real tmp690;
     fftw_real tmp689;
     fftw_real tmp688;
     fftw_real tmp687;
     fftw_real tmp686;
     fftw_real tmp685;
     fftw_real tmp684;
     fftw_real tmp683;
     fftw_real tmp682;
     fftw_real tmp681;
     fftw_real tmp680;
     fftw_real tmp679;
     fftw_real tmp678;
     fftw_real tmp677;
     fftw_real tmp676;
     fftw_real tmp675;
     fftw_real tmp674;
     fftw_real tmp673;
     fftw_real tmp672;
     fftw_real tmp671;
     fftw_real tmp670;
     fftw_real tmp669;
     fftw_real tmp668;
     fftw_real tmp667;
     fftw_real tmp666;
     fftw_real tmp665;
     fftw_real tmp664;
     fftw_real tmp663;
     fftw_real tmp662;
     fftw_real tmp661;
     fftw_real tmp660;
     fftw_real tmp659;
     fftw_real tmp658;
     fftw_real tmp657;
     fftw_real tmp656;
     fftw_real tmp655;
     fftw_real tmp654;
     fftw_real tmp653;
     fftw_real tmp652;
     fftw_real tmp651;
     fftw_real tmp650;
     fftw_real tmp649;
     fftw_real tmp648;
     fftw_real tmp647;
     fftw_real tmp646;
     fftw_real tmp645;
     fftw_real tmp644;
     fftw_real tmp643;
     fftw_real tmp642;
     fftw_real tmp641;
     fftw_real tmp640;
     fftw_real tmp639;
     fftw_real tmp638;
     fftw_real tmp637;
     fftw_real tmp636;
     fftw_real tmp635;
     fftw_real tmp634;
     fftw_real tmp633;
     fftw_real tmp632;
     fftw_real tmp631;
     fftw_real tmp630;
     fftw_real tmp629;
     fftw_real tmp628;
     fftw_real tmp627;
     fftw_real tmp626;
     fftw_real tmp625;
     fftw_real tmp624;
     fftw_real tmp623;
     fftw_real tmp622;
     fftw_real tmp621;
     fftw_real st1;
     fftw_real st2;
     fftw_real st3;
     fftw_real st4;
     fftw_real st5;
     fftw_real st6;
     fftw_real st7;
     fftw_real st8;
     st8 = c_re(input[0]);
     st8 = st8 - c_re(input[16]);
     st7 = c_im(input[8]);
     st7 = st7 - c_im(input[24]);
     st6 = st8 - st7;
     st8 = st8 + st7;
     st5 = c_re(input[0]);
     st5 = st5 + c_re(input[16]);
     st4 = c_re(input[8]);
     st4 = st4 + c_re(input[24]);
     st3 = st5 + st4;
     st5 = st5 - st4;
     st2 = c_im(input[0]);
     st2 = st2 + c_im(input[16]);
     st1 = c_im(input[8]);
     st1 = st1 + c_im(input[24]);
     st7 = st2 + st1;
     st2 = st2 - st1;
     st4 = c_re(input[4]);
     st4 = st4 + c_re(input[20]);
     st1 = c_re(input[28]);
     st1 = st1 + c_re(input[12]);
     tmp621 = st6;
     st6 = st4 + st1;
     st1 = st1 - st4;
     st4 = st3 + st6;
     st3 = st3 - st6;
     st6 = st2 - st1;
     st1 = st1 + st2;
     st2 = c_im(input[4]);
     st2 = st2 + c_im(input[20]);
     tmp622 = st1;
     st1 = c_im(input[28]);
     st1 = st1 + c_im(input[12]);
     tmp623 = st6;
     st6 = st2 + st1;
     st2 = st2 - st1;
     st1 = st7 + st6;
     st7 = st7 - st6;
     st6 = st5 - st2;
     st5 = st5 + st2;
     st2 = c_re(input[8]);
     st2 = st2 - c_re(input[24]);
     tmp624 = st5;
     st5 = c_im(input[0]);
     st5 = st5 - c_im(input[16]);
     tmp625 = st6;
     st6 = st2 + st5;
     st5 = st5 - st2;
     st2 = c_im(input[4]);
     st2 = st2 - c_im(input[20]);
     tmp626 = st3;
     st3 = c_re(input[4]);
     st3 = st3 - c_re(input[20]);
     tmp627 = st1;
     st1 = st2 - st3;
     st3 = st3 + st2;
     st2 = c_re(input[28]);
     st2 = st2 - c_re(input[12]);
     tmp628 = st7;
     st7 = c_im(input[28]);
     st7 = st7 - c_im(input[12]);
     tmp629 = st4;
     st4 = st2 + st7;
     st2 = st2 - st7;
     st7 = st1 - st4;
     st7 = st7 * K707106781[0];
     st1 = st1 + st4;
     st1 = st1 * K707106781[0];
     st4 = st2 - st3;
     st4 = st4 * K707106781[0];
     st3 = st3 + st2;
     st3 = st3 * K707106781[0];
     st2 = st8 - st3;
     tmp630 = st2;
     st2 = st5 - st1;
     tmp631 = st2;
     st2 = tmp621 - st7;
     tmp632 = st2;
     st2 = st6 - st4;
     st7 = tmp621 + st7;
     st6 = st6 + st4;
     st8 = st8 + st3;
     st5 = st5 + st1;
     st1 = c_re(input[2]);
     st1 = st1 + c_re(input[18]);
     st4 = c_re(input[10]);
     st4 = st4 + c_re(input[26]);
     st3 = st1 + st4;
     st1 = st1 - st4;
     st4 = c_im(input[2]);
     st4 = st4 + c_im(input[18]);
     tmp633 = st5;
     st5 = c_im(input[10]);
     st5 = st5 + c_im(input[26]);
     tmp634 = st8;
     st8 = st4 + st5;
     st4 = st4 - st5;
     st5 = st1 + st4;
     st4 = st4 - st1;
     st1 = c_re(input[30]);
     st1 = st1 + c_re(input[14]);
     tmp635 = st6;
     st6 = c_re(input[6]);
     st6 = st6 + c_re(input[22]);
     tmp636 = st7;
     st7 = st1 + st6;
     st1 = st1 - st6;
     st6 = st3 + st7;
     st7 = st7 - st3;
     st3 = tmp629 + st6;
     st6 = tmp629 - st6;
     tmp637 = st6;
     st6 = st7 + tmp628;
     st7 = tmp628 - st7;
     tmp638 = st7;
     st7 = c_im(input[30]);
     st7 = st7 + c_im(input[14]);
     tmp639 = st6;
     st6 = c_im(input[6]);
     st6 = st6 + c_im(input[22]);
     tmp640 = st3;
     st3 = st7 + st6;
     st7 = st7 - st6;
     st6 = st8 + st3;
     st8 = st8 - st3;
     st3 = st1 - st7;
     tmp641 = st2;
     st2 = st3 - st5;
     st2 = st2 * K707106781[0];
     st5 = st5 + st3;
     st5 = st5 * K707106781[0];
     st1 = st1 + st7;
     st7 = st4 - st1;
     st7 = st7 * K707106781[0];
     st4 = st4 + st1;
     st4 = st4 * K707106781[0];
     st3 = tmp627 - st6;
     st6 = tmp627 + st6;
     st1 = tmp626 - st8;
     st8 = tmp626 + st8;
     tmp642 = st8;
     st8 = tmp625 + st7;
     st7 = tmp625 - st7;
     tmp643 = st7;
     st7 = tmp623 - st2;
     st2 = tmp623 + st2;
     tmp644 = st2;
     st2 = tmp624 + st5;
     st5 = tmp624 - st5;
     tmp645 = st5;
     st5 = tmp622 - st4;
     st4 = tmp622 + st4;
     tmp646 = st4;
     st4 = c_re(input[6]);
     st4 = st4 - c_re(input[22]);
     tmp647 = st5;
     st5 = c_im(input[30]);
     st5 = st5 - c_im(input[14]);
     tmp648 = st2;
     st2 = st4 + st5;
     tmp649 = st7;
     st7 = st2 * K382683432[0];
     st5 = st5 - st4;
     st2 = st2 * K923879532[0];
     st4 = c_re(input[30]);
     tmp650 = st8;
     st8 = st5 * K923879532[0];
     st4 = st4 - c_re(input[14]);
     st5 = st5 * K382683432[0];
     tmp651 = st1;
     st1 = c_im(input[6]);
     st1 = st1 - c_im(input[22]);
     tmp652 = st6;
     st6 = st4 - st1;
     tmp653 = st3;
     st3 = st6 * K923879532[0];
     st4 = st4 + st1;
     st6 = st6 * K382683432[0];
     st7 = st7 + st3;
     st1 = st4 * K382683432[0];
     st6 = st6 - st2;
     st4 = st4 * K923879532[0];
     st8 = st8 + st1;
     st4 = st4 - st5;
     st2 = c_re(input[10]);
     st2 = st2 - c_re(input[26]);
     st5 = c_im(input[2]);
     st5 = st5 - c_im(input[18]);
     st3 = st2 + st5;
     st1 = st3 * K382683432[0];
     st5 = st5 - st2;
     st3 = st3 * K923879532[0];
     st2 = c_re(input[2]);
     tmp654 = st6;
     st6 = st5 * K923879532[0];
     st2 = st2 - c_re(input[18]);
     st5 = st5 * K382683432[0];
     tmp655 = st7;
     st7 = c_im(input[10]);
     st7 = st7 - c_im(input[26]);
     tmp656 = st4;
     st4 = st2 - st7;
     tmp657 = st8;
     st8 = st4 * K923879532[0];
     st2 = st2 + st7;
     st4 = st4 * K382683432[0];
     st1 = st1 - st8;
     st7 = st2 * K382683432[0];
     st3 = st3 + st4;
     st2 = st2 * K923879532[0];
     st6 = st6 - st7;
     st5 = st5 + st2;
     st8 = st6 - tmp657;
     st4 = tmp630 + st8;
     st8 = tmp630 - st8;
     st7 = tmp656 - st5;
     st2 = tmp631 - st7;
     st7 = tmp631 + st7;
     tmp658 = st7;
     st7 = st1 - tmp655;
     tmp659 = st8;
     st8 = tmp632 + st7;
     st7 = tmp632 - st7;
     tmp660 = st7;
     st7 = tmp654 - st3;
     tmp661 = st8;
     st8 = tmp641 - st7;
     st7 = tmp641 + st7;
     st3 = st3 + tmp654;
     tmp662 = st7;
     st7 = tmp636 + st3;
     st3 = tmp636 - st3;
     st1 = st1 + tmp655;
     tmp663 = st3;
     st3 = tmp635 - st1;
     st1 = tmp635 + st1;
     st5 = st5 + tmp656;
     tmp664 = st1;
     st1 = tmp634 + st5;
     st5 = tmp634 - st5;
     st6 = st6 + tmp657;
     tmp665 = st5;
     st5 = tmp633 - st6;
     st6 = tmp633 + st6;
     tmp666 = st6;
     st6 = c_re(input[1]);
     st6 = st6 + c_re(input[17]);
     tmp667 = st5;
     st5 = c_re(input[9]);
     st5 = st5 + c_re(input[25]);
     tmp668 = st1;
     st1 = st6 + st5;
     st6 = st6 - st5;
     st5 = c_im(input[1]);
     st5 = st5 + c_im(input[17]);
     tmp669 = st3;
     st3 = c_im(input[9]);
     st3 = st3 + c_im(input[25]);
     tmp670 = st7;
     st7 = st5 - st3;
     st5 = st5 + st3;
     st3 = c_re(input[5]);
     st3 = st3 + c_re(input[21]);
     tmp671 = st8;
     st8 = c_re(input[29]);
     st8 = st8 + c_re(input[13]);
     tmp672 = st2;
     st2 = st3 + st8;
     st8 = st8 - st3;
     st3 = st1 + st2;
     st1 = st1 - st2;
     st2 = st8 + st7;
     tmp673 = st4;
     st4 = st2 * K382683432[0];
     st7 = st7 - st8;
     st2 = st2 * K923879532[0];
     st8 = c_im(input[5]);
     tmp674 = st3;
     st3 = st7 * K923879532[0];
     st8 = st8 + c_im(input[21]);
     st7 = st7 * K382683432[0];
     tmp675 = st4;
     st4 = c_im(input[29]);
     st4 = st4 + c_im(input[13]);
     tmp676 = st7;
     st7 = st8 - st4;
     st8 = st8 + st4;
     st4 = st5 + st8;
     st5 = st5 - st8;
     st8 = st1 + st5;
     st5 = st5 - st1;
     st1 = st6 + st7;
     tmp677 = st8;
     st8 = st1 * K923879532[0];
     st6 = st6 - st7;
     st1 = st1 * K382683432[0];
     st2 = st2 - st1;
     st7 = st6 * K382683432[0];
     st3 = st3 + st7;
     st6 = st6 * K923879532[0];
     st6 = tmp676 - st6;
     st8 = tmp675 + st8;
     st1 = c_re(input[31]);
     st1 = st1 + c_re(input[15]);
     st7 = c_re(input[7]);
     st7 = st7 + c_re(input[23]);
     tmp678 = st8;
     st8 = st1 + st7;
     st1 = st1 - st7;
     st7 = c_im(input[31]);
     st7 = st7 + c_im(input[15]);
     tmp679 = st2;
     st2 = c_im(input[7]);
     st2 = st2 + c_im(input[23]);
     tmp680 = st3;
     st3 = st7 - st2;
     st7 = st7 + st2;
     st2 = c_re(input[3]);
     st2 = st2 + c_re(input[19]);
     tmp681 = st6;
     st6 = c_re(input[27]);
     st6 = st6 + c_re(input[11]);
     tmp682 = st5;
     st5 = st2 + st6;
     st6 = st6 - st2;
     st2 = st8 + st5;
     st8 = st8 - st5;
     st5 = st6 + st3;
     tmp683 = st4;
     st4 = st5 * K382683432[0];
     st3 = st3 - st6;
     st5 = st5 * K923879532[0];
     st6 = tmp674 + st2;
     tmp684 = st4;
     st4 = st3 * K923879532[0];
     tmp685 = st5;
     st5 = tmp640 - st6;
     st3 = st3 * K382683432[0];
     st6 = tmp640 + st6;
     st2 = st2 - tmp674;
     c_re(input[16]) = st5;
     st5 = st2 + tmp653;
     st2 = tmp653 - st2;
     c_re(input[0]) = st6;
     st6 = c_im(input[3]);
     st6 = st6 + c_im(input[19]);
     c_im(input[8]) = st5;
     st5 = c_im(input[27]);
     st5 = st5 + c_im(input[11]);
     c_im(input[24]) = st2;
     st2 = st6 - st5;
     st6 = st6 + st5;
     st5 = st7 + st6;
     st7 = st7 - st6;
     st6 = st8 - st7;
     st8 = st8 + st7;
     st7 = st1 + st2;
     tmp686 = st4;
     st4 = st7 * K923879532[0];
     st1 = st1 - st2;
     st7 = st7 * K382683432[0];
     st2 = tmp683 + st5;
     tmp687 = st4;
     st4 = st1 * K382683432[0];
     tmp688 = st7;
     st7 = tmp652 - st2;
     st1 = st1 * K923879532[0];
     st2 = tmp652 + st2;
     st5 = tmp683 - st5;
     c_im(input[16]) = st7;
     st7 = tmp637 - st5;
     st5 = tmp637 + st5;
     c_im(input[0]) = st2;
     st2 = tmp682 + st8;
     st2 = st2 * K707106781[0];
     c_re(input[24]) = st7;
     st7 = tmp639 - st2;
     st2 = tmp639 + st2;
     st8 = tmp682 - st8;
     st8 = st8 * K707106781[0];
     c_re(input[8]) = st5;
     st5 = tmp651 - st8;
     st8 = tmp651 + st8;
     c_im(input[20]) = st7;
     st7 = tmp677 + st6;
     st7 = st7 * K707106781[0];
     c_im(input[4]) = st2;
     st2 = tmp642 - st7;
     st7 = tmp642 + st7;
     st6 = st6 - tmp677;
     st6 = st6 * K707106781[0];
     c_re(input[28]) = st5;
     st5 = tmp638 - st6;
     st6 = tmp638 + st6;
     st3 = st3 + st1;
     st1 = tmp681 - st3;
     st3 = tmp681 + st3;
     st4 = st4 - tmp686;
     c_re(input[12]) = st8;
     st8 = tmp680 + st4;
     st4 = st4 - tmp680;
     c_re(input[20]) = st2;
     st2 = tmp650 - st8;
     st8 = tmp650 + st8;
     c_re(input[4]) = st7;
     st7 = tmp649 - st4;
     st4 = tmp649 + st4;
     c_im(input[28]) = st5;
     st5 = tmp643 - st1;
     st1 = tmp643 + st1;
     c_im(input[12]) = st6;
     st6 = tmp644 - st3;
     st3 = tmp644 + st3;
     c_re(input[22]) = st2;
     st2 = tmp685 + tmp688;
     c_re(input[6]) = st8;
     st8 = tmp679 - st2;
     st2 = tmp679 + st2;
     c_im(input[30]) = st7;
     st7 = tmp687 - tmp684;
     c_im(input[14]) = st4;
     st4 = tmp678 + st7;
     st7 = st7 - tmp678;
     c_re(input[30]) = st5;
     st5 = tmp648 - st4;
     st4 = tmp648 + st4;
     c_re(input[14]) = st1;
     st1 = tmp647 - st7;
     st7 = tmp647 + st7;
     c_im(input[22]) = st6;
     st6 = tmp645 - st8;
     st8 = tmp645 + st8;
     c_im(input[6]) = st3;
     st3 = tmp646 - st2;
     st2 = tmp646 + st2;
     c_re(input[18]) = st5;
     st5 = c_re(input[31]);
     st5 = st5 - c_re(input[15]);
     c_re(input[2]) = st4;
     st4 = c_im(input[7]);
     st4 = st4 - c_im(input[23]);
     c_im(input[26]) = st1;
     st1 = st5 - st4;
     st5 = st5 + st4;
     c_im(input[10]) = st7;
     st7 = c_re(input[7]);
     st7 = st7 - c_re(input[23]);
     c_re(input[26]) = st6;
     st6 = c_im(input[31]);
     st6 = st6 - c_im(input[15]);
     c_re(input[10]) = st8;
     st8 = st7 + st6;
     st6 = st6 - st7;
     c_im(input[18]) = st3;
     st3 = c_im(input[3]);
     st3 = st3 - c_im(input[19]);
     c_im(input[2]) = st2;
     st2 = c_re(input[3]);
     st2 = st2 - c_re(input[19]);
     st4 = st3 - st2;
     st2 = st2 + st3;
     st7 = c_re(input[27]);
     st7 = st7 - c_re(input[11]);
     st3 = c_im(input[27]);
     st3 = st3 - c_im(input[11]);
     tmp689 = st5;
     st5 = st7 + st3;
     st7 = st7 - st3;
     st3 = st4 - st5;
     st3 = st3 * K707106781[0];
     st4 = st4 + st5;
     st4 = st4 * K707106781[0];
     st5 = st7 - st2;
     st5 = st5 * K707106781[0];
     st2 = st2 + st7;
     st2 = st2 * K707106781[0];
     st7 = st1 - st3;
     tmp690 = st2;
     st2 = st7 * K980785280[0];
     st1 = st1 + st3;
     st7 = st7 * K195090322[0];
     st3 = st6 - st4;
     tmp691 = st7;
     st7 = st3 * K555570233[0];
     st6 = st6 + st4;
     st3 = st3 * K831469612[0];
     st4 = st8 - st5;
     tmp692 = st6;
     st6 = st4 * K195090322[0];
     st8 = st8 + st5;
     st4 = st4 * K980785280[0];
     st5 = tmp689 - tmp690;
     tmp693 = st4;
     st4 = st5 * K831469612[0];
     tmp694 = st2;
     st2 = tmp689 + tmp690;
     st5 = st5 * K555570233[0];
     st4 = st4 + st7;
     st7 = st8 * K831469612[0];
     st5 = st5 - st3;
     st3 = st1 * K555570233[0];
     st6 = st6 + tmp694;
     st1 = st1 * K831469612[0];
     tmp695 = st6;
     st6 = tmp691 - tmp693;
     st8 = st8 * K555570233[0];
     st7 = st7 + st3;
     st3 = st2 * K195090322[0];
     st1 = st1 - st8;
     st8 = tmp692 * K980785280[0];
     st3 = st3 + st8;
     st2 = st2 * K980785280[0];
     st8 = c_re(input[9]);
     tmp696 = st3;
     st3 = tmp692 * K195090322[0];
     st8 = st8 - c_re(input[25]);
     st2 = st2 - st3;
     st3 = c_im(input[1]);
     st3 = st3 - c_im(input[17]);
     tmp697 = st2;
     st2 = st8 + st3;
     st3 = st3 - st8;
     st8 = c_re(input[1]);
     st8 = st8 - c_re(input[17]);
     tmp698 = st1;
     st1 = c_im(input[9]);
     st1 = st1 - c_im(input[25]);
     tmp699 = st7;
     st7 = st8 - st1;
     st8 = st8 + st1;
     st1 = c_re(input[29]);
     st1 = st1 - c_re(input[13]);
     tmp700 = st6;
     st6 = c_im(input[29]);
     st6 = st6 - c_im(input[13]);
     tmp701 = st5;
     st5 = st1 - st6;
     st1 = st1 + st6;
     st6 = c_re(input[5]);
     st6 = st6 - c_re(input[21]);
     tmp702 = st4;
     st4 = c_im(input[5]);
     st4 = st4 - c_im(input[21]);
     tmp703 = st8;
     st8 = st6 + st4;
     st4 = st4 - st6;
     st6 = st5 - st8;
     st6 = st6 * K707106781[0];
     st8 = st8 + st5;
     st8 = st8 * K707106781[0];
     st5 = st4 - st1;
     st5 = st5 * K707106781[0];
     st4 = st4 + st1;
     st4 = st4 * K707106781[0];
     st1 = st2 - st6;
     tmp704 = st8;
     st8 = st1 * K195090322[0];
     st2 = st2 + st6;
     st1 = st1 * K980785280[0];
     st6 = st3 - st4;
     tmp705 = st1;
     st1 = st6 * K555570233[0];
     st3 = st3 + st4;
     st6 = st6 * K831469612[0];
     st4 = st7 - st5;
     tmp706 = st8;
     st8 = st4 * K980785280[0];
     st7 = st7 + st5;
     st4 = st4 * K195090322[0];
     st5 = tmp703 - tmp704;
     tmp707 = st4;
     st4 = st5 * K831469612[0];
     tmp708 = st8;
     st8 = tmp703 + tmp704;
     st5 = st5 * K555570233[0];
     st1 = st1 - st4;
     st4 = st2 * K831469612[0];
     tmp709 = st4;
     st4 = st1 - tmp702;
     tmp710 = st4;
     st4 = st7 * K555570233[0];
     st1 = st1 + tmp702;
     st2 = st2 * K555570233[0];
     st5 = st5 + st6;
     st7 = st7 * K831469612[0];
     st6 = st5 + tmp701;
     tmp711 = st2;
     st2 = st3 * K980785280[0];
     st5 = tmp701 - st5;
     tmp712 = st2;
     st2 = st8 * K195090322[0];
     tmp713 = st2;
     st2 = tmp673 - st6;
     st8 = st8 * K980785280[0];
     st3 = st3 * K195090322[0];
     st6 = tmp673 + st6;
     c_re(input[21]) = st2;
     st2 = tmp672 - st5;
     st5 = tmp672 + st5;
     c_re(input[5]) = st6;
     st6 = tmp659 - tmp710;
     c_im(input[29]) = st2;
     st2 = tmp659 + tmp710;
     c_im(input[13]) = st5;
     st5 = tmp658 - st1;
     st1 = tmp658 + st1;
     c_re(input[29]) = st6;
     st6 = tmp706 - tmp708;
     c_re(input[13]) = st2;
     st2 = st6 - tmp695;
     st6 = st6 + tmp695;
     c_im(input[21]) = st5;
     st5 = tmp705 + tmp707;
     c_im(input[5]) = st1;
     st1 = st5 + tmp700;
     st5 = tmp700 - st5;
     tmp714 = st8;
     st8 = tmp661 - st1;
     st1 = tmp661 + st1;
     c_re(input[23]) = st8;
     st8 = tmp671 - st5;
     st5 = tmp671 + st5;
     c_re(input[7]) = st1;
     st1 = tmp660 - st2;
     st2 = tmp660 + st2;
     c_im(input[31]) = st8;
     st8 = tmp662 - st6;
     st6 = tmp662 + st6;
     st4 = tmp709 - st4;
     c_im(input[15]) = st5;
     st5 = st4 - tmp699;
     st4 = st4 + tmp699;
     st7 = tmp711 + st7;
     c_re(input[31]) = st1;
     st1 = st7 + tmp698;
     st7 = tmp698 - st7;
     c_re(input[15]) = st2;
     st2 = tmp670 - st1;
     st1 = tmp670 + st1;
     c_im(input[23]) = st8;
     st8 = tmp669 - st7;
     st7 = tmp669 + st7;
     c_im(input[7]) = st6;
     st6 = tmp663 - st5;
     st5 = tmp663 + st5;
     c_re(input[19]) = st2;
     st2 = tmp664 - st4;
     st4 = tmp664 + st4;
     c_re(input[3]) = st1;
     st1 = tmp712 - tmp713;
     c_im(input[27]) = st8;
     st8 = st1 - tmp696;
     st1 = st1 + tmp696;
     st3 = tmp714 + st3;
     c_im(input[11]) = st7;
     st7 = st3 + tmp697;
     st3 = tmp697 - st3;
     c_re(input[27]) = st6;
     st6 = tmp668 - st7;
     st7 = tmp668 + st7;
     c_re(input[11]) = st5;
     st5 = tmp667 - st3;
     st3 = tmp667 + st3;
     c_im(input[19]) = st2;
     st2 = tmp665 - st8;
     st8 = tmp665 + st8;
     c_im(input[3]) = st4;
     st4 = tmp666 - st1;
     st1 = tmp666 + st1;
     c_re(input[17]) = st6;
     c_re(input[1]) = st7;
     c_im(input[25]) = st5;
     c_im(input[9]) = st3;
     c_re(input[25]) = st2;
     c_re(input[9]) = st8;
     c_im(input[17]) = st4;
     c_im(input[1]) = st1;
}

static void PFFTW(64)(fftw_complex *input) 
{ 
     PFFTW(twiddle_4)(input, PFFTW(W_64), 16);
     PFFTW(16)(input + 16 * 0);
     PFFTW(16)(input + 16 * 1);
     PFFTW(16)(input + 16 * 2);
     PFFTW(16)(input + 16 * 3);
}

static void PFFTW(128)(fftw_complex *input) 
{ 
     PFFTW(twiddle_4)(input, PFFTW(W_128), 32);
     PFFTW(32)(input + 32 * 0);
     PFFTW(32)(input + 32 * 1);
     PFFTW(32)(input + 32 * 2);
     PFFTW(32)(input + 32 * 3);
}

static void PFFTW(256)(fftw_complex *input) 
{ 
     PFFTW(twiddle_4)(input, PFFTW(W_256), 64);
     PFFTW(64)(input + 64 * 0);
     PFFTW(64)(input + 64 * 1);
     PFFTW(64)(input + 64 * 2);
     PFFTW(64)(input + 64 * 3);
}

static void PFFTW(512)(fftw_complex *input) 
{ 
     PFFTW(twiddle_4)(input, PFFTW(W_512), 128);
     PFFTW(128)(input + 128 * 0);
     PFFTW(128)(input + 128 * 1);
     PFFTW(128)(input + 128 * 2);
     PFFTW(128)(input + 128 * 3);
}

static void PFFTWI(16) (fftw_complex * input) {
     fftw_real tmp333;
     fftw_real tmp332;
     fftw_real tmp331;
     fftw_real tmp330;
     fftw_real tmp329;
     fftw_real tmp328;
     fftw_real tmp327;
     fftw_real tmp326;
     fftw_real tmp325;
     fftw_real tmp324;
     fftw_real tmp323;
     fftw_real tmp322;
     fftw_real tmp321;
     fftw_real tmp320;
     fftw_real tmp319;
     fftw_real tmp318;
     fftw_real tmp317;
     fftw_real tmp316;
     fftw_real tmp315;
     fftw_real tmp314;
     fftw_real tmp313;
     fftw_real tmp312;
     fftw_real tmp311;
     fftw_real tmp310;
     fftw_real tmp309;
     fftw_real tmp308;
     fftw_real tmp307;
     fftw_real tmp306;
     fftw_real tmp305;
     fftw_real tmp304;
     fftw_real tmp303;
     fftw_real tmp302;
     fftw_real tmp301;
     fftw_real st1;
     fftw_real st2;
     fftw_real st3;
     fftw_real st4;
     fftw_real st5;
     fftw_real st6;
     fftw_real st7;
     fftw_real st8;
     st8 = c_re(input[4]);
     st8 = st8 - c_re(input[12]);
     st7 = c_im(input[0]);
     st7 = st7 - c_im(input[8]);
     st6 = c_re(input[0]);
     st6 = st6 - c_re(input[8]);
     st5 = st8 + st7;
     st7 = st7 - st8;
     st4 = c_im(input[4]);
     st4 = st4 - c_im(input[12]);
     st3 = c_re(input[0]);
     st3 = st3 + c_re(input[8]);
     st2 = st6 - st4;
     st6 = st6 + st4;
     st1 = c_re(input[4]);
     st1 = st1 + c_re(input[12]);
     st8 = c_im(input[0]);
     st8 = st8 + c_im(input[8]);
     st4 = st3 + st1;
     st3 = st3 - st1;
     st1 = c_im(input[4]);
     st1 = st1 + c_im(input[12]);
     tmp301 = st6;
     st6 = c_re(input[2]);
     st6 = st6 + c_re(input[10]);
     tmp302 = st7;
     st7 = st8 + st1;
     st8 = st8 - st1;
     st1 = c_re(input[6]);
     st1 = st1 + c_re(input[14]);
     tmp303 = st2;
     st2 = c_im(input[2]);
     st2 = st2 + c_im(input[10]);
     tmp304 = st5;
     st5 = st6 + st1;
     st6 = st6 - st1;
     st1 = c_im(input[6]);
     st1 = st1 + c_im(input[14]);
     tmp305 = st3;
     st3 = st4 + st5;
     st4 = st4 - st5;
     st5 = st2 + st1;
     st2 = st2 - st1;
     st1 = st6 + st8;
     tmp306 = st1;
     st1 = st7 - st5;
     st7 = st7 + st5;
     st5 = tmp305 - st2;
     st2 = tmp305 + st2;
     st8 = st8 - st6;
     st6 = c_re(input[6]);
     st6 = st6 - c_re(input[14]);
     tmp307 = st8;
     st8 = c_im(input[2]);
     st8 = st8 - c_im(input[10]);
     tmp308 = st2;
     st2 = c_re(input[2]);
     st2 = st2 - c_re(input[10]);
     tmp309 = st4;
     st4 = st6 + st8;
     st8 = st8 - st6;
     st6 = c_im(input[6]);
     st6 = st6 - c_im(input[14]);
     tmp310 = st5;
     st5 = c_re(input[1]);
     st5 = st5 - c_re(input[9]);
     tmp311 = st7;
     st7 = st2 - st6;
     st2 = st2 + st6;
     st6 = c_im(input[5]);
     tmp312 = st1;
     st1 = st4 + st7;
     st7 = st7 - st4;
     st4 = st2 - st8;
     st1 = st1 * K707106781[0];
     st8 = st8 + st2;
     st7 = st7 * K707106781[0];
     st6 = st6 - c_im(input[13]);
     st4 = st4 * K707106781[0];
     st2 = tmp304 - st1;
     st8 = st8 * K707106781[0];
     tmp313 = st3;
     st3 = st5 - st6;
     st5 = st5 + st6;
     st6 = tmp303 + st7;
     tmp314 = st6;
     st6 = st3 * K923879532[0];
     st7 = tmp303 - st7;
     st3 = st3 * K382683432[0];
     st1 = tmp304 + st1;
     tmp315 = st1;
     st1 = st5 * K382683432[0];
     tmp316 = st7;
     st7 = tmp302 - st4;
     st5 = st5 * K923879532[0];
     st4 = tmp302 + st4;
     tmp317 = st4;
     st4 = tmp301 - st8;
     st8 = tmp301 + st8;
     tmp318 = st8;
     st8 = c_re(input[5]);
     st8 = st8 - c_re(input[13]);
     tmp319 = st4;
     st4 = c_im(input[1]);
     st4 = st4 - c_im(input[9]);
     tmp320 = st7;
     st7 = c_re(input[3]);
     st7 = st7 - c_re(input[11]);
     tmp321 = st2;
     st2 = st8 + st4;
     st4 = st4 - st8;
     st8 = c_im(input[7]);
     tmp322 = st5;
     st5 = st2 * K382683432[0];
     st8 = st8 - c_im(input[15]);
     st2 = st2 * K923879532[0];
     st6 = st6 - st5;
     st5 = st4 * K923879532[0];
     st2 = st2 + st3;
     st4 = st4 * K382683432[0];
     st1 = st1 - st5;
     st3 = st7 - st8;
     st4 = st4 + tmp322;
     st7 = st7 + st8;
     st8 = st3 * K382683432[0];
     st5 = c_re(input[7]);
     st3 = st3 * K923879532[0];
     st5 = st5 - c_re(input[15]);
     tmp323 = st4;
     st4 = st7 * K923879532[0];
     tmp324 = st1;
     st1 = c_im(input[3]);
     st7 = st7 * K382683432[0];
     st1 = st1 - c_im(input[11]);
     tmp325 = st2;
     st2 = c_re(input[1]);
     st2 = st2 + c_re(input[9]);
     tmp326 = st6;
     st6 = st5 + st1;
     st1 = st1 - st5;
     st5 = c_re(input[5]);
     tmp327 = st7;
     st7 = st6 * K923879532[0];
     st5 = st5 + c_re(input[13]);
     st6 = st6 * K382683432[0];
     st8 = st8 - st7;
     st7 = st1 * K382683432[0];
     st6 = st6 + st3;
     st1 = st1 * K923879532[0];
     st7 = st7 - st4;
     st3 = st2 + st5;
     st1 = st1 + tmp327;
     st2 = st2 - st5;
     st4 = tmp326 - st8;
     st8 = tmp326 + st8;
     st5 = tmp325 - st6;
     tmp328 = st2;
     st2 = tmp321 - st4;
     st4 = tmp321 + st4;
     tmp329 = st3;
     st3 = tmp314 - st8;
     st8 = tmp314 + st8;
     tmp330 = st2;
     st2 = tmp316 - st5;
     st5 = tmp316 + st5;
     c_re(input[9]) = st3;
     c_re(input[1]) = st8;
     c_re(input[5]) = st2;
     c_re(input[13]) = st5;
     st6 = tmp325 + st6;
     st3 = tmp324 - st7;
     st8 = tmp323 - st1;
     st2 = tmp315 - st6;
     st6 = tmp315 + st6;
     st5 = tmp320 - st3;
     st3 = tmp320 + st3;
     tmp331 = st5;
     st5 = tmp317 - st8;
     st8 = tmp317 + st8;
     st7 = tmp324 + st7;
     st1 = tmp323 + st1;
     tmp332 = st3;
     st3 = c_im(input[1]);
     c_im(input[1]) = st6;
     st3 = st3 + c_im(input[9]);
     c_im(input[9]) = st2;
     st2 = tmp319 - st7;
     st7 = tmp319 + st7;
     st6 = tmp318 - st1;
     st1 = tmp318 + st1;
     tmp333 = st5;
     st5 = c_im(input[5]);
     c_im(input[5]) = st4;
     st5 = st5 + c_im(input[13]);
     c_im(input[13]) = tmp330;
     st4 = st3 - st5;
     st3 = st3 + st5;
     st5 = c_re(input[3]);
     c_re(input[3]) = st7;
     st5 = st5 + c_re(input[11]);
     c_re(input[11]) = st2;
     st2 = c_re(input[7]);
     c_re(input[7]) = st6;
     st2 = st2 + c_re(input[15]);
     c_re(input[15]) = st1;
     st7 = st5 + st2;
     st5 = st5 - st2;
     st6 = c_im(input[3]);
     c_im(input[3]) = st8;
     st6 = st6 + c_im(input[11]);
     c_im(input[11]) = tmp333;
     st8 = tmp329 + st7;
     st7 = tmp329 - st7;
     st1 = st5 + st4;
     st4 = st4 - st5;
     st2 = tmp313 - st8;
     st8 = tmp313 + st8;
     st5 = st7 + tmp312;
     st7 = tmp312 - st7;
     c_re(input[8]) = st2;
     c_re(input[0]) = st8;
     c_im(input[4]) = st5;
     c_im(input[12]) = st7;
     st2 = c_im(input[7]);
     c_im(input[7]) = tmp332;
     st2 = st2 + c_im(input[15]);
     c_im(input[15]) = tmp331;
     st8 = st6 - st2;
     st6 = st6 + st2;
     st5 = tmp328 - st8;
     st8 = tmp328 + st8;
     st7 = st3 - st6;
     st2 = st5 - st1;
     st1 = st1 + st5;
     st3 = st3 + st6;
     st2 = st2 * K707106781[0];
     st6 = st4 + st8;
     st1 = st1 * K707106781[0];
     st8 = st8 - st4;
     st6 = st6 * K707106781[0];
     st4 = tmp311 + st3;
     st8 = st8 * K707106781[0];
     st3 = tmp311 - st3;
     st5 = tmp310 - st2;
     c_im(input[0]) = st4;
     c_im(input[8]) = st3;
     c_re(input[10]) = st5;
     st2 = tmp310 + st2;
     st4 = tmp309 + st7;
     st7 = tmp309 - st7;
     st3 = tmp308 - st6;
     c_re(input[2]) = st2;
     c_re(input[12]) = st4;
     c_re(input[4]) = st7;
     c_re(input[6]) = st3;
     st6 = tmp308 + st6;
     st5 = tmp307 - st8;
     st8 = tmp307 + st8;
     st2 = tmp306 - st1;
     c_re(input[14]) = st6;
     c_im(input[14]) = st5;
     c_im(input[6]) = st8;
     c_im(input[10]) = st2;
     st1 = tmp306 + st1;
     c_im(input[2]) = st1;
}

static void PFFTWI(32) (fftw_complex * input) {
     fftw_real tmp714;
     fftw_real tmp713;
     fftw_real tmp712;
     fftw_real tmp711;
     fftw_real tmp710;
     fftw_real tmp709;
     fftw_real tmp708;
     fftw_real tmp707;
     fftw_real tmp706;
     fftw_real tmp705;
     fftw_real tmp704;
     fftw_real tmp703;
     fftw_real tmp702;
     fftw_real tmp701;
     fftw_real tmp700;
     fftw_real tmp699;
     fftw_real tmp698;
     fftw_real tmp697;
     fftw_real tmp696;
     fftw_real tmp695;
     fftw_real tmp694;
     fftw_real tmp693;
     fftw_real tmp692;
     fftw_real tmp691;
     fftw_real tmp690;
     fftw_real tmp689;
     fftw_real tmp688;
     fftw_real tmp687;
     fftw_real tmp686;
     fftw_real tmp685;
     fftw_real tmp684;
     fftw_real tmp683;
     fftw_real tmp682;
     fftw_real tmp681;
     fftw_real tmp680;
     fftw_real tmp679;
     fftw_real tmp678;
     fftw_real tmp677;
     fftw_real tmp676;
     fftw_real tmp675;
     fftw_real tmp674;
     fftw_real tmp673;
     fftw_real tmp672;
     fftw_real tmp671;
     fftw_real tmp670;
     fftw_real tmp669;
     fftw_real tmp668;
     fftw_real tmp667;
     fftw_real tmp666;
     fftw_real tmp665;
     fftw_real tmp664;
     fftw_real tmp663;
     fftw_real tmp662;
     fftw_real tmp661;
     fftw_real tmp660;
     fftw_real tmp659;
     fftw_real tmp658;
     fftw_real tmp657;
     fftw_real tmp656;
     fftw_real tmp655;
     fftw_real tmp654;
     fftw_real tmp653;
     fftw_real tmp652;
     fftw_real tmp651;
     fftw_real tmp650;
     fftw_real tmp649;
     fftw_real tmp648;
     fftw_real tmp647;
     fftw_real tmp646;
     fftw_real tmp645;
     fftw_real tmp644;
     fftw_real tmp643;
     fftw_real tmp642;
     fftw_real tmp641;
     fftw_real tmp640;
     fftw_real tmp639;
     fftw_real tmp638;
     fftw_real tmp637;
     fftw_real tmp636;
     fftw_real tmp635;
     fftw_real tmp634;
     fftw_real tmp633;
     fftw_real tmp632;
     fftw_real tmp631;
     fftw_real tmp630;
     fftw_real tmp629;
     fftw_real tmp628;
     fftw_real tmp627;
     fftw_real tmp626;
     fftw_real tmp625;
     fftw_real tmp624;
     fftw_real tmp623;
     fftw_real tmp622;
     fftw_real tmp621;
     fftw_real st1;
     fftw_real st2;
     fftw_real st3;
     fftw_real st4;
     fftw_real st5;
     fftw_real st6;
     fftw_real st7;
     fftw_real st8;
     st8 = c_re(input[8]);
     st8 = st8 - c_re(input[24]);
     st7 = c_im(input[0]);
     st7 = st7 - c_im(input[16]);
     st6 = st8 + st7;
     st7 = st7 - st8;
     st5 = c_re(input[0]);
     st5 = st5 + c_re(input[16]);
     st4 = c_re(input[8]);
     st4 = st4 + c_re(input[24]);
     st3 = st5 + st4;
     st5 = st5 - st4;
     st2 = c_im(input[0]);
     st2 = st2 + c_im(input[16]);
     st1 = c_im(input[8]);
     st1 = st1 + c_im(input[24]);
     st8 = st2 + st1;
     st2 = st2 - st1;
     st4 = c_re(input[4]);
     st4 = st4 + c_re(input[20]);
     st1 = c_re(input[28]);
     st1 = st1 + c_re(input[12]);
     tmp621 = st6;
     st6 = st4 + st1;
     st4 = st4 - st1;
     st1 = st3 + st6;
     st3 = st3 - st6;
     st6 = st2 - st4;
     st4 = st4 + st2;
     st2 = c_im(input[4]);
     st2 = st2 + c_im(input[20]);
     tmp622 = st4;
     st4 = c_im(input[28]);
     st4 = st4 + c_im(input[12]);
     tmp623 = st6;
     st6 = st2 + st4;
     st4 = st4 - st2;
     st2 = st8 + st6;
     st8 = st8 - st6;
     st6 = st5 - st4;
     st5 = st5 + st4;
     st4 = c_re(input[0]);
     st4 = st4 - c_re(input[16]);
     tmp624 = st5;
     st5 = c_im(input[8]);
     st5 = st5 - c_im(input[24]);
     tmp625 = st6;
     st6 = st4 - st5;
     st4 = st4 + st5;
     st5 = c_re(input[4]);
     st5 = st5 - c_re(input[20]);
     tmp626 = st3;
     st3 = c_im(input[4]);
     st3 = st3 - c_im(input[20]);
     tmp627 = st2;
     st2 = st5 + st3;
     st5 = st5 - st3;
     st3 = c_im(input[28]);
     st3 = st3 - c_im(input[12]);
     tmp628 = st8;
     st8 = c_re(input[28]);
     st8 = st8 - c_re(input[12]);
     tmp629 = st1;
     st1 = st3 - st8;
     st8 = st8 + st3;
     st3 = st2 + st1;
     st3 = st3 * K707106781[0];
     st1 = st1 - st2;
     st1 = st1 * K707106781[0];
     st2 = st5 + st8;
     st2 = st2 * K707106781[0];
     st5 = st5 - st8;
     st5 = st5 * K707106781[0];
     st8 = st7 - st5;
     tmp630 = st8;
     st8 = st4 - st1;
     tmp631 = st8;
     st8 = tmp621 - st3;
     tmp632 = st8;
     st8 = st6 - st2;
     st3 = tmp621 + st3;
     st6 = st6 + st2;
     st7 = st7 + st5;
     st4 = st4 + st1;
     st1 = c_re(input[2]);
     st1 = st1 + c_re(input[18]);
     st2 = c_re(input[10]);
     st2 = st2 + c_re(input[26]);
     st5 = st1 + st2;
     st1 = st1 - st2;
     st2 = c_im(input[2]);
     st2 = st2 + c_im(input[18]);
     tmp633 = st4;
     st4 = c_im(input[10]);
     st4 = st4 + c_im(input[26]);
     tmp634 = st7;
     st7 = st2 + st4;
     st2 = st2 - st4;
     st4 = st1 + st2;
     st1 = st1 - st2;
     st2 = c_re(input[30]);
     st2 = st2 + c_re(input[14]);
     tmp635 = st6;
     st6 = c_re(input[6]);
     st6 = st6 + c_re(input[22]);
     tmp636 = st3;
     st3 = st2 + st6;
     st2 = st2 - st6;
     st6 = st5 + st3;
     st5 = st5 - st3;
     st3 = tmp629 + st6;
     st6 = tmp629 - st6;
     tmp637 = st6;
     st6 = tmp628 - st5;
     st5 = st5 + tmp628;
     tmp638 = st5;
     st5 = c_im(input[30]);
     st5 = st5 + c_im(input[14]);
     tmp639 = st6;
     st6 = c_im(input[6]);
     st6 = st6 + c_im(input[22]);
     tmp640 = st3;
     st3 = st5 + st6;
     st5 = st5 - st6;
     st6 = st7 + st3;
     st3 = st3 - st7;
     st7 = st5 - st2;
     tmp641 = st8;
     st8 = st7 - st4;
     st8 = st8 * K707106781[0];
     st4 = st4 + st7;
     st4 = st4 * K707106781[0];
     st2 = st2 + st5;
     st5 = st1 - st2;
     st5 = st5 * K707106781[0];
     st1 = st1 + st2;
     st1 = st1 * K707106781[0];
     st7 = tmp627 - st6;
     st6 = tmp627 + st6;
     st2 = tmp626 + st3;
     st3 = tmp626 - st3;
     tmp642 = st3;
     st3 = tmp623 + st5;
     st5 = tmp623 - st5;
     tmp643 = st5;
     st5 = tmp625 - st8;
     st8 = tmp625 + st8;
     tmp644 = st8;
     st8 = tmp622 + st4;
     st4 = tmp622 - st4;
     tmp645 = st4;
     st4 = tmp624 - st1;
     st1 = tmp624 + st1;
     tmp646 = st1;
     st1 = c_re(input[6]);
     st1 = st1 - c_re(input[22]);
     tmp647 = st4;
     st4 = c_im(input[30]);
     st4 = st4 - c_im(input[14]);
     tmp648 = st8;
     st8 = st1 + st4;
     tmp649 = st5;
     st5 = st8 * K382683432[0];
     st4 = st4 - st1;
     st8 = st8 * K923879532[0];
     st1 = c_re(input[30]);
     tmp650 = st3;
     st3 = st4 * K923879532[0];
     st1 = st1 - c_re(input[14]);
     st4 = st4 * K382683432[0];
     tmp651 = st2;
     st2 = c_im(input[6]);
     st2 = st2 - c_im(input[22]);
     tmp652 = st6;
     st6 = st1 - st2;
     tmp653 = st7;
     st7 = st6 * K923879532[0];
     st1 = st1 + st2;
     st6 = st6 * K382683432[0];
     st5 = st5 + st7;
     st2 = st1 * K382683432[0];
     st8 = st8 - st6;
     st1 = st1 * K923879532[0];
     st3 = st3 + st2;
     st4 = st4 - st1;
     st7 = c_re(input[2]);
     st7 = st7 - c_re(input[18]);
     st6 = c_im(input[10]);
     st6 = st6 - c_im(input[26]);
     st2 = st7 - st6;
     st1 = st2 * K923879532[0];
     st7 = st7 + st6;
     st2 = st2 * K382683432[0];
     st6 = c_re(input[10]);
     tmp654 = st8;
     st8 = st7 * K382683432[0];
     st6 = st6 - c_re(input[26]);
     st7 = st7 * K923879532[0];
     tmp655 = st5;
     st5 = c_im(input[2]);
     st5 = st5 - c_im(input[18]);
     tmp656 = st4;
     st4 = st6 + st5;
     tmp657 = st3;
     st3 = st4 * K382683432[0];
     st5 = st5 - st6;
     st4 = st4 * K923879532[0];
     st1 = st1 - st3;
     st6 = st5 * K923879532[0];
     st4 = st4 + st2;
     st5 = st5 * K382683432[0];
     st8 = st8 - st6;
     st5 = st5 + st7;
     st2 = st8 - tmp657;
     st7 = tmp630 + st2;
     st2 = tmp630 - st2;
     st3 = tmp656 - st5;
     st6 = tmp631 - st3;
     st3 = tmp631 + st3;
     tmp658 = st3;
     st3 = st1 - tmp655;
     tmp659 = st2;
     st2 = tmp632 - st3;
     st3 = tmp632 + st3;
     tmp660 = st3;
     st3 = tmp654 - st4;
     tmp661 = st2;
     st2 = tmp641 + st3;
     st3 = tmp641 - st3;
     st4 = st4 + tmp654;
     tmp662 = st3;
     st3 = tmp636 - st4;
     st4 = tmp636 + st4;
     st1 = st1 + tmp655;
     tmp663 = st4;
     st4 = tmp635 + st1;
     st1 = tmp635 - st1;
     st5 = st5 + tmp656;
     tmp664 = st1;
     st1 = tmp634 + st5;
     st5 = tmp634 - st5;
     st8 = st8 + tmp657;
     tmp665 = st5;
     st5 = tmp633 - st8;
     st8 = tmp633 + st8;
     tmp666 = st8;
     st8 = c_re(input[1]);
     st8 = st8 + c_re(input[17]);
     tmp667 = st5;
     st5 = c_re(input[9]);
     st5 = st5 + c_re(input[25]);
     tmp668 = st1;
     st1 = st8 + st5;
     st8 = st8 - st5;
     st5 = c_im(input[1]);
     st5 = st5 + c_im(input[17]);
     tmp669 = st4;
     st4 = c_im(input[9]);
     st4 = st4 + c_im(input[25]);
     tmp670 = st3;
     st3 = st5 + st4;
     st5 = st5 - st4;
     st4 = c_re(input[5]);
     st4 = st4 + c_re(input[21]);
     tmp671 = st2;
     st2 = c_re(input[29]);
     st2 = st2 + c_re(input[13]);
     tmp672 = st6;
     st6 = st4 + st2;
     st4 = st4 - st2;
     st2 = st1 + st6;
     st1 = st1 - st6;
     st6 = st4 + st5;
     tmp673 = st7;
     st7 = st6 * K923879532[0];
     st5 = st5 - st4;
     st6 = st6 * K382683432[0];
     st4 = c_im(input[5]);
     tmp674 = st2;
     st2 = st5 * K382683432[0];
     st4 = st4 + c_im(input[21]);
     st5 = st5 * K923879532[0];
     tmp675 = st7;
     st7 = c_im(input[29]);
     st7 = st7 + c_im(input[13]);
     tmp676 = st5;
     st5 = st4 + st7;
     st7 = st7 - st4;
     st4 = st3 + st5;
     st3 = st3 - st5;
     st5 = st1 - st3;
     st1 = st1 + st3;
     st3 = st8 + st7;
     tmp677 = st1;
     st1 = st3 * K382683432[0];
     st8 = st8 - st7;
     st3 = st3 * K923879532[0];
     st3 = st3 - st6;
     st6 = st8 * K923879532[0];
     st2 = st2 + st6;
     st8 = st8 * K382683432[0];
     st8 = st8 - tmp676;
     st1 = tmp675 + st1;
     st7 = c_re(input[31]);
     st7 = st7 + c_re(input[15]);
     st6 = c_re(input[7]);
     st6 = st6 + c_re(input[23]);
     tmp678 = st1;
     st1 = st7 + st6;
     st7 = st7 - st6;
     st6 = c_im(input[31]);
     st6 = st6 + c_im(input[15]);
     tmp679 = st3;
     st3 = c_im(input[7]);
     st3 = st3 + c_im(input[23]);
     tmp680 = st2;
     st2 = st6 + st3;
     st6 = st6 - st3;
     st3 = c_re(input[3]);
     st3 = st3 + c_re(input[19]);
     tmp681 = st8;
     st8 = c_re(input[27]);
     st8 = st8 + c_re(input[11]);
     tmp682 = st5;
     st5 = st3 + st8;
     st3 = st3 - st8;
     st8 = st1 + st5;
     st1 = st1 - st5;
     st5 = st3 + st6;
     tmp683 = st4;
     st4 = st5 * K923879532[0];
     st6 = st6 - st3;
     st5 = st5 * K382683432[0];
     st3 = tmp674 + st8;
     tmp684 = st4;
     st4 = st6 * K382683432[0];
     tmp685 = st5;
     st5 = tmp640 - st3;
     st6 = st6 * K923879532[0];
     st3 = tmp640 + st3;
     st8 = tmp674 - st8;
     c_re(input[16]) = st5;
     st5 = st8 + tmp653;
     st8 = tmp653 - st8;
     c_re(input[0]) = st3;
     st3 = c_im(input[3]);
     st3 = st3 + c_im(input[19]);
     c_im(input[8]) = st5;
     st5 = c_im(input[27]);
     st5 = st5 + c_im(input[11]);
     c_im(input[24]) = st8;
     st8 = st3 + st5;
     st5 = st5 - st3;
     st3 = st2 + st8;
     st2 = st2 - st8;
     st8 = st1 + st2;
     st2 = st2 - st1;
     st1 = st7 + st5;
     tmp686 = st4;
     st4 = st1 * K382683432[0];
     st7 = st7 - st5;
     st1 = st1 * K923879532[0];
     st5 = tmp683 + st3;
     tmp687 = st4;
     st4 = st7 * K923879532[0];
     tmp688 = st1;
     st1 = tmp652 - st5;
     st7 = st7 * K382683432[0];
     st5 = tmp652 + st5;
     st3 = st3 - tmp683;
     c_im(input[16]) = st1;
     st1 = tmp637 - st3;
     st3 = tmp637 + st3;
     c_im(input[0]) = st5;
     st5 = tmp682 - st8;
     st5 = st5 * K707106781[0];
     c_re(input[24]) = st1;
     st1 = tmp639 - st5;
     st5 = tmp639 + st5;
     st8 = tmp682 + st8;
     st8 = st8 * K707106781[0];
     c_re(input[8]) = st3;
     st3 = tmp651 - st8;
     st8 = tmp651 + st8;
     c_im(input[28]) = st1;
     st1 = st2 - tmp677;
     st1 = st1 * K707106781[0];
     c_im(input[12]) = st5;
     st5 = tmp642 - st1;
     st1 = tmp642 + st1;
     st2 = tmp677 + st2;
     st2 = st2 * K707106781[0];
     c_re(input[20]) = st3;
     st3 = tmp638 - st2;
     st2 = tmp638 + st2;
     st6 = st6 + st7;
     st7 = tmp681 - st6;
     st6 = tmp681 + st6;
     st4 = tmp686 - st4;
     c_re(input[4]) = st8;
     st8 = tmp680 + st4;
     st4 = st4 - tmp680;
     c_re(input[28]) = st5;
     st5 = tmp650 - st8;
     st8 = tmp650 + st8;
     c_re(input[12]) = st1;
     st1 = tmp649 - st4;
     st4 = tmp649 + st4;
     c_im(input[20]) = st3;
     st3 = tmp643 - st7;
     st7 = tmp643 + st7;
     c_im(input[4]) = st2;
     st2 = tmp644 - st6;
     st6 = tmp644 + st6;
     c_im(input[22]) = st5;
     st5 = tmp685 + tmp688;
     c_im(input[6]) = st8;
     st8 = tmp679 - st5;
     st5 = tmp679 + st5;
     c_re(input[30]) = st1;
     st1 = tmp684 - tmp687;
     c_re(input[14]) = st4;
     st4 = tmp678 + st1;
     st1 = st1 - tmp678;
     c_im(input[30]) = st3;
     st3 = tmp648 - st4;
     st4 = tmp648 + st4;
     c_im(input[14]) = st7;
     st7 = tmp647 - st1;
     st1 = tmp647 + st1;
     c_re(input[22]) = st2;
     st2 = tmp645 - st8;
     st8 = tmp645 + st8;
     c_re(input[6]) = st6;
     st6 = tmp646 - st5;
     st5 = tmp646 + st5;
     c_im(input[18]) = st3;
     st3 = c_re(input[31]);
     st3 = st3 - c_re(input[15]);
     c_im(input[2]) = st4;
     st4 = c_im(input[7]);
     st4 = st4 - c_im(input[23]);
     c_re(input[26]) = st7;
     st7 = st3 - st4;
     st3 = st3 + st4;
     c_re(input[10]) = st1;
     st1 = c_re(input[7]);
     st1 = st1 - c_re(input[23]);
     c_im(input[26]) = st2;
     st2 = c_im(input[31]);
     st2 = st2 - c_im(input[15]);
     c_im(input[10]) = st8;
     st8 = st1 + st2;
     st2 = st2 - st1;
     c_re(input[18]) = st6;
     st6 = c_re(input[3]);
     st6 = st6 - c_re(input[19]);
     c_re(input[2]) = st5;
     st5 = c_im(input[3]);
     st5 = st5 - c_im(input[19]);
     st4 = st6 - st5;
     st6 = st6 + st5;
     st1 = c_re(input[27]);
     st1 = st1 - c_re(input[11]);
     st5 = c_im(input[27]);
     st5 = st5 - c_im(input[11]);
     tmp689 = st2;
     st2 = st1 + st5;
     st5 = st5 - st1;
     st1 = st4 + st2;
     st1 = st1 * K707106781[0];
     st4 = st4 - st2;
     st4 = st4 * K707106781[0];
     st2 = st6 + st5;
     st2 = st2 * K707106781[0];
     st5 = st5 - st6;
     st5 = st5 * K707106781[0];
     st6 = st7 - st1;
     tmp690 = st4;
     st4 = st6 * K831469612[0];
     st7 = st7 + st1;
     st6 = st6 * K555570233[0];
     st1 = st3 - st5;
     tmp691 = st6;
     st6 = st1 * K195090322[0];
     st3 = st3 + st5;
     st1 = st1 * K980785280[0];
     st5 = st8 - st2;
     tmp692 = st3;
     st3 = st5 * K555570233[0];
     st8 = st8 + st2;
     st5 = st5 * K831469612[0];
     st2 = tmp689 - tmp690;
     tmp693 = st5;
     st5 = st2 * K980785280[0];
     tmp694 = st4;
     st4 = tmp689 + tmp690;
     st2 = st2 * K195090322[0];
     st5 = st5 + st6;
     st6 = st8 * K980785280[0];
     st2 = st2 - st1;
     st1 = st7 * K195090322[0];
     st3 = st3 - tmp694;
     st7 = st7 * K980785280[0];
     tmp695 = st3;
     st3 = tmp691 + tmp693;
     st8 = st8 * K195090322[0];
     st6 = st6 - st1;
     st1 = st4 * K555570233[0];
     st7 = st7 + st8;
     st8 = tmp692 * K831469612[0];
     st1 = st1 + st8;
     st4 = st4 * K831469612[0];
     st8 = c_re(input[1]);
     tmp696 = st1;
     st1 = tmp692 * K555570233[0];
     st8 = st8 - c_re(input[17]);
     st4 = st4 - st1;
     st1 = c_im(input[9]);
     st1 = st1 - c_im(input[25]);
     tmp697 = st4;
     st4 = st8 - st1;
     st8 = st8 + st1;
     st1 = c_re(input[9]);
     st1 = st1 - c_re(input[25]);
     tmp698 = st7;
     st7 = c_im(input[1]);
     st7 = st7 - c_im(input[17]);
     tmp699 = st6;
     st6 = st1 + st7;
     st7 = st7 - st1;
     st1 = c_re(input[5]);
     st1 = st1 - c_re(input[21]);
     tmp700 = st3;
     st3 = c_im(input[5]);
     st3 = st3 - c_im(input[21]);
     tmp701 = st2;
     st2 = st1 - st3;
     st1 = st1 + st3;
     st3 = c_re(input[29]);
     st3 = st3 - c_re(input[13]);
     tmp702 = st5;
     st5 = c_im(input[29]);
     st5 = st5 - c_im(input[13]);
     tmp703 = st7;
     st7 = st3 + st5;
     st5 = st5 - st3;
     st3 = st2 + st7;
     st3 = st3 * K707106781[0];
     st2 = st2 - st7;
     st2 = st2 * K707106781[0];
     st7 = st1 + st5;
     st7 = st7 * K707106781[0];
     st5 = st5 - st1;
     st5 = st5 * K707106781[0];
     st1 = st4 - st3;
     tmp704 = st2;
     st2 = st1 * K831469612[0];
     st4 = st4 + st3;
     st1 = st1 * K555570233[0];
     st3 = st8 - st5;
     tmp705 = st1;
     st1 = st3 * K195090322[0];
     st8 = st8 + st5;
     st3 = st3 * K980785280[0];
     st5 = st6 - st7;
     tmp706 = st2;
     st2 = st5 * K555570233[0];
     st6 = st6 + st7;
     st5 = st5 * K831469612[0];
     st7 = tmp703 - tmp704;
     tmp707 = st5;
     st5 = st7 * K980785280[0];
     tmp708 = st2;
     st2 = tmp703 + tmp704;
     st7 = st7 * K195090322[0];
     st1 = st1 - st5;
     st5 = st4 * K195090322[0];
     tmp709 = st5;
     st5 = st1 - tmp702;
     tmp710 = st5;
     st5 = st6 * K980785280[0];
     st1 = st1 + tmp702;
     st4 = st4 * K980785280[0];
     st7 = st7 + st3;
     st6 = st6 * K195090322[0];
     st3 = st7 + tmp701;
     tmp711 = st4;
     st4 = st8 * K831469612[0];
     st7 = tmp701 - st7;
     tmp712 = st4;
     st4 = st2 * K555570233[0];
     tmp713 = st4;
     st4 = tmp673 - st3;
     st2 = st2 * K831469612[0];
     st8 = st8 * K555570233[0];
     st3 = tmp673 + st3;
     c_im(input[23]) = st4;
     st4 = tmp672 - st7;
     st7 = tmp672 + st7;
     c_im(input[7]) = st3;
     st3 = tmp659 - tmp710;
     c_re(input[31]) = st4;
     st4 = tmp659 + tmp710;
     c_re(input[15]) = st7;
     st7 = tmp658 - st1;
     st1 = tmp658 + st1;
     c_im(input[31]) = st3;
     st3 = tmp706 + tmp708;
     c_im(input[15]) = st4;
     st4 = st3 + tmp695;
     st3 = tmp695 - st3;
     c_re(input[23]) = st7;
     st7 = tmp705 - tmp707;
     c_re(input[7]) = st1;
     st1 = st7 - tmp700;
     st7 = st7 + tmp700;
     tmp714 = st2;
     st2 = tmp661 - st1;
     st1 = tmp661 + st1;
     c_im(input[29]) = st2;
     st2 = tmp671 - st7;
     st7 = tmp671 + st7;
     c_im(input[13]) = st1;
     st1 = tmp660 - st4;
     st4 = tmp660 + st4;
     c_re(input[21]) = st2;
     st2 = tmp662 - st3;
     st3 = tmp662 + st3;
     st5 = tmp709 + st5;
     c_re(input[5]) = st7;
     st7 = st5 + tmp699;
     st5 = tmp699 - st5;
     st6 = tmp711 - st6;
     c_im(input[21]) = st1;
     st1 = st6 - tmp698;
     st6 = st6 + tmp698;
     c_im(input[5]) = st4;
     st4 = tmp670 - st1;
     st1 = tmp670 + st1;
     c_re(input[29]) = st2;
     st2 = tmp669 - st6;
     st6 = tmp669 + st6;
     c_re(input[13]) = st3;
     st3 = tmp663 - st7;
     st7 = tmp663 + st7;
     c_im(input[25]) = st4;
     st4 = tmp664 - st5;
     st5 = tmp664 + st5;
     c_im(input[9]) = st1;
     st1 = tmp712 - tmp713;
     c_re(input[17]) = st2;
     st2 = st1 - tmp696;
     st1 = st1 + tmp696;
     st8 = tmp714 + st8;
     c_re(input[1]) = st6;
     st6 = st8 + tmp697;
     st8 = tmp697 - st8;
     c_im(input[17]) = st3;
     st3 = tmp668 - st6;
     st6 = tmp668 + st6;
     c_im(input[1]) = st7;
     st7 = tmp667 - st8;
     st8 = tmp667 + st8;
     c_re(input[25]) = st4;
     st4 = tmp665 - st2;
     st2 = tmp665 + st2;
     c_re(input[9]) = st5;
     st5 = tmp666 - st1;
     st1 = tmp666 + st1;
     c_im(input[19]) = st3;
     c_im(input[3]) = st6;
     c_re(input[27]) = st7;
     c_re(input[11]) = st8;
     c_im(input[27]) = st4;
     c_im(input[11]) = st2;
     c_re(input[19]) = st5;
     c_re(input[3]) = st1;
}

static void PFFTWI(64)(fftw_complex *input) 
{
     PFFTWI(16)(input + 16 * 0);
     PFFTWI(16)(input + 16 * 1);
     PFFTWI(16)(input + 16 * 2);
     PFFTWI(16)(input + 16 * 3);
     PFFTWI(twiddle_4)(input, PFFTW(W_64), 16);
}

static void PFFTWI(128)(fftw_complex *input) 
{
     PFFTWI(32)(input + 32 * 0);
     PFFTWI(32)(input + 32 * 1);
     PFFTWI(32)(input + 32 * 2);
     PFFTWI(32)(input + 32 * 3);
     PFFTWI(twiddle_4)(input, PFFTW(W_128), 32);
}

static void PFFTWI(256)(fftw_complex *input) 
{
     PFFTWI(64)(input + 64 * 0);
     PFFTWI(64)(input + 64 * 1);
     PFFTWI(64)(input + 64 * 2);
     PFFTWI(64)(input + 64 * 3);
     PFFTWI(twiddle_4)(input, PFFTW(W_256), 64);
}

static void PFFTWI(512)(fftw_complex *input) 
{
     PFFTWI(128)(input + 128 * 0);
     PFFTWI(128)(input + 128 * 1);
     PFFTWI(128)(input + 128 * 2);
     PFFTWI(128)(input + 128 * 3);
     PFFTWI(twiddle_4)(input, PFFTW(W_512), 128);
}

static void PFFTW(twiddle_4) (fftw_complex * A, const fftw_complex * W, uint16_t iostride) {
     uint16_t i;
     fftw_complex *inout;
     inout = A;
     {
	  fftw_real st1;
	  fftw_real st2;
	  fftw_real st3;
	  fftw_real st4;
	  fftw_real st5;
	  fftw_real st6;
	  fftw_real st7;
	  fftw_real st8;
	  st8 = c_re(inout[0]);
	  st8 = st8 + c_re(inout[2 * iostride]);
	  st7 = c_re(inout[iostride]);
	  st7 = st7 + c_re(inout[3 * iostride]);
	  st6 = st8 - st7;
	  st8 = st8 + st7;
	  st5 = c_im(inout[0]);
	  st5 = st5 + c_im(inout[2 * iostride]);
	  st4 = c_im(inout[iostride]);
	  st4 = st4 + c_im(inout[3 * iostride]);
	  st3 = st5 - st4;
	  st5 = st5 + st4;
	  st2 = c_im(inout[0]);
	  st2 = st2 - c_im(inout[2 * iostride]);
	  st1 = c_re(inout[iostride]);
	  st1 = st1 - c_re(inout[3 * iostride]);
	  st7 = st2 - st1;
	  st1 = st1 + st2;
	  st4 = c_re(inout[0]);
	  st4 = st4 - c_re(inout[2 * iostride]);
	  c_re(inout[2 * iostride]) = st6;
	  st6 = c_im(inout[iostride]);
	  st6 = st6 - c_im(inout[3 * iostride]);
	  c_re(inout[0]) = st8;
	  st8 = st4 - st6;
	  st4 = st4 + st6;
	  c_im(inout[0]) = st5;
	  c_im(inout[2 * iostride]) = st3;
	  c_im(inout[iostride]) = st7;
	  c_im(inout[3 * iostride]) = st1;
	  c_re(inout[3 * iostride]) = st8;
	  c_re(inout[iostride]) = st4;
     }
     inout = inout + 1;
     i = iostride - 1;
     do {
	  {
	       fftw_real st1;
	       fftw_real st2;
	       fftw_real st3;
	       fftw_real st4;
	       fftw_real st5;
	       fftw_real st6;
	       fftw_real st7;
	       fftw_real st8;
	       st8 = c_re(inout[0]);
	       st8 = st8 + c_re(inout[2 * iostride]);
	       st7 = c_re(inout[iostride]);
	       st7 = st7 + c_re(inout[3 * iostride]);
	       st6 = st8 - st7;
	       st5 = st6 * c_im(W[1]);
	       st8 = st8 + st7;
	       st6 = st6 * c_re(W[1]);
	       st4 = c_im(inout[0]);
	       st4 = st4 + c_im(inout[2 * iostride]);
	       st3 = c_im(inout[iostride]);
	       st3 = st3 + c_im(inout[3 * iostride]);
	       st2 = st4 - st3;
	       st1 = st2 * c_im(W[1]);
	       st4 = st4 + st3;
	       st2 = st2 * c_re(W[1]);
	       st2 = st2 - st5;
	       st6 = st6 + st1;
	       st7 = c_re(inout[0]);
	       st7 = st7 - c_re(inout[2 * iostride]);
	       st5 = c_im(inout[iostride]);
	       st5 = st5 - c_im(inout[3 * iostride]);
	       c_re(inout[0]) = st8;
	       st8 = st7 - st5;
	       st3 = st8 * c_re(W[0]);
	       st7 = st7 + st5;
	       st8 = st8 * c_im(W[0]);
	       st1 = c_re(inout[iostride]);
	       c_re(inout[2 * iostride]) = st6;
	       st6 = st7 * c_im(W[0]);
	       st1 = st1 - c_re(inout[3 * iostride]);
	       st7 = st7 * c_re(W[0]);
	       st5 = c_im(inout[0]);
	       st5 = st5 - c_im(inout[2 * iostride]);
	       c_im(inout[0]) = st4;
	       st4 = st1 + st5;
	       c_im(inout[2 * iostride]) = st2;
	       st2 = st4 * c_im(W[0]);
	       st5 = st5 - st1;
	       st4 = st4 * c_re(W[0]);
	       st3 = st3 - st2;
	       st1 = st5 * c_re(W[0]);
	       st5 = st5 * c_im(W[0]);
	       st4 = st4 + st8;
	       st5 = st5 + st7;
	       st1 = st1 - st6;
	       c_re(inout[3 * iostride]) = st3;
	       c_im(inout[3 * iostride]) = st4;
	       c_re(inout[iostride]) = st5;
	       c_im(inout[iostride]) = st1;
	  }
	  i = i - 1, inout = inout + 1, W = W + 2;
     } while (i > 0);
}

static void PFFTWI(twiddle_4) (fftw_complex * A, const fftw_complex * W, uint16_t iostride) {
     uint16_t i;
     fftw_complex *inout;
     inout = A;
     {
	  fftw_real st1;
	  fftw_real st2;
	  fftw_real st3;
	  fftw_real st4;
	  fftw_real st5;
	  fftw_real st6;
	  fftw_real st7;
	  fftw_real st8;
	  st8 = c_re(inout[0]);
	  st8 = st8 + c_re(inout[2 * iostride]);
	  st7 = c_re(inout[iostride]);
	  st7 = st7 + c_re(inout[3 * iostride]);
	  st6 = st8 - st7;
	  st8 = st8 + st7;
	  st5 = c_im(inout[0]);
	  st5 = st5 + c_im(inout[2 * iostride]);
	  st4 = c_im(inout[iostride]);
	  st4 = st4 + c_im(inout[3 * iostride]);
	  st3 = st5 - st4;
	  st5 = st5 + st4;
	  st2 = c_re(inout[iostride]);
	  st2 = st2 - c_re(inout[3 * iostride]);
	  st1 = c_im(inout[0]);
	  st1 = st1 - c_im(inout[2 * iostride]);
	  st7 = st2 + st1;
	  st1 = st1 - st2;
	  st4 = c_re(inout[0]);
	  st4 = st4 - c_re(inout[2 * iostride]);
	  c_re(inout[2 * iostride]) = st6;
	  st6 = c_im(inout[iostride]);
	  st6 = st6 - c_im(inout[3 * iostride]);
	  c_re(inout[0]) = st8;
	  st8 = st4 - st6;
	  st4 = st4 + st6;
	  c_im(inout[0]) = st5;
	  c_im(inout[2 * iostride]) = st3;
	  c_im(inout[iostride]) = st7;
	  c_im(inout[3 * iostride]) = st1;
	  c_re(inout[iostride]) = st8;
	  c_re(inout[3 * iostride]) = st4;
     }
     inout = inout + 1;
     i = iostride - 1;
     do {
	  {
	       fftw_real st1;
	       fftw_real st2;
	       fftw_real st3;
	       fftw_real st4;
	       fftw_real st5;
	       fftw_real st6;
	       fftw_real st7;
	       fftw_real st8;
	       st8 = c_re(inout[2 * iostride]);
	       st8 = st8 * c_re(W[1]);
	       st7 = c_im(inout[2 * iostride]);
	       st7 = st7 * c_im(W[1]);
	       st8 = st8 - st7;
	       st6 = st8 + c_re(inout[0]);
	       st8 = c_re(inout[0]) - st8;
	       st5 = c_re(inout[2 * iostride]);
	       st5 = st5 * c_im(W[1]);
	       st4 = c_im(inout[2 * iostride]);
	       st4 = st4 * c_re(W[1]);
	       st5 = st5 + st4;
	       st3 = st5 + c_im(inout[0]);
	       st5 = c_im(inout[0]) - st5;
	       st2 = c_re(inout[iostride]);
	       st2 = st2 * c_re(W[0]);
	       st1 = c_im(inout[iostride]);
	       st1 = st1 * c_im(W[0]);
	       st2 = st2 - st1;
	       st7 = c_re(inout[3 * iostride]);
	       st7 = st7 * c_re(W[0]);
	       st4 = c_im(inout[3 * iostride]);
	       st4 = st4 * c_im(W[0]);
	       st7 = st7 + st4;
	       st1 = st2 + st7;
	       st2 = st2 - st7;
	       st4 = st6 - st1;
	       st6 = st6 + st1;
	       st7 = st2 + st5;
	       st5 = st5 - st2;
	       st1 = c_re(inout[iostride]);
	       st1 = st1 * c_im(W[0]);
	       st2 = c_im(inout[iostride]);
	       st2 = st2 * c_re(W[0]);
	       st1 = st1 + st2;
	       c_re(inout[2 * iostride]) = st4;
	       st4 = c_im(inout[3 * iostride]);
	       st4 = st4 * c_re(W[0]);
	       c_re(inout[0]) = st6;
	       st6 = c_re(inout[3 * iostride]);
	       st6 = st6 * c_im(W[0]);
	       st4 = st4 - st6;
	       c_im(inout[iostride]) = st7;
	       st7 = st1 - st4;
	       st1 = st1 + st4;
	       c_im(inout[3 * iostride]) = st5;
	       st5 = st8 - st7;
	       st8 = st8 + st7;
	       st2 = st1 + st3;
	       st3 = st3 - st1;
	       c_re(inout[iostride]) = st5;
	       c_re(inout[3 * iostride]) = st8;
	       c_im(inout[0]) = st2;
	       c_im(inout[2 * iostride]) = st3;
	  }
	  i = i - 1, inout = inout + 1, W = W + 2;
     } while (i > 0);
}

uint16_t PFFTW(permutation_16) (uint16_t i)
{
    return i;
}

uint16_t PFFTW(permutation_32) (uint16_t i)
{
     return i;
}

static uint16_t PFFTW(permutation_64)(uint16_t i)
{
    uint16_t i1 = i % 4;
    uint16_t i2 = i / 4;
    if (i1 <= (4 / 2))
       return (i1 * 16 + PFFTW(permutation_16)(i2));
    else
       return (i1 * 16 + PFFTW(permutation_16)((i2 + 1) % 16));
}

static uint16_t PFFTW(permutation_128)(uint16_t i)
{
    uint16_t i1 = i % 4;
    uint16_t i2 = i / 4;
    if (i1 <= (4 / 2))
       return (i1 * 32 + PFFTW(permutation_32)(i2));
    else
       return (i1 * 32 + PFFTW(permutation_32)((i2 + 1) % 32));
}

static uint16_t PFFTW(permutation_256)(uint16_t i)
{
    uint16_t i1 = i % 4;
    uint16_t i2 = i / 4;
    if (i1 <= (4 / 2))
       return (i1 * 64 + PFFTW(permutation_64)(i2));
    else
       return (i1 * 64 + PFFTW(permutation_64)((i2 + 1) % 64));
}

static uint16_t PFFTW(permutation_512)(uint16_t i)
{
    uint16_t i1 = i % 4;
    uint16_t i2 = i / 4;
    if (i1 <= (4 / 2))
       return (i1 * 128 + PFFTW(permutation_128)(i2));
    else
       return (i1 * 128 + PFFTW(permutation_128)((i2 + 1) % 128));
}

static void make_fft_order(uint16_t *unscrambled, uint16_t len)
{
    uint16_t i;

    switch (len)
    {
    case 64:
        for (i = 0; i < len; i++)
            unscrambled[i] = PFFTW(permutation_64)(i);
        break;
    case 256:
        for (i = 0; i < len; i++)
            unscrambled[i] = PFFTW(permutation_256)(i);
        break;
    case 512:
        for (i = 0; i < len; i++)
            unscrambled[i] = PFFTW(permutation_512)(i);
        break;
    }
}
