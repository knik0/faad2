/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2004 M. Bakker, Ahead Software AG, http://www.nero.com
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
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: drm_dec.h,v 1.1 2004/01/19 21:49:53 menno Exp $
**/

#ifndef __DRM_DEC_H__
#define __DRM_DEC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bits.h"

#define DRM_PARAMETRIC_STEREO 0

typedef struct
{
    uint8_t bs_enable_sa;
    uint8_t bs_enable_pan;

    uint8_t bs_sa_dt_flag;
    uint8_t bs_pan_dt_flag;

    int8_t bs_sa_data[8];
    int8_t bs_pan_data[20];
} drm_ps_info;

uint16_t drm_ps_data(drm_ps_info *ps, bitfile *ld);


#ifdef __cplusplus
}
#endif
#endif

