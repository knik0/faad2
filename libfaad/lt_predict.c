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
** $Id: lt_predict.c,v 1.1 2002/01/14 19:15:56 menno Exp $
**/

#include "syntax.h"
#include "lt_predict.h"
#include "filtbank.h"
#include "tns.h"

static float codebook[8] =
{
    0.570829f, 0.696616f, 0.813004f, 0.911304f, 0.984900f, 1.067894f,
    1.194601f, 1.369533f
};

void lt_prediction(ic_stream *ics, ltp_info *ltp, float *spec,
                   float *lt_pred_stat, fb_info *fb, int win_shape,
                   int win_shape_prev, int sr_index, int object_type)
{
    int sfb, i, bin;
    int num_samples;
    float x_est[2*1024];
    float X_est[2*1024];

    if (ics->window_sequence != EIGHT_SHORT_SEQUENCE)
    {
        if (ltp->data_present)
        {
            if (ltp->lag < 1024)
                num_samples = 1024 + ltp->lag;
            else
                num_samples = 2*1024;

            for(i = 0; i < num_samples; i++)
                x_est[i] = codebook[ltp->coef] * lt_pred_stat[i - ltp->lag + 2*1024];
            for( ; i < 2*1024; i++)
                x_est[i] = 0.0f;

            filter_bank_ltp(fb, ics->window_sequence, win_shape, win_shape_prev,
                x_est, X_est);

            tns_encode_frame(ics, &(ics->tns), sr_index, object_type, X_est);

            for (sfb = 0; sfb < ltp->last_band; sfb++)
            {
                if (ltp->long_used[sfb])
                {
                    int low  = ics->swb_offset[sfb];
                    int high = ics->swb_offset[sfb+1];

                    for (bin = low; bin < high; bin++)
                    {
                        spec[bin] += X_est[bin];
                    }
                }
            }
        }
    }
}

void lt_update_state(float *lt_pred_stat, float *time, float *overlap)
{
    int i;

    for(i = 0; i < 1024; i++)
    {
        lt_pred_stat[i]          = lt_pred_stat[i + 1024];
        lt_pred_stat[1024 + i]   = time[i];
        lt_pred_stat[2*1024 + i] = overlap[i];
    }
}