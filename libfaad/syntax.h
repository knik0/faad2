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
** $Id: syntax.h,v 1.16 2002/08/05 20:33:38 menno Exp $
**/

#ifndef __SYNTAX_H__
#define __SYNTAX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bits.h"

#define MAIN    0
#define LC      1
#define SSR     2
#define LTP     3
#define LD      23
#define ER_LC   17
#define ER_LTP  19
#define DRM_ER_LC   27 /* special object type for DRM */


/* First object type that has ER */
#define ER_OBJECT_START 17


/* Bitstream */
#define LEN_SE_ID 3
#define LEN_TAG   4
#define LEN_BYTE  8

#define EXT_FILL_DATA     1
#define EXT_DYNAMIC_RANGE 11

/* Syntax elements */
#define ID_SCE 0x0
#define ID_CPE 0x1
#define ID_CCE 0x2
#define ID_LFE 0x3
#define ID_DSE 0x4
#define ID_PCE 0x5
#define ID_FIL 0x6
#define ID_END 0x7

#define MAX_CHANNELS        64
#define MAX_SYNTAX_ELEMENTS 48
#define MAX_WINDOW_GROUPS    8
#define MAX_SFB             51
#define MAX_LTP_SFB         40
#define MAX_LTP_SFB_S        8


#define ONLY_LONG_SEQUENCE   0x0
#define LONG_START_SEQUENCE  0x1
#define EIGHT_SHORT_SEQUENCE 0x2
#define LONG_STOP_SEQUENCE   0x3

#define ZERO_HCB       0
#define FIRST_PAIR_HCB 5
#define ESC_HCB        11
#define QUAD_LEN       4
#define PAIR_LEN       2
#define BOOKSCL        12
#define NOISE_HCB      13
#define INTENSITY_HCB2 14
#define INTENSITY_HCB  15


typedef struct
{
    uint8_t element_instance_tag;
    uint8_t object_type;
    uint8_t sf_index;
    uint8_t num_front_channel_elements;
    uint8_t num_side_channel_elements;
    uint8_t num_back_channel_elements;
    uint8_t num_lfe_channel_elements;
    uint8_t num_assoc_data_elements;
    uint8_t num_valid_cc_elements;
    uint8_t mono_mixdown_present;
    uint8_t mono_mixdown_element_number;
    uint8_t stereo_mixdown_present;
    uint8_t stereo_mixdown_element_number;
    uint8_t matrix_mixdown_idx_present;
    uint8_t pseudo_surround_enable;
    uint8_t matrix_mixdown_idx;
    uint8_t front_element_is_cpe[16];
    uint8_t front_element_tag_select[16];
    uint8_t side_element_is_cpe[16];
    uint8_t side_element_tag_select[16];
    uint8_t back_element_is_cpe[16];
    uint8_t back_element_tag_select[16];
    uint8_t lfe_element_tag_select[16];
    uint8_t assoc_data_element_tag_select[16];
    uint8_t cc_element_is_ind_sw[16];
    uint8_t valid_cc_element_tag_select[16];

    uint8_t channels;

    uint8_t comment_field_bytes;
    uint8_t comment_field_data[257];
} program_config;

typedef struct
{
    uint16_t syncword;
    uint8_t id;
    uint8_t layer;
    uint8_t protection_absent;
    uint8_t profile;
    uint8_t sf_index;
    uint8_t private_bit;
    uint8_t channel_configuration;
    uint8_t original;
    uint8_t home;
    uint8_t emphasis;
    uint8_t copyright_identification_bit;
    uint8_t copyright_identification_start;
    uint16_t aac_frame_length;
    uint16_t adts_buffer_fullness;
    uint8_t no_raw_data_blocks_in_frame;
    uint16_t crc_check;
} adts_header;

typedef struct
{
    uint8_t copyright_id_present;
    int8_t copyright_id[10];
    uint8_t original_copy;
    uint8_t home;
    uint8_t bitstream_type;
    uint32_t bitrate;
    uint8_t num_program_config_elements;
    uint32_t adif_buffer_fullness;

    program_config pce;
} adif_header;

typedef struct
{
    uint8_t last_band;
    uint8_t data_present;
    uint16_t lag;
    uint8_t lag_update;
    uint8_t coef;
    uint8_t long_used[51];
    uint8_t short_used[8];
    uint8_t short_lag_present[8];
    uint8_t short_lag[8];
} ltp_info;

typedef struct
{
    uint8_t limit;
    uint8_t predictor_reset;
    uint8_t predictor_reset_group_number;
    uint8_t prediction_used[41];
} pred_info;

typedef struct
{
    uint8_t number_pulse;
    uint8_t pulse_start_sfb;
    uint8_t pulse_offset[4];
    uint8_t pulse_amp[4];
} pulse_info;

typedef struct
{
    uint8_t n_filt[8];
    uint8_t coef_res[8];
    uint8_t length[8][4];
    uint8_t order[8][4];
    uint8_t direction[8][4];
    uint8_t coef_compress[8][4];
    uint8_t coef[8][4][32];
} tns_info;

typedef struct
{
    uint8_t present;

    uint8_t num_bands;
    uint8_t pce_instance_tag;
    uint8_t excluded_chns_present;
    uint8_t band_top[17];
    uint8_t prog_ref_level;
    uint8_t dyn_rng_sgn[17];
    uint8_t dyn_rng_ctl[17];
    uint8_t exclude_mask[MAX_CHANNELS];
    uint8_t additional_excluded_chns[MAX_CHANNELS];

    real_t ctrl1;
    real_t ctrl2;
} drc_info;

typedef struct
{
    uint8_t max_sfb;

    uint8_t num_swb;
    uint8_t num_window_groups;
    uint8_t num_windows;
    uint8_t window_sequence;
    uint8_t window_group_length[8];
    uint8_t window_shape;
    uint8_t scale_factor_grouping;
    uint16_t sect_sfb_offset[8][15*8];
    uint16_t swb_offset[52];

    uint8_t sect_cb[8][15*8];
    uint16_t sect_start[8][15*8];
    uint16_t sect_end[8][15*8];
    uint8_t sfb_cb[8][8*15];
    uint8_t num_sec[8]; /* number of sections in a group */

    uint8_t global_gain;
    uint16_t scale_factors[8][51];

    uint8_t ms_mask_present;
    uint8_t ms_used[MAX_WINDOW_GROUPS][MAX_SFB];

    uint8_t noise_used;

    uint8_t pulse_data_present;
    uint8_t tns_data_present;
    uint8_t gain_control_data_present;
    uint8_t predictor_data_present;

    pulse_info pul;
    tns_info tns;
    pred_info pred;
    ltp_info ltp;
    ltp_info ltp2;

#ifdef ERROR_RESILIENCE
    /* ER HCR data */
    uint16_t length_of_reordered_spectral_data;
    uint8_t length_of_longest_codeword;
    /* ER RLVC data */
    uint8_t sf_concealment;
    uint8_t rev_global_gain;
    uint16_t length_of_rvlc_sf;
    uint16_t dpcm_noise_nrg;
    uint8_t sf_escapes_present;
    uint8_t length_of_rvlc_escapes;
    uint16_t dpcm_noise_last_position;
#endif
} ic_stream; /* individual channel stream */

typedef struct
{
    uint8_t ele_id;

    uint8_t channel;
    uint8_t paired_channel;

    uint8_t element_instance_tag;
    uint8_t common_window;

    ic_stream ics1;
    ic_stream ics2;
} element; /* syntax element (SCE, CPE, LFE) */


int8_t GASpecificConfig(bitfile *ld, uint8_t *channelConfiguration,
                        uint8_t object_type,
                        uint8_t *aacSectionDataResilienceFlag,
                        uint8_t *aacScalefactorDataResilienceFlag,
                        uint8_t *aacSpectralDataResilienceFlag,
                        uint8_t *frameLengthFlag);
uint8_t single_lfe_channel_element(element *sce, bitfile *ld, int16_t *spec_data,
                                   uint8_t sf_index, uint8_t object_type,
                                   uint16_t frame_len
#ifdef ERROR_RESILIENCE
                                   ,uint8_t aacSectionDataResilienceFlag,
                                   uint8_t aacScalefactorDataResilienceFlag,
                                   uint8_t aacSpectralDataResilienceFlag
#endif
                                   );
uint8_t channel_pair_element(element *cpe, bitfile *ld, int16_t *spec_data1,
                             int16_t *spec_data2, uint8_t sf_index, uint8_t object_type,
                             uint16_t frame_len
#ifdef ERROR_RESILIENCE
                             ,uint8_t aacSectionDataResilienceFlag,
                             uint8_t aacScalefactorDataResilienceFlag,
                             uint8_t aacSpectralDataResilienceFlag
#endif
                             );
uint16_t data_stream_element(bitfile *ld);
uint8_t program_config_element(program_config *pce, bitfile *ld);
uint8_t fill_element(bitfile *ld, drc_info *drc
#ifdef SBR
                     ,uint8_t next_ele_id
#endif
                     );
uint8_t adts_frame(adts_header *adts, bitfile *ld);
void get_adif_header(adif_header *adif, bitfile *ld);


/* static functions */
static uint8_t individual_channel_stream(element *ele, bitfile *ld,
                                     ic_stream *ics, uint8_t scal_flag,
                                     int16_t *spec_data, uint8_t sf_index,
                                     uint8_t object_type, uint16_t frame_len
#ifdef ERROR_RESILIENCE
                                     ,uint8_t aacSectionDataResilienceFlag,
                                     uint8_t aacScalefactorDataResilienceFlag,
                                     uint8_t aacSpectralDataResilienceFlag
#endif
                                     );
static uint8_t ics_info(ic_stream *ics, bitfile *ld, uint8_t common_window,
                        uint8_t fs_index, uint8_t object_type,
                        uint16_t frame_len);
static void section_data(ic_stream *ics, bitfile *ld
#ifdef ERROR_RESILIENCE
                         ,uint8_t aacSectionDataResilienceFlag
#endif
                         );
static uint8_t scale_factor_data(ic_stream *ics, bitfile *ld
#ifdef ERROR_RESILIENCE
                                 ,uint8_t aacScalefactorDataResilienceFlag
#endif
                                 );
static void gain_control_data(bitfile *ld, ic_stream *ics);
static uint8_t spectral_data(ic_stream *ics, bitfile *ld, int16_t *spectral_data,
                             uint16_t frame_len);
static uint16_t extension_payload(bitfile *ld, drc_info *drc, uint16_t count);
#ifdef ERROR_RESILIENCE
uint8_t reordered_spectral_data(ic_stream *ics, bitfile *ld, int16_t *spectral_data,
                                uint16_t frame_len, uint8_t aacSectionDataResilienceFlag);
#endif
static void pulse_data(pulse_info *pul, bitfile *ld);
static void tns_data(ic_stream *ics, tns_info *tns, bitfile *ld);
static void ltp_data(ic_stream *ics, ltp_info *ltp, bitfile *ld,
                     uint8_t object_type);
static uint8_t adts_fixed_header(adts_header *adts, bitfile *ld);
static void adts_variable_header(adts_header *adts, bitfile *ld);
static void adts_error_check(adts_header *adts, bitfile *ld);
static uint8_t dynamic_range_info(bitfile *ld, drc_info *drc);
static uint8_t excluded_channels(bitfile *ld, drc_info *drc);


#ifdef __cplusplus
}
#endif
#endif
