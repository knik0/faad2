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
** $Id: decoder.h,v 1.17 2002/11/01 11:19:35 menno Exp $
**/

#ifndef __DECODER_H__
#define __DECODER_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
  #pragma pack(push, 8)
  #ifndef FAADAPI
    #define FAADAPI __cdecl
  #endif
#else
  #ifndef FAADAPI
    #define FAADAPI
  #endif
#endif

#include "bits.h"
#include "syntax.h"
#include "specrec.h"
#include "filtbank.h"
#include "ic_predict.h"


#define FAAD_FMT_16BIT 1
#define FAAD_FMT_24BIT 2
#define FAAD_FMT_32BIT 3
#define FAAD_FMT_FLOAT 4
#define FAAD_FMT_16BIT_DITHER 5
#define FAAD_FMT_16BIT_L_SHAPE 6
#define FAAD_FMT_16BIT_M_SHAPE 7
#define FAAD_FMT_16BIT_H_SHAPE 8

typedef struct faacDecConfiguration
{
    uint8_t defObjectType;
    uint32_t defSampleRate;
    uint8_t outputFormat;
} faacDecConfiguration, *faacDecConfigurationPtr;

typedef struct faacDecFrameInfo
{
    uint32_t bytesconsumed;
    uint32_t samples;
    uint8_t channels;
    uint8_t error;
} faacDecFrameInfo;

typedef struct
{
    uint8_t adts_header_present;
    uint8_t adif_header_present;
    uint8_t sf_index;
    uint8_t object_type;
    uint8_t channelConfiguration;
#ifdef ERROR_RESILIENCE
    uint8_t aacSectionDataResilienceFlag;
    uint8_t aacScalefactorDataResilienceFlag;
    uint8_t aacSpectralDataResilienceFlag;
#endif
    uint16_t frameLength;

    uint32_t frame;

    void *sample_buffer;

    uint8_t window_shape_prev[MAX_CHANNELS];
#ifdef LTP_DEC
    uint16_t ltp_lag[MAX_CHANNELS];
#endif
    fb_info *fb;
    drc_info *drc;

    real_t *time_out[MAX_CHANNELS];

#ifdef MAIN_DEC
    pred_state *pred_stat[MAX_CHANNELS];
#endif
#ifdef LTP_DEC
    real_t *lt_pred_stat[MAX_CHANNELS];
#endif

#ifndef FIXED_POINT
#if POW_TABLE_SIZE
    real_t *pow2_table;
#endif
#endif

    /* Configuration data */
    faacDecConfiguration config;
} faacDecStruct, *faacDecHandle;


int8_t* FAADAPI faacDecGetErrorMessage(uint8_t errcode);

faacDecHandle FAADAPI faacDecOpen();

faacDecConfigurationPtr FAADAPI faacDecGetCurrentConfiguration(faacDecHandle hDecoder);

uint8_t FAADAPI faacDecSetConfiguration(faacDecHandle hDecoder,
                                    faacDecConfigurationPtr config);

/* Init the library based on info from the AAC file (ADTS/ADIF) */
int32_t FAADAPI faacDecInit(faacDecHandle hDecoder,
                            uint8_t *buffer,
                            uint32_t buffer_size,
                            uint32_t *samplerate,
                            uint8_t *channels);

/* Init the library using a DecoderSpecificInfo */
int8_t FAADAPI faacDecInit2(faacDecHandle hDecoder, uint8_t *pBuffer,
                         uint32_t SizeOfDecoderSpecificInfo,
                         uint32_t *samplerate, uint8_t *channels);

void FAADAPI faacDecClose(faacDecHandle hDecoder);

void* FAADAPI faacDecDecode(faacDecHandle hDecoder,
                            faacDecFrameInfo *hInfo,
                            uint8_t *buffer,
                            uint32_t buffer_size);

/* these functions are in syntax.c */
element *decode_sce_lfe(faacDecHandle hDecoder,
                        faacDecFrameInfo *hInfo, bitfile *ld,
                        int16_t **spec_data, real_t **spec_coef,
                        uint8_t channels, uint8_t id_syn_ele);
element *decode_cpe(faacDecHandle hDecoder,
                    faacDecFrameInfo *hInfo, bitfile *ld,
                    int16_t **spec_data, real_t **spec_coef,
                    uint8_t channels, uint8_t id_syn_ele);
element **raw_data_block(faacDecHandle hDecoder, faacDecFrameInfo *hInfo,
                         bitfile *ld, element **elements,
                         int16_t **spec_data, real_t **spec_coef,
                         uint8_t *out_ch_ele, uint8_t *out_channels,
                         program_config *pce, drc_info *drc);

#ifdef _WIN32
  #pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif
#endif
