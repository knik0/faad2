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
** $Id: filtbank.c,v 1.16 2002/08/27 10:24:55 menno Exp $
**/

#include "common.h"

#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include "filtbank.h"
#include "syntax.h"
#include "kbd_win.h"
#include "mdct.h"


fb_info *filter_bank_init(uint16_t frame_len)
{
    uint16_t i;
    uint16_t nshort = frame_len/8;
#ifdef LD_DEC
    uint16_t frame_len_ld = frame_len/2;
#endif

    fb_info *fb = (fb_info*)malloc(sizeof(fb_info));
    memset(fb, 0, sizeof(fb_info));

    /* normal */
    fb->mdct256 = faad_mdct_init(2*nshort);
    fb->mdct2048 = faad_mdct_init(2*frame_len);

    fb->long_window[0]  = (real_t*)malloc(frame_len*sizeof(real_t));
    fb->short_window[0] = (real_t*)malloc(nshort*sizeof(real_t));
#ifndef FIXED_POINT
    fb->long_window[1]  = kbd_long;
    fb->short_window[1] = kbd_short;
#else
    fb->long_window[1]  = (real_t*)malloc(frame_len*sizeof(real_t));
    fb->short_window[1] = (real_t*)malloc(nshort*sizeof(real_t));

    for (i = 0; i < frame_len; i++)
        fb->long_window[1][i] = COEF_CONST(kbd_long[i]);
    for (i = 0; i < nshort; i++)
        fb->short_window[1][i] = COEF_CONST(kbd_short[i]);
#endif

    /* calculate the sine windows */
    for (i = 0; i < frame_len; i++)
        fb->long_window[0][i] = COEF_CONST(sin(M_PI / (2.0 * (float32_t)frame_len) * ((float32_t)i + 0.5)));
    for (i = 0; i < nshort; i++)
        fb->short_window[0][i] = COEF_CONST(sin(M_PI / (2.0 * (float32_t)nshort) * ((float32_t)i + 0.5)));

#ifdef LD_DEC
    /* LD */
    fb->mdct1024 = faad_mdct_init(frame_len_ld);

    fb->ld_window[0] = (real_t*)malloc(frame_len_ld*sizeof(real_t));
    fb->ld_window[1] = (real_t*)malloc(frame_len_ld*sizeof(real_t));

    /* calculate the sine windows */
    for (i = 0; i < frame_len_ld; i++)
        fb->ld_window[0][i] = COEF_CONST(sin(M_PI / (2.0 * frame_len_ld) * (i + 0.5)));

    /* low overlap window */
    for (i = 0; i < 3*(frame_len_ld>>3); i++)
        fb->ld_window[1][i] = 0.0;
    for (; i < 5*(frame_len_ld>>3); i++)
        fb->ld_window[1][i] = COEF_CONST(sin((i-3*(frame_len_ld>>3)+0.5) * M_PI / (real_t)(frame_len_ld>>1)));
    for (; i < frame_len_ld; i++)
        fb->ld_window[1][i] = 1.0;
#endif

    return fb;
}

void filter_bank_end(fb_info *fb)
{
    faad_mdct_end(fb->mdct256);
    faad_mdct_end(fb->mdct2048);

    if (fb->long_window[0]) free(fb->long_window[0]);
    if (fb->short_window[0]) free(fb->short_window[0]);
#ifdef FIXED_POINT
    if (fb->long_window[1]) free(fb->long_window[1]);
    if (fb->short_window[1]) free(fb->short_window[1]);
#endif

#ifdef LD_DEC
    faad_mdct_end(fb->mdct1024);

    if (fb->ld_window[0]) free(fb->ld_window[0]);
    if (fb->ld_window[1]) free(fb->ld_window[1]);
#endif

    if (fb) free(fb);
}

static INLINE void imdct(fb_info *fb, real_t *in_data, real_t *out_data, uint16_t len)
{
    mdct_info *mdct;

    switch (len)
    {
    case 2048:
    case 1920:
        mdct = fb->mdct2048;
        break;
    case 256:
    case 240:
        mdct = fb->mdct256;
        break;
#ifdef LD_DEC
    case 1024:
    case 960:
        mdct = fb->mdct1024;
        break;
#endif
    }

    faad_imdct(mdct, in_data, out_data);
}

#ifdef LTP_DEC
static INLINE void mdct(fb_info *fb, real_t *in_data, real_t *out_data, uint16_t len)
{
    mdct_info *mdct;

    switch (len)
    {
    case 2048:
    case 1920:
        mdct = fb->mdct2048;
        break;
    case 256:
    case 240:
        mdct = fb->mdct256;
        break;
#ifdef LD_DEC
    case 1024:
    case 960:
        mdct = fb->mdct1024;
        break;
#endif
    }

    faad_mdct(mdct, in_data, out_data);
}
#endif

void ifilter_bank(fb_info *fb, uint8_t window_sequence, uint8_t window_shape,
                  uint8_t window_shape_prev, real_t *freq_in,
                  real_t *time_out, uint8_t object_type, uint16_t frame_len)
{
    int16_t i;
    real_t *transf_buf;

    real_t *window_long;
    real_t *window_long_prev;
    real_t *window_short;
    real_t *window_short_prev;

    uint16_t nlong = frame_len;
    uint16_t nshort = frame_len/8;
    uint16_t trans = nshort/2;

    uint16_t nflat_ls = (nlong-nshort)/2;

    transf_buf = (real_t*)malloc(2*nlong*sizeof(real_t));

#ifdef LD_DEC
    if (object_type == LD)
    {
        window_long       = fb->ld_window[window_shape];
        window_long_prev  = fb->ld_window[window_shape_prev];
    } else {
#endif
        window_long       = fb->long_window[window_shape];
        window_long_prev  = fb->long_window[window_shape_prev];
        window_short      = fb->short_window[window_shape];
        window_short_prev = fb->short_window[window_shape_prev];
#ifdef LD_DEC
    }
#endif

    switch (window_sequence)
    {
    case ONLY_LONG_SEQUENCE:
        imdct(fb, freq_in, transf_buf, 2*nlong);
        for (i = nlong-1; i >= 0; i--)
        {
            time_out[i] = time_out[nlong+i] + MUL_R_C(transf_buf[i],window_long_prev[i]);
            time_out[nlong+i] = MUL_R_C(transf_buf[nlong+i],window_long[nlong-1-i]);
        }
        break;

    case LONG_START_SEQUENCE:
        imdct(fb, freq_in, transf_buf, 2*nlong);
        for (i = 0; i < 0; i++)
            time_out[i] = time_out[nlong+i] + MUL_R_C(transf_buf[i],window_long_prev[i]);
        for (i = 0; i < nflat_ls; i++)
            time_out[nlong+i] = transf_buf[nlong+i];
        for (i = 0; i < nshort; i++)
            time_out[nlong+nflat_ls+i] = MUL_R_C(transf_buf[nlong+nflat_ls+i],window_short[nshort-i-1]);
        for (i = 0; i < nflat_ls; i++)
            time_out[nlong+nflat_ls+nshort+i] = REAL_CONST(0.0);
        break;

    case EIGHT_SHORT_SEQUENCE:
        imdct(fb, freq_in+0*nshort, transf_buf+2*nshort*0, 2*nshort);
        imdct(fb, freq_in+1*nshort, transf_buf+2*nshort*1, 2*nshort);
        imdct(fb, freq_in+2*nshort, transf_buf+2*nshort*2, 2*nshort);
        imdct(fb, freq_in+3*nshort, transf_buf+2*nshort*3, 2*nshort);
        imdct(fb, freq_in+4*nshort, transf_buf+2*nshort*4, 2*nshort);
        imdct(fb, freq_in+5*nshort, transf_buf+2*nshort*5, 2*nshort);
        imdct(fb, freq_in+6*nshort, transf_buf+2*nshort*6, 2*nshort);
        imdct(fb, freq_in+7*nshort, transf_buf+2*nshort*7, 2*nshort);
        for (i = 0; i < nflat_ls; i++)
            time_out[i] = time_out[nlong+i];
        for(i = nshort-1; i >= 0; i--)
        {
            time_out[nflat_ls+         i] = time_out[nlong+nflat_ls+         i] + MUL_R_C(transf_buf[nshort*0+i],window_short_prev[i]);
            time_out[nflat_ls+1*nshort+i] = time_out[nlong+nflat_ls+nshort*1+i] + MUL_R_C(transf_buf[nshort*1+i],window_short[nshort-1-i]) + MUL_R_C(transf_buf[nshort*2+i],window_short[i]);
            time_out[nflat_ls+2*nshort+i] = time_out[nlong+nflat_ls+nshort*2+i] + MUL_R_C(transf_buf[nshort*3+i],window_short[nshort-1-i]) + MUL_R_C(transf_buf[nshort*4+i],window_short[i]);
            time_out[nflat_ls+3*nshort+i] = time_out[nlong+nflat_ls+nshort*3+i] + MUL_R_C(transf_buf[nshort*5+i],window_short[nshort-1-i]) + MUL_R_C(transf_buf[nshort*6+i],window_short[i]);
            if (i < trans)
                time_out[nflat_ls+4*nshort+i] = time_out[nlong+nflat_ls+nshort*4+i] + MUL_R_C(transf_buf[nshort*7+i],window_short[nshort-1-i]) + MUL_R_C(transf_buf[nshort*8+i],window_short[i]);
            else
                time_out[nflat_ls+4*nshort+i] = MUL_R_C(transf_buf[nshort*7+i],window_short[nshort-1-i]) + MUL_R_C(transf_buf[nshort*8+i],window_short[i]);
            time_out[nflat_ls+5*nshort+i] = MUL_R_C(transf_buf[nshort*9+i],window_short[nshort-1-i]) + MUL_R_C(transf_buf[nshort*10+i],window_short[i]);
            time_out[nflat_ls+6*nshort+i] = MUL_R_C(transf_buf[nshort*11+i],window_short[nshort-1-i]) + MUL_R_C(transf_buf[nshort*12+i],window_short[i]);
            time_out[nflat_ls+7*nshort+i] = MUL_R_C(transf_buf[nshort*13+i],window_short[nshort-1-i]) + MUL_R_C(transf_buf[nshort*14+i],window_short[i]);
            time_out[nflat_ls+8*nshort+i] = MUL_R_C(transf_buf[nshort*15+i],window_short[nshort-1-i]);
        }
        for (i = 0; i < nflat_ls; i++)
            time_out[nlong+nflat_ls+nshort+i] = REAL_CONST(0.0);
        break;

    case LONG_STOP_SEQUENCE:
        imdct(fb, freq_in, transf_buf, 2*nlong);
        for (i = 0; i < nflat_ls; i++)
            time_out[i] = time_out[nlong+i];
        for (i = 0; i < nshort; i++)
            time_out[nflat_ls+i] = time_out[nlong+nflat_ls+i] + MUL_R_C(transf_buf[nflat_ls+i],window_short_prev[i]);
        for (i = 0; i < nflat_ls; i++)
            time_out[nflat_ls+nshort+i] = time_out[nlong+nflat_ls+nshort+i] + transf_buf[nflat_ls+nshort+i];
        for (i = 0; i < nlong; i++)
            time_out[nlong+i] = MUL_R_C(transf_buf[nlong+i],window_long[nlong-1-i]);
		break;
    }

    free(transf_buf);
}

#ifdef LTP_DEC
/* only works for LTP -> no overlapping no short blocks */
void filter_bank_ltp(fb_info *fb, uint8_t window_sequence, uint8_t window_shape,
                     uint8_t window_shape_prev, real_t *in_data, real_t *out_mdct,
                     uint8_t object_type, uint16_t frame_len)
{
    int16_t i;
    real_t *windowed_buf;

    real_t *window_long;
    real_t *window_long_prev;
    real_t *window_short;
    real_t *window_short_prev;

    uint16_t nlong = frame_len;
    uint16_t nshort = frame_len/8;
    uint16_t nflat_ls = (nlong-nshort)/2;

    assert(window_sequence != EIGHT_SHORT_SEQUENCE);

    windowed_buf = (real_t*)malloc(nlong*2*sizeof(real_t));

#ifdef LD_DEC
    if (object_type == LD)
    {
        window_long       = fb->ld_window[window_shape];
        window_long_prev  = fb->ld_window[window_shape_prev];
    } else {
#endif
        window_long       = fb->long_window[window_shape];
        window_long_prev  = fb->long_window[window_shape_prev];
        window_short      = fb->short_window[window_shape];
        window_short_prev = fb->short_window[window_shape_prev];
#ifdef LD_DEC
    }
#endif

    switch(window_sequence)
    {
    case ONLY_LONG_SEQUENCE:
        for (i = nlong-1; i >= 0; i--)
        {
            windowed_buf[i] = MUL_R_C(in_data[i], window_long_prev[i]);
            windowed_buf[i+nlong] = MUL_R_C(in_data[i+nlong], window_long[nlong-1-i]);
        }
        mdct(fb, windowed_buf, out_mdct, 2*nlong);
        break;

    case LONG_START_SEQUENCE:
        for (i = 0; i < nlong; i++)
            windowed_buf[i] = MUL_R_C(in_data[i], window_long_prev[i]);
        for (i = 0; i < nflat_ls; i++)
            windowed_buf[i+nlong] = in_data[i+nlong];
        for (i = 0; i < nshort; i++)
            windowed_buf[i+nlong+nflat_ls] = MUL_R_C(in_data[i+nlong+nflat_ls], window_short[nshort-1-i]);
        for (i = 0; i < nflat_ls; i++)
            windowed_buf[i+nlong+nflat_ls+nshort] = 0;
        mdct(fb, windowed_buf, out_mdct, 2*nlong);
        break;

    case LONG_STOP_SEQUENCE:
        for (i = 0; i < nflat_ls; i++)
            windowed_buf[i] = 0;
        for (i = 0; i < nshort; i++)
            windowed_buf[i+nflat_ls] = MUL_R_C(in_data[i+nflat_ls], window_short_prev[i]);
        for (i = 0; i < nflat_ls; i++)
            windowed_buf[i+nflat_ls+nshort] = in_data[i+nflat_ls+nshort];
        for (i = 0; i < nlong; i++)
            windowed_buf[i+nlong] = MUL_R_C(in_data[i+nlong], window_long[nlong-1-i]);
        mdct(fb, windowed_buf, out_mdct, 2*nlong);
        break;
    }

    free(windowed_buf);
}
#endif
