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
** $Id: mdct.h,v 1.11 2002/09/08 18:14:37 menno Exp $
**/

#ifndef __MDCT_H__
#define __MDCT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "cfft.h"

typedef struct {
    real_t sin;
    real_t cos;
} faad_sincos;

typedef struct {
    faad_sincos *sincos;
    real_t *Z1;
    complex_t *Z2;
    cfft_info *cfft;
    uint16_t N;
} mdct_info;

mdct_info *faad_mdct_init(uint16_t N);
void faad_mdct_end(mdct_info *mdct);
void faad_imdct(mdct_info *mdct, real_t *X_in, real_t *X_out);
void faad_mdct(mdct_info *mdct, real_t *X_in, real_t *X_out);


#ifdef __cplusplus
}
#endif
#endif
