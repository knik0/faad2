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
** $Id: tns.h,v 1.1 2002/01/14 19:15:57 menno Exp $
**/

#ifndef __TNS_H__
#define __TNS_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif


#define TNS_MAX_ORDER 20

    
void tns_decode_frame(ic_stream *ics, tns_info *tns, int sr_index,
                      int object_type, float *spec);
void tns_encode_frame(ic_stream *ics, tns_info *tns, int sr_index,
                      int object_type, float *spec);

static void tns_decode_coef(int order, int coef_res_bits, int coef_compress,
                            int *coef, float *a);
static void tns_ar_filter(float *spectrum, int size, int inc, float *lpc,
                          int order);
static void tns_ma_filter(float *spectrum, int size, int inc, float *lpc,
                          int order);
static int tns_max_bands(ic_stream *ics, int sr_index);
static int tns_max_order(ic_stream *ics, int sr_index,
                         int object_type);


#ifdef __cplusplus
}
#endif
#endif
