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
** $Id: lt_predict.h,v 1.1 2002/01/14 19:15:56 menno Exp $
**/

#ifndef __LT_PREDICT_H__
#define __LT_PREDICT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "filtbank.h"

void lt_prediction(ic_stream *ics, ltp_info *ltp, float *spec,
                   float *lt_pred_stat, fb_info *fb, int win_shape,
                   int win_shape_prev, int sr_index, int object_type);
void lt_update_state(float *lt_pred_stat, float *time, float *overlap);

#ifdef __cplusplus
}
#endif
#endif
