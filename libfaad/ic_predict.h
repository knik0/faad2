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
** $Id: ic_predict.h,v 1.2 2002/02/18 10:01:05 menno Exp $
**/

#ifndef __IC_PREDICT_H__
#define __IC_PREDICT_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ALPHA      0.90625f
#define A          0.953125f
#define B          0.953125f


/* used to save the state */
typedef struct {
    real_t r[2];
    real_t KOR[2];
    real_t VAR[2];
} pred_state;


void pns_reset_pred_state(ic_stream *ics, pred_state *state);
void reset_all_predictors(pred_state *state);
void ic_prediction(ic_stream *ics, real_t *spec, pred_state *state);


#ifdef __cplusplus
}
#endif
#endif
