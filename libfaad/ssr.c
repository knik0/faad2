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
** $Id: ssr.c,v 1.1 2002/11/28 18:48:30 menno Exp $
**/

#include "common.h"
#include "structs.h"

#ifdef SSR_DEC

#include "syntax.h"
#include "filtbank.h"
#include "ssr.h"
#include "ssr_fb.h"

void ssr_decode(ssr_info *ssr, fb_info *fb, uint8_t window_sequence,
                uint8_t window_shape, uint8_t window_shape_prev,
                real_t *freq_in, real_t *time_out, uint16_t frame_len)
{
    uint8_t band;
    uint16_t ssr_frame_len = frame_len/SSR_BANDS;
    real_t time_tmp[2048];

    for (band = 0; band < SSR_BANDS; band++)
    {
        int16_t j;

        /* uneven bands have inverted frequency scale */
        if (band == 1 || band == 3)
        {
            for (j = 0; j < ssr_frame_len/2; j++)
            {
                real_t tmp;
                tmp = freq_in[j + ssr_frame_len*band];
                freq_in[j + ssr_frame_len*band] =
                    freq_in[ssr_frame_len - j - 1 + ssr_frame_len*band];
                freq_in[ssr_frame_len - j - 1 + ssr_frame_len*band] = tmp;
            }
        }

        /* non-overlapping inverse filterbank for SSR */
        ssr_ifilter_bank(fb, window_sequence, window_shape, window_shape_prev,
            freq_in + band*ssr_frame_len, time_tmp + band*ssr_frame_len,
            ssr_frame_len);

#if 0
        /* gain control */
        ssr_gain_control(ssr, time_tmp, window_sequence, ssr_frame_len);
#endif
    }

#if 0
    /* inverse pqf to bring subbands together again */
    ssr_ipqf();
#endif
}

static void ssr_gain_control(ssr_info *ssr, real_t *data, uint8_t window_sequence,
                             uint16_t frame_len)
{
    if (window_sequence != EIGHT_SHORT_SEQUENCE)
    {
//        ssr_gc_function();
    } else {
    }
}

#if 0
static void ssr_gc_function(ssr_info *ssr, real_t *prevFMD,
                            real_t *gc_function, uint16_t frame_len)
{
    int32_t aloc[10];
    real_t alev[10];

    switch (window_sequence)
    {
    case ONLY_LONG_SEQUENCE:
        len_area1 = frame_len/SSR_BANDS;
        len_area2 = 0;
        break;
    case LONG_START_SEQUENCE:
        len_area1 = (frame_len/SSR_BANDS)*7/32;
        len_area2 = (frame_len/SSR_BANDS)/16;
        break;
    case EIGHT_SHORT_SEQUENCE:
        len_area1 = (frame_len/8)/SSR_BANDS;
        len_area2 = 0;
        break;
    case LONG_STOP_SEQUENCE:
        len_area1 = (frame_len/SSR_BANDS);
        len_area2 = 0;
        break;
    }

    /* decode bitstream information */

    /* build array M */
}
#endif

#endif
