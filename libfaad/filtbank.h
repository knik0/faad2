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
** $Id: filtbank.h,v 1.9 2002/08/26 18:41:47 menno Exp $
**/

#ifndef __FILTBANK_H__
#define __FILTBANK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "mdct.h"


typedef struct
{
    real_t *long_window[2];
    real_t *short_window[2];
#ifdef LD_DEC
    real_t *ld_window[2];
#endif

    mdct_info *mdct256;
#ifdef LD_DEC
    mdct_info *mdct1024;
#endif
    mdct_info *mdct2048;
} fb_info;

fb_info *filter_bank_init(uint16_t frame_len);
void filter_bank_end(fb_info *fb);

#ifdef LTP_DEC
void filter_bank_ltp(fb_info *fb,
                     uint8_t window_sequence,
                     uint8_t window_shape,
                     uint8_t window_shape_prev,
                     real_t *in_data,
                     real_t *out_mdct,
                     uint8_t object_type,
                     uint16_t frame_len);
#endif

void ifilter_bank(fb_info *fb,
                  uint8_t window_sequence,
                  uint8_t window_shape,
                  uint8_t window_shape_prev,
                  real_t *freq_in,
                  real_t *time_out,
                  uint8_t object_type,
                  uint16_t frame_len);

#ifdef __cplusplus
}
#endif
#endif
