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
** $Id: filtbank.c,v 1.6 2002/03/27 19:09:29 menno Exp $
**/

#include "common.h"

#include <stdlib.h>
#include <assert.h>
#include "filtbank.h"
#include "syntax.h"
#include "kbd_win.h"
#include "mdct.h"


void filter_bank_init(fb_info *fb)
{
    uint16_t i;

    /* normal */
    mdct_init(&(fb->mdct256), 256);
    mdct_init(&(fb->mdct2048), 2048);

    fb->long_window[0]  = malloc(BLOCK_LEN_LONG*sizeof(real_t));
    fb->short_window[0] = malloc(BLOCK_LEN_SHORT*sizeof(real_t));
    fb->long_window[1]  = kbd_long;
    fb->short_window[1] = kbd_short;

    /* calculate the sine windows */
    for (i = 0; i < BLOCK_LEN_LONG; i++)
        fb->long_window[0][i] = (real_t)sin(M_PI / (2.0 * BLOCK_LEN_LONG) * (i + 0.5));
    for (i = 0; i < BLOCK_LEN_SHORT; i++)
        fb->short_window[0][i] = (real_t)sin(M_PI / (2.0 * BLOCK_LEN_SHORT) * (i + 0.5));

#ifdef LD_DEC
    /* LD */
    mdct_init(&(fb->mdct1024), 1024);

    fb->ld_window[0] = malloc(BLOCK_LEN_LD*sizeof(real_t));
    fb->ld_window[1] = malloc(BLOCK_LEN_LD*sizeof(real_t));

    /* calculate the sine windows */
    for (i = 0; i < BLOCK_LEN_LD; i++)
        fb->ld_window[0][i] = (real_t)sin(M_PI / (2.0 * BLOCK_LEN_LD) * (i + 0.5));

    /* low overlap window */
    for (i = 0; i < 3*(BLOCK_LEN_LD>>3); i++)
        fb->ld_window[1][i] = 0.0;
    for (; i < 5*(BLOCK_LEN_LD>>3); i++)
        fb->ld_window[1][i] = (real_t)sin((i-3*(BLOCK_LEN_LD>>3)+0.5) * M_PI / (BLOCK_LEN_LD>>1));
    for (; i < BLOCK_LEN_LD; i++)
        fb->ld_window[1][i] = 1.0;
#endif
}

void filter_bank_end(fb_info *fb)
{
    mdct_end(&(fb->mdct256));
    mdct_end(&(fb->mdct2048));

    if (fb->long_window[0]) free(fb->long_window[0]);
    if (fb->short_window[0]) free(fb->short_window[0]);

#ifdef LD_DEC
    mdct_end(&(fb->mdct1024));

    if (fb->ld_window[0]) free(fb->ld_window[0]);
    if (fb->ld_window[1]) free(fb->ld_window[1]);
#endif
}

static INLINE void vcopy(real_t *src, real_t *dest, uint16_t vlen)
{
    int16_t i;

    assert(vlen % 16 == 0);

    for (i = vlen/16-1; i >= 0; --i)
    {
        *dest++ = *src++; *dest++ = *src++; *dest++ = *src++; *dest++ = *src++;
        *dest++ = *src++; *dest++ = *src++; *dest++ = *src++; *dest++ = *src++;
        *dest++ = *src++; *dest++ = *src++; *dest++ = *src++; *dest++ = *src++;
        *dest++ = *src++; *dest++ = *src++; *dest++ = *src++; *dest++ = *src++;
    }
}

static INLINE void vzero(real_t *dest, uint16_t vlen)
{
    int16_t i;

    assert(vlen % 16 == 0);

    for (i = vlen/16-1; i >= 0; --i)
    {
        *dest-- = 0; *dest-- = 0; *dest-- = 0; *dest-- = 0;
        *dest-- = 0; *dest-- = 0; *dest-- = 0; *dest-- = 0;
        *dest-- = 0; *dest-- = 0; *dest-- = 0; *dest-- = 0;
        *dest-- = 0; *dest-- = 0; *dest-- = 0; *dest-- = 0;
    }
}

static INLINE void vmult1(real_t *src1, real_t *src2, real_t *dest, uint16_t vlen)
{
    int16_t i;

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

static INLINE void vmult2(real_t *src1, real_t *src2, real_t *dest, uint16_t vlen)
{
    int16_t i;

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

static INLINE void vadd(real_t *src1, real_t *src2, real_t *dest, uint16_t vlen)
{
    int16_t i;

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

static INLINE void imdct(fb_info *fb, real_t *in_data, real_t *out_data, uint16_t len)
{
    switch (len)
    {
    case 2048:
        IMDCT_2048(&(fb->mdct2048), in_data, out_data);
        return;
    case 256:
        IMDCT_256(&(fb->mdct256), in_data, out_data);
        return;
#ifdef LD_DEC
    case 1024:
        IMDCT_1024(&(fb->mdct1024), in_data, out_data);
        return;
#endif
    }
}

static INLINE void mdct(fb_info *fb, real_t *in_data, real_t *out_data, uint16_t len)
{
    switch (len)
    {
    case 2048:
        MDCT_2048(&(fb->mdct2048), in_data, out_data);
        return;
    case 256:
        MDCT_256(&(fb->mdct256), in_data, out_data);
        return;
#ifdef LD_DEC
    case 1024:
        MDCT_1024(&(fb->mdct1024), in_data, out_data);
        return;
#endif
    }
}

void ifilter_bank(fb_info *fb, uint8_t window_sequence, uint8_t window_shape,
                  uint8_t window_shape_prev, real_t *freq_in, real_t *time_buff,
                  real_t *time_out, uint8_t object_type)
{
    real_t *o_buf, *transf_buf;
    real_t *obuf_temp;

    real_t *window_long;
    real_t *window_long_prev;
    real_t *window_short;
    real_t *window_short_prev;
    real_t *window_short_prev_ptr;

    real_t *fp;
    int8_t win;
    uint16_t nlong =
#ifdef LD_DEC
        (object_type == LD) ? 512 :
#endif
        1024;
    uint16_t nshort = 128;

    uint16_t nflat_ls = (nlong-nshort)/2;

    transf_buf = malloc(2*nlong*sizeof(real_t));

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
        obuf_temp = malloc(2*nlong*sizeof(real_t));
        vzero(obuf_temp+2*nlong-1, 2*nlong);

		fp = obuf_temp;
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

        vadd(o_buf + 448, obuf_temp, o_buf + 448, nlong - 448);
        vcopy(obuf_temp, o_buf + 448, nlong*2-nflat_ls);
        
        vzero(o_buf+2*nlong-1, nflat_ls);

        free(obuf_temp);
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
        vadd(transf_buf, o_buf, o_buf, nlong);

        /* reversed window function on second half of the new data */      
        vmult2(transf_buf+nlong, window_long+nlong-1, o_buf+nlong, nlong);
		break;
    }

    /* save second half of data */
    vcopy(o_buf+nlong, time_buff, nlong);

    free(transf_buf);
}

/* only works for LTP -> no overlapping */
void filter_bank_ltp(fb_info *fb, uint8_t window_sequence, uint8_t window_shape,
                     uint8_t window_shape_prev, real_t *in_data, real_t *out_mdct,
                     uint8_t object_type)
{
    int8_t win;
    real_t *windowed_buf;
    real_t *p_o_buf;

    real_t *window_long;
    real_t *window_long_prev;
    real_t *window_short;
    real_t *window_short_prev;
    real_t *window_short_prev_ptr;

    uint16_t nlong =
#ifdef LD_DEC
        (object_type == LD) ? 512 :
#endif
        1024;
    uint16_t nshort = 128;
    uint16_t nflat_ls = (nlong-nshort)/2;

    windowed_buf = malloc(nlong*2*sizeof(real_t));

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

    free(windowed_buf);
}
