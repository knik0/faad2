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
** $Id: filtbank.h,v 1.1 2002/01/14 19:15:55 menno Exp $
**/

#ifndef __FILTBANK_H__
#define __FILTBANK_H__

#ifdef __cplusplus
extern "C" {
#endif


#define BLOCK_LEN_LONG  1024
#define BLOCK_LEN_SHORT  128


typedef struct
{
    short unscrambled64[64];
    short unscrambled512[512];

    float *sin_long;
    float *sin_short;
} fb_info;

void filter_bank_init(fb_info *fb);
void filter_bank_end(fb_info *fb);

void filter_bank_ltp(fb_info *fb, int window_sequence, int window_shape,
                     int window_shape_prev, float *in_data, float *out_mdct);
void ifilter_bank(fb_info *fb, int window_sequence, int window_shape, 
                  int window_shape_prev, float *freq_in, float *time_buff,
                  float *time_out);

#ifdef __cplusplus
}
#endif
#endif
