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
** $Id: ic_predict.c,v 1.9 2002/09/08 18:14:37 menno Exp $
**/

#include "common.h"

#ifdef MAIN_DEC

#include "syntax.h"
#include "ic_predict.h"
#include "pns.h"

static void ic_predict(pred_state *state, real_t input, real_t *output, uint8_t pred)
{
    real_t dr1, predictedvalue;
    real_t e0, e1;
    real_t k1, k2;

    real_t *r;
    real_t *KOR;
    real_t *VAR;

    r   = state->r;   /* delay elements */
    KOR = state->KOR; /* correlations */
    VAR = state->VAR; /* variances */

    if (VAR[0] == REAL_CONST(0.0))
        k1 = REAL_CONST(0.0);
    else
        k1 = KOR[0]/VAR[0]*B;

    if (pred)
    {
        /* only needed for the actual predicted value, k1 is always needed */
        if (VAR[1] == REAL_CONST(0.0))
            k2 = REAL_CONST(0.0);
        else
            k2 = KOR[1]/VAR[1]*B;

        predictedvalue = MUL(k1, r[0]) + MUL(k2, r[1]);

        *output = input + predictedvalue;
    } else {
        *output = input;
    }

    /* calculate new state data */
    e0 = *output;
    e1 = e0 - MUL(k1, r[0]);

    dr1 = MUL(k1, e0);

    VAR[0] = MUL(ALPHA, VAR[0]) + MUL(REAL_CONST(0.5), (MUL(r[0], r[0]) + MUL(e0, e0)));
    KOR[0] = MUL(ALPHA, KOR[0]) + MUL(r[0], e0);
    VAR[1] = MUL(ALPHA, VAR[1]) + MUL(REAL_CONST(0.5), (MUL(r[1], r[1]) + MUL(e1, e1)));
    KOR[1] = MUL(ALPHA, KOR[1]) + MUL(r[1], e1);

    r[1] = MUL(A, (r[0]-dr1));
    r[0] = MUL(A, e0);
}

static void reset_pred_state(pred_state *state)
{
    state->r[0]   = REAL_CONST(0.0);
    state->r[1]   = REAL_CONST(0.0);
    state->KOR[0] = REAL_CONST(0.0);
    state->KOR[1] = REAL_CONST(0.0);
    state->VAR[0] = REAL_CONST(1.0);
    state->VAR[1] = REAL_CONST(1.0);
}

void pns_reset_pred_state(ic_stream *ics, pred_state *state)
{
    uint8_t sfb, g, b;
    uint16_t i, offs, offs2;

    /* prediction only for long blocks */
    if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
        return;

    for (g = 0; g < ics->num_window_groups; g++)
    {
        for (b = 0; b < ics->window_group_length[g]; b++)
        {
            for (sfb = 0; sfb < ics->max_sfb; sfb++)
            {
                if (is_noise(ics, g, sfb))
                {
                    offs = ics->swb_offset[sfb];
                    offs2 = ics->swb_offset[sfb+1];

                    for (i = offs; i < offs2; i++)
                        reset_pred_state(&state[i]);
                }
            }
        }
    }
}

void reset_all_predictors(pred_state *state, uint16_t frame_len)
{
    uint16_t i;

    for (i = 0; i < frame_len; i++)
        reset_pred_state(&state[i]);
}

/* intra channel prediction */
void ic_prediction(ic_stream *ics, real_t *spec, pred_state *state,
                   uint16_t frame_len)
{
    uint8_t sfb;
    uint16_t bin;

    if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
    {
        reset_all_predictors(state, frame_len);
    } else {
        for (sfb = 0; sfb < ics->pred.limit; sfb++)
        {
            uint16_t low  = ics->swb_offset[sfb];
            uint16_t high = ics->swb_offset[sfb+1];

            for (bin = low; bin < high; bin++)
            {
                ic_predict(&state[bin], spec[bin], &spec[bin],
                    (ics->predictor_data_present &&
                    ics->pred.prediction_used[sfb]));
            }
        }

        if (ics->predictor_data_present)
        {
            if (ics->pred.predictor_reset)
            {
                for (bin = ics->pred.predictor_reset_group_number - 1;
                     bin < frame_len; bin += 30)
                {
                    reset_pred_state(&state[bin]);
                }
            }
        }
    }
}

#endif
