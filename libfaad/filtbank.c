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
** $Id: filtbank.c,v 1.1 2002/01/14 19:15:55 menno Exp $
**/

#include <stdlib.h>
#ifdef __ICL
#include <mathf.h>
#else
#include <math.h>
#endif
#include <assert.h>
#include "filtbank.h"
#include "syntax.h"
#include "kbd_win.h"
#include "mdct.h"

#ifndef PI
#define PI 3.14159265359f
#endif

#ifdef LINUX
#define INLINE inline
#else
#ifdef WIN32
#define INLINE __inline
#else
#define INLINE
#endif
#endif

float *long_window[2];
float *short_window[2];

void filter_bank_init(fb_info *fb)
{
    int i;

    make_fft_order(fb->unscrambled64, fb->unscrambled512);

    fb->sin_long  = malloc(BLOCK_LEN_LONG*sizeof(float));
    fb->sin_short = malloc(BLOCK_LEN_SHORT*sizeof(float));

    long_window[0]  = fb->sin_long;
    long_window[1]  = kbd_long;
    short_window[0] = fb->sin_short;
    short_window[1] = kbd_short;

    /* calculate the sine windows */
    for (i = 0; i < BLOCK_LEN_LONG; i++)
#ifdef __ICL
        fb->sin_long[i] = sinf(PI / (2.0f * BLOCK_LEN_LONG) * (i + 0.5));
#else
        fb->sin_long[i] = (float)sin(PI / (2.0 * BLOCK_LEN_LONG) * (i + 0.5));
#endif
    for (i = 0; i < BLOCK_LEN_SHORT; i++)
#ifdef __ICL
        fb->sin_short[i] = sinf(PI / (2.0f * BLOCK_LEN_SHORT) * (i + 0.5));
#else
        fb->sin_short[i] = (float)sin(PI / (2.0 * BLOCK_LEN_SHORT) * (i + 0.5));
#endif
}

void filter_bank_end(fb_info *fb)
{
    if (fb->sin_long) free(fb->sin_long);
    if (fb->sin_short) free(fb->sin_short);
}

static INLINE void vcopy(float *src, float *dest, int vlen)
{
    int i;

    assert(vlen % 16 == 0);

    for (i = vlen/16-1; i >= 0; --i)
    {
        *dest++ = *src++; *dest++ = *src++; *dest++ = *src++; *dest++ = *src++;
        *dest++ = *src++; *dest++ = *src++; *dest++ = *src++; *dest++ = *src++;
        *dest++ = *src++; *dest++ = *src++; *dest++ = *src++; *dest++ = *src++;
        *dest++ = *src++; *dest++ = *src++; *dest++ = *src++; *dest++ = *src++;
    }
}

static INLINE void vzero(float *dest, int vlen)
{
    int i;

    assert(vlen % 16 == 0);

    for (i = vlen/16-1; i >= 0; --i)
    {
        *dest-- = 0; *dest-- = 0; *dest-- = 0; *dest-- = 0;
        *dest-- = 0; *dest-- = 0; *dest-- = 0; *dest-- = 0;
        *dest-- = 0; *dest-- = 0; *dest-- = 0; *dest-- = 0;
        *dest-- = 0; *dest-- = 0; *dest-- = 0; *dest-- = 0;
    }
}

static INLINE void vmult1(float *src1, float *src2, float *dest, int vlen)
{
    int i;

    assert(vlen % 16 == 0);

    for (i = vlen/16-1; i >= 0 ; --i)
    {
        *dest++ = *src1++ * *src2++; *dest++ = *src1++ * *src2++;
        *dest++ = *src1++ * *src2++; *dest++ = *src1++ * *src2++;
        *dest++ = *src1++ * *src2++; *dest++ = *src1++ * *src2++;
        *dest++ = *src1++ * *src2++; *dest++ = *src1++ * *src2++;
        *dest++ = *src1++ * *src2++; *dest++ = *src1++ * *src2++;
        *dest++ = *src1++ * *src2++; *dest++ = *src1++ * *src2++;
        *dest++ = *src1++ * *src2++; *dest++ = *src1++ * *src2++;
        *dest++ = *src1++ * *src2++; *dest++ = *src1++ * *src2++;
    }
}

static INLINE void vmult2(float *src1, float *src2, float *dest, int vlen)
{
    int i;

    assert(vlen % 16 == 0);

    for (i = vlen/16-1; i >= 0 ; --i)
    {
        *dest++ = *src1++ * *src2--; *dest++ = *src1++ * *src2--;
        *dest++ = *src1++ * *src2--; *dest++ = *src1++ * *src2--;
        *dest++ = *src1++ * *src2--; *dest++ = *src1++ * *src2--;
        *dest++ = *src1++ * *src2--; *dest++ = *src1++ * *src2--;
        *dest++ = *src1++ * *src2--; *dest++ = *src1++ * *src2--;
        *dest++ = *src1++ * *src2--; *dest++ = *src1++ * *src2--;
        *dest++ = *src1++ * *src2--; *dest++ = *src1++ * *src2--;
        *dest++ = *src1++ * *src2--; *dest++ = *src1++ * *src2--;
    }
}

static INLINE void vadd(float *src1, float *src2, float *dest, int vlen)
{
    int i;

    assert(vlen % 16 == 0);

    for (i = vlen/16-1; i >= 0; --i)
    {
        *dest++ = *src1++ + *src2++; *dest++ = *src1++ + *src2++;
        *dest++ = *src1++ + *src2++; *dest++ = *src1++ + *src2++;
        *dest++ = *src1++ + *src2++; *dest++ = *src1++ + *src2++;
        *dest++ = *src1++ + *src2++; *dest++ = *src1++ + *src2++;
        *dest++ = *src1++ + *src2++; *dest++ = *src1++ + *src2++;
        *dest++ = *src1++ + *src2++; *dest++ = *src1++ + *src2++;
        *dest++ = *src1++ + *src2++; *dest++ = *src1++ + *src2++;
        *dest++ = *src1++ + *src2++; *dest++ = *src1++ + *src2++;
    }
}

static INLINE void imdct(fb_info *fb, float *in_data, float *out_data, int len)
{
    switch (len)
    {
    case 2048:
        IMDCT_long(in_data, out_data, fb->unscrambled512);
        return;
    case 256:
        IMDCT_short(in_data, out_data, fb->unscrambled64);
        return;
    }
}

static INLINE void mdct(fb_info *fb, float *in_data, float *out_data, int len)
{
    switch (len)
    {
    case 2048:
        MDCT_long(in_data, out_data, fb->unscrambled512);
        return;
    case 256:
        MDCT_short(in_data, out_data, fb->unscrambled64);
        return;
    }
}

void ifilter_bank(fb_info *fb, int window_sequence, int window_shape,
                  int window_shape_prev, float *freq_in, float *time_buff,
                  float *time_out)
{
    float *o_buf, transf_buf[2*BLOCK_LEN_LONG];

    float *window_long;
    float *window_long_prev;
    float *window_short;
    float *window_short_prev;
    float *window_short_prev_ptr;

    float *fp;
    int win;
    int nlong = 1024;
    int nshort = 128;

    int nflat_ls = (nlong-nshort)/2;

    window_long       =  long_window[window_shape];
    window_long_prev  =  long_window[window_shape_prev];
    window_short      = short_window[window_shape];
    window_short_prev = short_window[window_shape_prev];

    /* pointer to previous window function */
    window_short_prev_ptr = window_short_prev;

    vcopy(time_buff, time_out, nlong);
    o_buf = time_out;

    switch (window_sequence)
    {
    case ONLY_LONG_SEQUENCE:
        /* inverse transform */
        imdct(fb, freq_in, transf_buf, 2*nlong);

        /* window function (previous) on first half of the new data */
        vmult1(transf_buf, window_long_prev, transf_buf, nlong);

        /* overlap and add second half of the old data with first half
           of the new data */
        vadd(transf_buf, o_buf, o_buf, nlong);

        /* reversed window function on second half of the new data */
        vmult2(transf_buf+nlong, window_long+nlong-1, o_buf+nlong, nlong);
        break;

    case LONG_START_SEQUENCE:
        /* inverse transform */
        imdct(fb, freq_in, transf_buf, 2*nlong);

        /* window function (previous) on first half of the new data */
        vmult1(transf_buf, window_long_prev, transf_buf, nlong);

        /* overlap and add second half of the old data with first half
           of the new data */
        vadd(transf_buf, o_buf, o_buf, nlong);

        /* copy data from nlong upto (3*nlong-nshort)/4; (window function = 1.0) */
        vcopy(transf_buf+nlong, o_buf+nlong, nflat_ls);

        /* reversed window function on part of second half of the new data */
        vmult2(transf_buf+nlong+nflat_ls, window_short+nshort-1,
            o_buf+nlong+nflat_ls, nshort);

        /* zero rest of the data; (window function = 0.0) */
        vzero(o_buf+2*nlong-1, nflat_ls);
        break;

    case EIGHT_SHORT_SEQUENCE:
        fp = o_buf + nflat_ls;
        for (win = 8-1; win >= 0; --win)
        {
            /* inverse transform */
            imdct(fb, freq_in, transf_buf, 2*nshort);

            /* window function (previous) on first half of the new data */
            vmult1(transf_buf, window_short_prev_ptr, transf_buf, nshort);

            /* overlap and add second half of the old data with first half
               of the new data */
            vadd(transf_buf, fp, fp, nshort);

            /* reversed window function on second half of the new data */
            vmult2(transf_buf+nshort, window_short+nshort-1, fp+nshort, nshort);

            /* shift to next short block */
            freq_in += nshort;
            fp      += nshort;
            window_short_prev_ptr = window_short;
        }
        vzero(o_buf+2*nlong-1, nflat_ls);
        break;

    case LONG_STOP_SEQUENCE:
        /* inverse transform */
        imdct(fb, freq_in, transf_buf, 2*nlong);

        /* zero first part of first half of the data (window function = 0.0) */
        vzero(transf_buf+nflat_ls-1, nflat_ls);

        /* window function (previous) on part of the first half of
           the new data */
        vmult1(transf_buf+nflat_ls, window_short_prev_ptr,
            transf_buf+nflat_ls, nshort);

        /* third part of the stop sequence window is window function = 1,
           so no need to actually apply that */

        /* overlap and add second half of the old data with first half
           of the new data */
        vadd(transf_buf+nflat_ls, o_buf+nflat_ls, o_buf+nflat_ls, nshort);

        /* copy last part of first half of the data (window function = 1.0) */
        vcopy(transf_buf+nflat_ls+nshort, o_buf+nflat_ls+nshort, nflat_ls);

        /* reversed window function on second half of the new data */
        vmult2(transf_buf+nlong, window_long+nlong-1, o_buf+nlong, nlong);
        break;
    }

    /* save second half of data */
    vcopy(o_buf+nlong, time_buff, nlong);
}

/* only works for LTP -> no overlapping */
void filter_bank_ltp(fb_info *fb, int window_sequence, int window_shape,
                     int window_shape_prev, float *in_data, float *out_mdct)
{
    int win;
    float windowed_buf[2*1024];
    float *p_o_buf;

    float *window_long;
    float *window_long_prev;
    float *window_short;
    float *window_short_prev;
    float *window_short_prev_ptr;

    int nlong = 1024;
    int nshort = 128;
    int nflat_ls = (nlong-nshort)/2;


    window_long       =  long_window[window_shape];
    window_long_prev  =  long_window[window_shape_prev];
    window_short      = short_window[window_shape];
    window_short_prev = short_window[window_shape_prev];
    window_short_prev_ptr = window_short_prev;
  
    p_o_buf = in_data;

    switch(window_sequence)
    {
    case ONLY_LONG_SEQUENCE:
        vmult1(p_o_buf, window_long_prev, windowed_buf, nlong);
        vmult2(p_o_buf+nlong, window_long+nlong-1, windowed_buf+nlong, nlong);
        mdct(fb, windowed_buf, out_mdct, 2*nlong);
        break;

    case LONG_START_SEQUENCE:
        vmult1(p_o_buf, window_long_prev, windowed_buf, nlong);
        vcopy(p_o_buf+nlong, windowed_buf+nlong, nflat_ls);
        vmult2(p_o_buf+nlong+nflat_ls, window_short+nshort-1, windowed_buf+nlong+nflat_ls, nshort);
        vzero(windowed_buf+2*nlong-1, nflat_ls);
        mdct(fb, windowed_buf, out_mdct, 2*nlong);
        break;

    case EIGHT_SHORT_SEQUENCE:
        for (win = 8-1; win >= 0; --win)
        {
            vmult1(p_o_buf, window_short_prev_ptr, windowed_buf, nshort);
            vmult2(p_o_buf+nshort, window_short+nshort-1, windowed_buf+nshort, nshort);
            mdct(fb, windowed_buf, out_mdct, 2*nshort);

            out_mdct += nshort;
            p_o_buf  += 2*nshort;
            window_short_prev_ptr = window_short; 
        }
        break;

    case LONG_STOP_SEQUENCE:
        vzero(windowed_buf+nflat_ls-1, nflat_ls);
        vmult1(p_o_buf+nflat_ls, window_short_prev_ptr, windowed_buf+nflat_ls, nshort);
        vcopy(p_o_buf+nflat_ls+nshort, windowed_buf+nflat_ls+nshort, nflat_ls);
        vmult2(p_o_buf+nlong, window_long+nlong-1, windowed_buf+nlong, nlong);
        mdct(fb, windowed_buf, out_mdct, 2*nlong);
        break;
    }
}
