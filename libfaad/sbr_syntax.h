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
** $Id: sbr_syntax.h,v 1.2 2002/04/20 22:20:15 menno Exp $
**/

#ifdef SBR

#ifndef __SBR_SYNTAX_H__
#define __SBR_SYNTAX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bits.h"


#define SBR_STD 12
#define SBR_HDR 13

#define FIXFIX 0
#define FIXVAR 1
#define VARFIX 2
#define VARVAR 3

#define NO_TIME_SLOTS 16

typedef struct
{
    uint8_t bs_crc_flag;
    uint8_t bs_sbr_crc_bits;
    uint8_t bs_protocol_version;
    uint8_t bs_amp_res;
    uint8_t bs_start_freq;
    uint8_t bs_stop_freq;
    uint8_t bs_xover_band;
    uint8_t bs_freq_scale;
    uint8_t bs_alter_scale;
    uint8_t bs_noise_bands;
    uint8_t bs_limiter_bands;
    uint8_t bs_limiter_gains;
    uint8_t bs_interpol_freq;
    uint8_t bs_smoothing_mode;
    uint8_t bs_samplerate_mode;
    uint8_t bs_add_harmonic_flag[2];
    uint8_t bs_extended_data[2];
    uint8_t bs_extension_id;
    uint8_t bs_coupling;
    uint8_t bs_frame_class;
    uint8_t bs_num_env[2];
    uint8_t bs_freq_res[2][6];
    uint8_t bs_abs_bord[2];
    uint8_t bs_rel_bord[2][9];
    uint8_t bs_rel_bord_0[2][9];
    uint8_t bs_rel_bord_1[2][9];
    uint8_t bs_pointer[2];
    uint8_t bs_abs_bord_0[2];
    uint8_t bs_abs_bord_1[2];
    uint8_t bs_num_rel_0[2];
    uint8_t bs_num_rel_1[2];
    uint8_t bs_num_noise[2];
    uint8_t bs_df_env[2][9];
    uint8_t bs_df_noise[2][3];
    uint8_t num_noise_bands[2];
    uint8_t bs_invf_mode_vec[2][/*??*/10];
    uint8_t num_high_res[2];
    uint8_t bs_add_harmonic[2][/*??*/10];
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