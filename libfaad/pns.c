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
** $Id: pns.c,v 1.1 2002/01/14 19:15:56 menno Exp $
**/

#ifdef __ICL
#include <mathf.h>
#else
#include <math.h>
#endif
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



static __inline long random2(long *seed)
{
    *seed = (1664525L * *seed) + 1013904223L;  /* Numerical recipes */

    return *seed;
}

/* The function gen_rand_vector(addr, size) generates a vector of length
   <size> with signed random values of average energy MEAN_NRG per random
   value. A suitable random number generator can be realized using one
   multiplication/accumulation per random value.
*/
static __inline void gen_rand_vector(float *spec, int size, long *state)
{
    int i;

    for (i = 0; i < size; i++)
    {
        spec[i] = (float)random2(state);
    }
}

void pns_decode(ic_stream *ics, float *spec)
{
    int g, sfb, b, i;
    int size, offs;
    float scale;

    int group = 0;

    static int state = 1;

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
                    gen_rand_vector(&spec[(group*128)+offs], size, &state);

                    /* 14496-3 says:
                       scale = 1.0f/(size * (float)sqrt(MEAN_NRG));
                     */
#ifdef __ICL
                    scale = 1.0f/sqrtf(size * MEAN_NRG);
                    scale *= powf(2.0f, 0.25f*ics->scale_factors[g][sfb]);
#else
                    scale = 1.0f/(float)sqrt(size * MEAN_NRG);
                    scale *= (float)pow(2.0, 0.25*ics->scale_factors[g][sfb]);
#endif

                    /* Scale random vector to desired target energy */
                    for (i = 0; i < size; i++)
                        spec[(group*128)+offs+i] *= scale;
                }
            }
            group++;
        }
    }
}
