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
** $Id: pns.c,v 1.7 2002/06/13 11:03:27 menno Exp $
**/

#include "common.h"

#include "pns.h"


/* Needs some more work */
/* From the spec:
   If the same scalefactor band and group is coded by perceptual noise
   substitution in both channels of a channel pair, the correlation of
   the noise signal can be controlled by means of the ms_used field: While
   the default noise generation process works independently for each channel
   (separate generation of random vectors), the same random vector is used
   for both channels if ms_used[] is set for a particular scalefactor band
   and group. In this case, no M/S stereo coding is carried out (because M/S
   stereo coding and noise substitution coding are mutually exclusive).
   If the same scalefactor band and group is coded by perceptual noise
   substitution in only one channel of a channel pair the setting of ms_used[]
   is not evaluated.
*/



static INLINE int32_t random2()
{
    static int32_t state = 1;

    state = (1664525L * state) + 1013904223L;  /* Numerical recipes */

    return state;
}

/* The function gen_rand_vector(addr, size) generates a vector of length
   <size> with signed random values of average energy MEAN_NRG per random
   value. A suitable random number generator can be realized using one
   multiplication/accumulation per random value.
*/
static INLINE void gen_rand_vector(real_t *spec, uint16_t scale_factor, uint16_t size)
{
    uint16_t i;
    real_t scale;

    for (i = 0; i < size; i++)
    {
        spec[i] = (real_t)random2();
    }

    /* 14496-3 says:
       scale = 1.0f/(size * (real_t)sqrt(MEAN_NRG));
    */
    scale = 1.0f/(real_t)sqrt(size * MEAN_NRG);
    scale = MUL(scale, (real_t)exp(LN2 * 0.25 * scale_factor));

    /* Scale random vector to desired target energy */
    for (i = 0; i < size; i++)
        spec[i] = MUL(spec[i], scale);
}

void pns_decode(ic_stream *ics, real_t *spec, uint16_t frame_len)
{
    uint8_t g, sfb, b;
    uint16_t size, offs;

    uint8_t group = 0;
    uint16_t nshort = frame_len/8;

    for (g = 0; g < ics->num_window_groups; g++)
    {
        /* Do perceptual noise substitution decoding */
        for (b = 0; b < ics->window_group_length[g]; b++)
        {
            for (sfb = 0; sfb < ics->max_sfb; sfb++)
            {
                if (is_noise(ics, g, sfb))
                {
                    /* Simultaneous use of LTP and PNS is not prevented in the
                       syntax. If both LTP, and PNS are enabled on the same
                       scalefactor band, PNS takes precedence, and no prediction
                       is applied to this band.
                     */
                    ics->ltp.long_used[sfb] = 0;
                    ics->ltp2.long_used[sfb] = 0;

                    /* For scalefactor bands coded using PNS the corresponding
                       predictors are switched to "off".
                     */
                    ics->pred.prediction_used[sfb] = 0;
                    
                    offs = ics->swb_offset[sfb];
                    size = ics->swb_offset[sfb+1] - offs;

                    /* Generate random vector */
                    gen_rand_vector(&spec[(group*nshort)+offs],
                        ics->scale_factors[g][sfb], size);
                }
            }
            group++;
        }
    }
}
