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
** $Id: sbr_syntax.h,v 1.1 2002/04/20 14:45:13 menno Exp $
**/

#ifdef SBR

#ifndef __SBR_SYNTAX_H__
#define __SBR_SYNTAX_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SBR_STD 12
#define SBR_HDR 13

#define FIXFIX 0
#define FIXVAR 1
#define VARFIX 2
#define VARVAR 3

typedef struct
{
    uint8_t dummy;
} sbr_info;

uint8_t sbr_bitstream(bitfile *ld, sbr_info *sbr, uint8_t id_aac,
                      uint8_t bs_extension_type);
static void sbr_header(bitfile *ld, sbr_info *sbr, uint8_t id_aac);
static void sbr_data(bitfile *ld, sbr_info *sbr, uint8_t id_aac);
static void sbr_single_channel_element(bitfile *ld, sbr_info *sbr);
static void sbr_channel_pair_element(bitfile *ld, sbr_info *sbr);
static void sbr_grid(bitfile *ld, sbr_info *sbr, uint8_t ch);
static void sbr_dtdf(bitfile *ld, sbr_info *sbr, uint8_t ch);
static void invf_mode(bitfile *ld, sbr_info *sbr, uint8_t ch);
static void sbr_envelope(bitfile *ld, sbr_info *sbr, uint8_t ch);
static void sbr_noise(bitfile *ld, sbr_info *sbr, uint8_t ch);
static void sinusoidal_coding(bitfile *ld, sbr_info *sbr, uint8_t ch);

#ifdef __cplusplus
}
#endif
#endif /* __SBR_SYNTAX_H__ */

#endif /* SBR */