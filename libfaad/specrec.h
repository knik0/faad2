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
** $Id: specrec.h,v 1.6 2002/08/26 18:41:47 menno Exp $
**/

#ifndef __SPECREC_H__
#define __SPECREC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "syntax.h"

/* !!!DON'T CHANGE IQ_TABLE_SIZE!!! */
#define IQ_TABLE_SIZE   1026


uint8_t window_grouping_info(ic_stream *ics, uint8_t fs_index,
                             uint8_t object_type, uint16_t frame_len);
void quant_to_spec(ic_stream *ics, real_t *spec_data, uint16_t frame_len);
void build_tables(real_t *iq_table);
void iquant_and_apply_scalefactors(ic_stream *ics, real_t *x_invquant,
                                   int16_t *x_quant, real_t *iq_table,
                                   uint16_t frame_len);

#ifdef __cplusplus
}
#endif
#endif
