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
** $Id: pns.c,v 1.12 2002/08/30 12:11:57 menno Exp $
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


static const unsigned char Parity [256] = {  // parity
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0
};

static unsigned int  __r1 = 1;
static unsigned int  __r2 = 1;

static INLINE uint32_t random2()
{
    uint32_t  t1, t2, t3, t4;

    t3   = t1 = __r1;   t4   = t2 = __r2;       // Parity calculation is done via table lookup, this is also available
    t1  &= 0xF5;        t2 >>= 25;              // on CPUs without parity, can be implemented in C and avoid unpredictable
    t1   = Parity [t1]; t2  &= 0x63;            // jumps and slow rotate through the carry flag operations.
    t1 <<= 31;          t2   = Parity [t2];

    return (__r1 = (t3 >> 1) | t1 ) ^ (__r2 = (t4 + t4) | t2 );
}

/* The function gen_rand_vector(addr, size) generates a vector of length
   <size> with signed random values of average energy MEAN_NRG per random
   value. A suitable random number generator can be realized using one
   multiplication/accumulation per random value.
*/
static INLINE void gen_rand_vector(real_t *spec, uint16_t scale_factor, uint16_t size)
{
    uint16_t i;
    real_t energy = 0;

    /* 14496-3 says:
       scale = 1.0f/(size * (fftw_real)sqrt(MEAN_NRG));
    */
    float32_t scale = 1.0/(float32_t)sqrt(size * MEAN_NRG);

    for (i = 0; i < size; i++)
    {
        spec[i] = REAL_CONST(scale*(float32_t)random2());
        energy += MUL(spec[i],spec[i]);
    }

    scale = 1 / (float32_t)sqrt(energy);
    scale *= (float32_t)pow(2.0, 0.25 * scale_factor);
    for (i = 0; i < size; i++)
    {
        spec[i] = MUL(REAL_CONST(scale), spec[i]);
    }
}

void pns_decode(ic_stream *ics, real_t *spec, uint16_t frame_len)
{
    uint8_t g, sfb, b;
    uint16_t size, offs;

    uint8_t group = 0;
    uint16_t nshort = frame_len >> 3;

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
