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
** $Id: is.c,v 1.8 2002/08/26 18:41:47 menno Exp $
**/

#include "common.h"

#include "syntax.h"
#include "is.h"

#ifdef FIXED_POINT

static real_t pow05_table[4] = {
    COEF_CONST(1.0),
    COEF_CONST(0.8408964), /* pow(2,-.25) */
    COEF_CONST(0.7071068), /* pow(2,-.5) */
    COEF_CONST(0.5946036)  /* pow(2,-.75) */
};

void is_decode(ic_stream *ics, ic_stream *icsr, real_t *l_spec, real_t *r_spec,
               uint16_t frame_len)
{
    uint8_t g, sfb, b;
    uint16_t i, k;

    uint16_t nshort = frame_len/8;
    uint8_t group = 0;

    for (g = 0; g < icsr->num_window_groups; g++)
    {
        // Do intensity stereo decoding
        for (b = 0; b < icsr->window_group_length[g]; b++)
        {
            for (sfb = 0; sfb < icsr->max_sfb; sfb++)
            {
                if (is_intensity(icsr, g, sfb))
                {
                    real_t frac;

                    // For scalefactor bands coded in intensity stereo the
                    // corresponding predictors in the right channel are
                    // switched to "off".
                    ics->pred.prediction_used[sfb] = 0;
                    icsr->pred.prediction_used[sfb] = 0;

                    frac = pow05_table[icsr->scale_factors[g][sfb] & 3];

                    if (is_intensity(icsr, g, sfb) != invert_intensity(ics, g, sfb))
                        frac = -frac;
#if 0
                    float32_t scale = is_intensity(icsr, g, sfb) *
                        invert_intensity(ics, g, sfb) *
                        (real_t)exp(LN05 * (0.25*icsr->scale_factors[g][sfb]));
#endif

                    if (icsr->scale_factors[g][sfb] > 0)
                    {
                        int32_t shift = icsr->scale_factors[g][sfb] >> 2;

                        if (shift > 31)
                            shift = 31;
                        for (i = icsr->swb_offset[sfb]; i < icsr->swb_offset[sfb+1]; i++)
                        {
                            k = (group*nshort) + i;
                            r_spec[k] = l_spec[k];
                            r_spec[k] >>= shift;
                            r_spec[k] = MUL_R_C(r_spec[k],frac);
                        }
                    } else {
                        int32_t shift = -(icsr->scale_factors[g][sfb] >> 2);

                        for (i = icsr->swb_offset[sfb]; i < icsr->swb_offset[sfb+1]; i++)
                        {
                            k = (group*nshort)+i;
                            r_spec[k] = l_spec[k];
                            r_spec[k] <<= shift;
                            r_spec[k] = MUL_R_C(r_spec[k],frac);
                        }
                    }
                }
            }
            group++;
        }
    }
}

#else

void is_decode(ic_stream *ics, ic_stream *icsr, real_t *l_spec, real_t *r_spec,
               uint16_t frame_len)
{
    uint8_t g, sfb, b;
    uint16_t i, k;
    real_t scale;

    uint16_t nshort = frame_len/8;
    uint8_t group = 0;

    for (g = 0; g < icsr->num_window_groups; g++)
    {
        /* Do intensity stereo decoding */
        for (b = 0; b < icsr->window_group_length[g]; b++)
        {
            for (sfb = 0; sfb < icsr->max_sfb; sfb++)
            {
                if (is_intensity(icsr, g, sfb))
                {
                    /* For scalefactor bands coded in intensity stereo the
                       corresponding predictors in the right channel are
                       switched to "off".
                     */
                    ics->pred.prediction_used[sfb] = 0;
                    icsr->pred.prediction_used[sfb] = 0;

                    scale = is_intensity(icsr, g, sfb) *
                        invert_intensity(ics, g, sfb) *
                        exp(LN05 * (0.25*icsr->scale_factors[g][sfb]));

                    /* Scale from left to right channel,
                       do not touch left channel */
                    for (i = icsr->swb_offset[sfb]; i < icsr->swb_offset[sfb+1]; i++)
                    {
                        k = (group*nshort)+i;
                        r_spec[k] = MUL(l_spec[k], scale);
                    }
                }
            }
            group++;
        }
    }
}

#endif
