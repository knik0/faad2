/*
 * FAAD - Freeware Advanced Audio Decoder
 * Copyright (C) 2001 M. Bakker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: predict.c,v 1.1 2002/01/14 19:15:57 menno Exp $
 */

#include "syntax.h"
#include "ic_predict.h"

void quantize_pred_state(pred_state *state, tmp_pred_state *tmp_state)
{
    int i;
    short *s;
    unsigned long *s_tmp;

    s     = (short*)state;
    s_tmp = (unsigned long*)tmp_state;

    for (i = 0; i < 6; i++)
        s[i] = (short)(s_tmp[i]>>16);
}

void inv_quantize_pred_state(pred_state *state, tmp_pred_state *tmp_state)
{
    int i;
    short *s;
    unsigned long *s_tmp;


    s_tmp = (unsigned long*)tmp_state;
    s     = (short*)state;
    
    for (i = 0; i < 6; i++)
        s_tmp[i] = ((unsigned long)s[i])<<16;
}

static void flt_round_inf(float *pf)
{
    int flg;
    unsigned long tmp;
    float *pt = (float *)&tmp;

    *pt = *pf;
    flg = tmp & (unsigned long)0x00008000;
    tmp &= (unsigned long)0xffff0000;
    *pf = *pt;

    /* round 1/2 lsb toward infinity */
    if (flg)
    {
        tmp &= (unsigned long)0xff800000; /* extract exponent and sign */
        tmp |= (unsigned long)0x00010000; /* insert 1 lsb */
        *pf += *pt;                       /* add 1 lsb and elided one */
        tmp &= (unsigned long)0xff800000; /* extract exponent and sign */
        *pf -= *pt;                       /* subtract elided one */
    }
}

static void ic_predict(pred_state *state, float input, float *output,
                        int pred, float *mnt_table, float *exp_table)
{
    unsigned long tmp;
    int i, j;
    float dr1, predictedvalue;
    float e0, e1;
    float r0, r1;
    float k1, k2;

    float *r;
    float *KOR;
    float *VAR;

    tmp_pred_state tmp_state2;
    tmp_pred_state *tmp_state;

    tmp_state = &tmp_state2;

    /* inversely quantize the prediction state */
    inv_quantize_pred_state(state, tmp_state);

    r   = tmp_state->r;   /* delay elements */
    KOR = tmp_state->KOR; /* correlations */
    VAR = tmp_state->VAR; /* variances */

    r0 = r[0];
    r1 = r[1];

    tmp = state->VAR[0];
    j   = (tmp >> 7); /* exponent */
    i   = tmp & 0x7f; /* mantissa */
    k1  = KOR[1-1] * exp_table[j] * mnt_table[i];

    if (pred)
    {
        /* only needed for the actual predicted value, k1 is always needed */
        tmp = state->VAR[1];
        j   = (tmp >> 7); /* exponent */
        i   = tmp & 0x7f; /* mantissa */
        k2  = KOR[2-1] * exp_table[j] * mnt_table[i];

        predictedvalue = k1*r0 + k2*r1;
        flt_round_inf(&predictedvalue);

        *output = input + predictedvalue;
    } else {
        *output = input;
    }

    /* calculate new state data */
    e0 = *output;
    e1 = e0 - k1 * r0;

    dr1 = k1 * e0;

    VAR[0] = ALPHA * VAR[0] + (0.5f)*(r0*r0 + e0*e0);
    KOR[0] = ALPHA * KOR[0] + r0*e0;
    VAR[1] = ALPHA * VAR[1] + (0.5f)*(r1*r1 + e1*e1);
    KOR[1] = ALPHA * KOR[1] + r1*e1;

    r1 = A * (r0-dr1);
    r0 = A * e0;

    r[0] = r0;
    r[1] = r1;

    /* quantize the prediction state */
    quantize_pred_state(state, tmp_state);
}

static void reset_pred_state(pred_state *state)
{
    state->r[0]   = 0;
    state->r[1]   = 0;
    state->KOR[0] = 0;
    state->KOR[1] = 0;
    state->VAR[0] = 0x3F80;
    state->VAR[1] = 0x3F80;
}

static void reset_all_predictors(pred_state *state)
{
    int i;

    for (i = 0; i < 1024; i++)
        reset_pred_state(&state[i]);
}

void ic_prediction_init(pred_state *state)
{
}

/* intra channel prediction */
void ic_prediction(ic_stream *ics, float *spec, pred_state *state,
                   pred_tables *tables)
{
    int sfb, bin;

    if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
    {
        reset_all_predictors(state);
    } else {
        for (sfb = 0; sfb < ics->pred.limit; sfb++)
        {
            int fc = ics->swb_offset[sfb];
            int lc = ics->swb_offset[sfb+1];

            for (bin = fc; bin < lc; bin++)
            {
                ic_predict(&state[bin], spec[bin], &spec[bin],
                    (ics->predictor_data_present &&
                    ics->pred.prediction_used[sfb]), tables->mnt_table,
                    tables->exp_table);
            }
        }

        if (ics->pred.predictor_reset)
        {
            int reset_group_number = ics->pred.predictor_reset_group_number;

            for (bin = reset_group_number - 1; bin < 1024; bin += 30)
            {
                reset_pred_state(&state[bin]);
            }
        }
    }
}
