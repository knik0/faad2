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
** $Id: is.c,v 1.2 2002/02/18 10:01:05 menno Exp $
**/

#include "common.h"

#ifdef USE_FMATH
#include <mathf.h>
#else
#include <math.h>
#endif
#include "syntax.h"
#include "is.h"

void is_decode(ic_stream *ics, ic_stream *icsr, real_t *l_spec, real_t *r_spec)
{
    uint8_t g, sfb, b;
    uint16_t i, k;
    real_t scale;

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
#ifdef USE_FMATH
                        powf(0.5f, (0.25f*icsr->scale_factors[g][sfb]));
#else
                        (real_t)pow(0.5, (0.25*icsr->scale_factors[g][sfb]));
#endif

                    /* Scale from left to right channel,
                       do not touch left channel */
                    for (i = icsr->swb_offset[sfb]; i < icsr->swb_offset[sfb+1]; i++)
                    {
                        k = (group*128)+i;
                        r_spec[k] = l_spec[k] * scale;
                    }
                }
            }
            group++;
        }
    }
}
