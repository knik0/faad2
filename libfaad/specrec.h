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
** $Id: specrec.h,v 1.1 2002/01/14 19:15:57 menno Exp $
**/

#ifndef __SPECREC_H__
#define __SPECREC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "syntax.h"

#define IQ_TABLE_SIZE   200
#define POW_TABLE_SIZE  200


int window_grouping_info(ic_stream *ics, int fs_index);
void quant_to_spec(ic_stream *ics, float *spec_data);
void build_tables(float *iq_table, float *pow2_table);
void inverse_quantization(float *x_invquant, short *x_quant, float *iq_table);
void apply_scalefactors(ic_stream *ics, float *x_invquant, float *pow2_table);


#ifdef __cplusplus
}
#endif
#endif
