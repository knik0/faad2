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
** $Id: syntax.h,v 1.2 2002/01/19 09:39:41 menno Exp $
**/

#ifndef __SYNTAX_H__
#define __SYNTAX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bits.h"

#define MAIN 0
#define LC   1
#define SSR  2
#define LTP  3


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
#define NSPECBOOKS     ESC_HCB + 1
#define BOOKSCL        NSPECBOOKS
#define NOISE_HCB      13
#define INTENSITY_HCB2 14
#define INTENSITY_HCB  15


typedef struct
{
    int element_instance_tag;
    int object_type;
    int sf_index;
    int num_front_channel_elements;
    int num_side_channel_elements;
    int num_back_channel_elements;
    int num_lfe_channel_elements;
    int num_assoc_data_elements;
    int num_valid_cc_elements;
    int mono_mixdown_present;
    int mono_mixdown_element_number;
    int stereo_mixdown_present;
    int stereo_mixdown_element_number;
    int matrix_mixdown_idx_present;
    int pseudo_surround_enable;
    int matrix_mixdown_idx;
    int front_element_is_cpe[16];
    int front_element_tag_select[16];
    int side_element_is_cpe[16];
    int side_element_tag_select[16];
    int back_element_is_cpe[16];
    int back_element_tag_select[16];
    int lfe_element_tag_select[16];
    int assoc_data_element_tag_select[16];
    int cc_element_is_ind_sw[16];
    int valid_cc_element_tag_select[16];

    int channels;

    int comment_field_bytes;
    unsigned char comment_field_data[257];
} program_config;

typedef struct
{
    int syncword;
    int id;
    int layer;
    int protection_absent;
    int profile;
    int sf_index;
    int private_bit;
    int channel_configuration;
    int original;
    int home;
    int emphasis;
    int copyright_identification_bit;
    int copyright_identification_start;
    int aac_frame_length;
    int adts_buffer_fullness;
    int no_raw_data_blocks_in_frame;
    int crc_check;
} adts_header;

typedef struct
{
    int copyright_id_present;
    char copyright_id[10];
    int original_copy;
    int home;
    int bitstream_type;
    int bitrate;
    int num_program_config_elements;
    int adif_buffer_fullness;

    program_config pce;
} adif_header;

typedef struct
{
    int last_band;
    int data_present;
    int lag;
    int coef;
    int short_used[8];
    int short_lag_present[8];
    int short_lag[8];
    int long_used[51];
} ltp_info;

typedef struct
{
    int limit;
    int predictor_reset;
    int predictor_reset_group_number;
    int prediction_used[41];
} pred_info;

typedef struct
{
    int number_pulse;
    int pulse_start_sfb;
    int pulse_offset[4];
    int pulse_amp[4];
} pulse_info;

typedef struct
{
    int n_filt[8];
    int coef_res[8];
    int length[8][4];
    int order[8][4];
    int direction[8][4];
    int coef_compress[8][4];
    int coef[8][4][32];
} tns_info;

typedef struct
{
    float ctrl1;
    float ctrl2;

    int present;

    int num_bands;
    int pce_instance_tag;
    int excluded_chns_present;
    int band_top[17];
    int prog_ref_level;
    int dyn_rng_sgn[17];
    int dyn_rng_ctl[17];
    int exclude_mask[MAX_CHANNELS];
    int additional_excluded_chns[MAX_CHANNELS];
} drc_info;

typedef struct
{
    int max_sfb;

    int num_swb;
    int num_window_groups;
    int num_windows;
    int window_sequence;
    int window_group_length[8];
    int window_shape;
    int scale_factor_grouping;
    int sect_sfb_offset[8][15*8];
    int swb_offset[51];

    int sect_cb[8][15*8];
    int sect_start[8][15*8];
    int sect_end[8][15*8];
    int sfb_cb[8][8*15];
    int num_sec[8]; /* number of sections in a group */

    int global_gain;

    int ms_mask_present;
    int ms_used[MAX_WINDOW_GROUPS][MAX_SFB];

    int pulse_data_present;
    int tns_data_present;
    int gain_control_data_present;
    int predictor_data_present;
    int scale_factors[8][51];

    pulse_info pul;
    tns_info tns;
    pred_info pred;
    ltp_info ltp;
    ltp_info ltp2;
} ic_stream; /* individual channel stream */

typedef struct
{
    int ele_id;

    int channel;
    int paired_channel;

    int element_instance_tag;
    int common_window;

    ic_stream ics1;
    ic_stream ics2;
} element; /* syntax element (SCE, CPE, LFE) */

int single_lfe_channel_element(element *sce, bitfile *ld, short *spec_data,
                               int sf_index, int object_type);
int channel_pair_element(element *cpe, bitfile *ld, short *spec_data1,
                         short *spec_data2, int sf_index, int object_type);
int data_stream_element(bitfile *ld);
int program_config_element(program_config *pce, bitfile *ld);
int fill_element(bitfile *ld, drc_info *drc);
int adts_frame(adts_header *adts, bitfile *ld);
void get_adif_header(adif_header *adif, bitfile *ld);


/* static functions */
static int individual_channel_stream(element *ele, bitfile *ld,
                                     ic_stream *ics, int scal_flag,
                                     short *spec_data, int sf_index,
                                     int object_type);
static int ics_info(ic_stream *ics, bitfile *ld,
                    int common_window, int fs_index, int object_type);
static void section_data(ic_stream *ics, bitfile *ld);
static int scale_factor_data(ic_stream *ics, bitfile *ld);
static int spectral_data(ic_stream *ics, bitfile *ld, short *spectral_data);
static int extension_payload(bitfile *ld, drc_info *drc, int count);
static void pulse_data(pulse_info *pul, bitfile *ld);
static void tns_data(ic_stream *ics, tns_info *tns, bitfile *ld);
static void ltp_data(ic_stream *ics, ltp_info *ltp, bitfile *ld);
static int adts_fixed_header(adts_header *adts, bitfile *ld);
static void adts_variable_header(adts_header *adts, bitfile *ld);
static void adts_error_check(adts_header *adts, bitfile *ld);
static int dynamic_range_info(bitfile *ld, drc_info *drc);
static int excluded_channels(bitfile *ld, drc_info *drc);


#ifdef __cplusplus
}
#endif
#endif
