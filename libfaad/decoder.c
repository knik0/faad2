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
** $Id: decoder.c,v 1.19 2002/08/17 10:03:12 menno Exp $
**/

#include <stdlib.h>
#include <memory.h>
#include "common.h"
#include "decoder.h"
#include "mp4.h"
#include "syntax.h"
#include "specrec.h"
#include "data.h"
#include "tns.h"
#include "pns.h"
#include "is.h"
#include "ms.h"
#include "ic_predict.h"
#include "lt_predict.h"
#include "drc.h"
#include "error.h"
#include "output.h"

#ifdef ANALYSIS
uint16_t dbg_count;
#endif

uint8_t* FAADAPI faacDecGetErrorMessage(uint8_t errcode)
{
    return err_msg[errcode];
}

faacDecHandle FAADAPI faacDecOpen()
{
    uint8_t i;
    faacDecHandle hDecoder = NULL;

    if ((hDecoder = (faacDecHandle)malloc(sizeof(faacDecStruct))) == NULL)
        return NULL;

    memset(hDecoder, 0, sizeof(faacDecStruct));

    hDecoder->config.outputFormat  = FAAD_FMT_16BIT;
    hDecoder->config.defObjectType = MAIN;
    hDecoder->config.defSampleRate = 44100; /* Default: 44.1kHz */
    hDecoder->adts_header_present = 0;
    hDecoder->adif_header_present = 0;
    hDecoder->aacSectionDataResilienceFlag = 0;
    hDecoder->aacScalefactorDataResilienceFlag = 0;
    hDecoder->aacSpectralDataResilienceFlag = 0;
    hDecoder->frameLength = 1024;

    hDecoder->frame = 0;
    hDecoder->sample_buffer = NULL;

    for (i = 0; i < MAX_CHANNELS; i++)
    {
        hDecoder->window_shape_prev[i] = 0;
        hDecoder->time_state[i] = NULL;
        hDecoder->time_out[i] = NULL;
#ifdef MAIN_DEC
        hDecoder->pred_stat[i] = NULL;
#endif
#ifdef LTP_DEC
        hDecoder->ltp_lag[i] = 0;
        hDecoder->lt_pred_stat[i] = NULL;
#endif
    }

    hDecoder->drc = drc_init(1.0f, 1.0f);
#if IQ_TABLE_SIZE && POW_TABLE_SIZE
    build_tables(hDecoder->iq_table, hDecoder->pow2_table);
#elif !POW_TABLE_SIZE
    build_tables(hDecoder->iq_table, NULL);
#endif

    return hDecoder;
}

faacDecConfigurationPtr FAADAPI faacDecGetCurrentConfiguration(faacDecHandle hDecoder)
{
    faacDecConfigurationPtr config = &(hDecoder->config);

    return config;
}

uint8_t FAADAPI faacDecSetConfiguration(faacDecHandle hDecoder,
                                    faacDecConfigurationPtr config)
{
    hDecoder->config.defObjectType = config->defObjectType;
#ifdef DRM
    if (config->defObjectType == DRM_ER_LC)
    {
        hDecoder->aacSectionDataResilienceFlag = 1; /* VCB11 */
        hDecoder->aacScalefactorDataResilienceFlag = 0; /* no RVLC */
        hDecoder->aacSpectralDataResilienceFlag = 1; /* HCR */
        hDecoder->frameLength = 960;
    }
#endif
    hDecoder->config.defSampleRate = config->defSampleRate;
    hDecoder->config.outputFormat  = config->outputFormat;

    /* OK */
    return 1;
}

/* Returns the sample rate index */
static uint8_t get_sr_index(uint32_t samplerate)
{
    if (92017 <= samplerate) return 0;
    if (75132 <= samplerate) return 1;
    if (55426 <= samplerate) return 2;
    if (46009 <= samplerate) return 3;
    if (37566 <= samplerate) return 4;
    if (27713 <= samplerate) return 5;
    if (23004 <= samplerate) return 6;
    if (18783 <= samplerate) return 7;
    if (13856 <= samplerate) return 8;
    if (11502 <= samplerate) return 9;
    if (9391 <= samplerate) return 10;

    return 11;
}

/* Returns 0 if an object type is decodable, otherwise returns -1 */
static int8_t can_decode_ot(uint8_t object_type)
{
    switch (object_type)
    {
    case LC:
        return 0;
    case MAIN:
#ifdef MAIN_DEC
        return 0;
#else
        return -1;
#endif
    case SSR:
        return -1;
    case LTP:
#ifdef LTP_DEC
        return 0;
#else
        return -1;
#endif

    /* ER object types */
#ifdef ERROR_RESILIENCE
    case ER_LC:
#ifdef DRM
    case DRM_ER_LC:
#endif
        return 0;
    case ER_LTP:
#ifdef LTP_DEC
        return 0;
#else
        return -1;
#endif
    case LD:
#ifdef LD_DEC
        return 0;
#else
        return -1;
#endif
#endif
    }

    return -1;
}

int32_t FAADAPI faacDecInit(faacDecHandle hDecoder, uint8_t *buffer,
                        uint32_t *samplerate, uint8_t *channels)
{
    uint32_t bits = 0;
    bitfile ld;
    adif_header adif;
    adts_header adts;

    hDecoder->sf_index = get_sr_index(hDecoder->config.defSampleRate);
    hDecoder->object_type = hDecoder->config.defObjectType;
    *samplerate = sample_rates[hDecoder->sf_index];
    *channels = 1;

    if (buffer != NULL)
    {
        faad_initbits(&ld, buffer);

#ifdef DRM
        if (hDecoder->object_type != DRM_ER_LC)
#endif        
        /* Check if an ADIF header is present */
        if ((buffer[0] == 'A') && (buffer[1] == 'D') &&
            (buffer[2] == 'I') && (buffer[3] == 'F'))
        {
            hDecoder->adif_header_present = 1;

            get_adif_header(&adif, &ld);

            hDecoder->sf_index = adif.pce.sf_index;
            hDecoder->object_type = adif.pce.object_type;

            *samplerate = sample_rates[hDecoder->sf_index];
            *channels = adif.pce.channels;

            bits = bit2byte(faad_get_processed_bits(&ld));

        /* Check if an ADTS header is present */
        } else if (faad_showbits(&ld, 12) == 0xfff) {
            hDecoder->adts_header_present = 1;

            adts_frame(&adts, &ld);

            hDecoder->sf_index = adts.sf_index;
            hDecoder->object_type = adts.profile;

            *samplerate = sample_rates[hDecoder->sf_index];
            *channels = (adts.channel_configuration > 6) ?
                2 : adts.channel_configuration;
        }
    }
    hDecoder->channelConfiguration = *channels;

    /* must be done before frameLength is divided by 2 for LD */
    hDecoder->fb = filter_bank_init(hDecoder->frameLength);

#ifdef LD_DEC
    if (hDecoder->object_type == LD)
        hDecoder->frameLength >>= 1;
#endif

    if (can_decode_ot(hDecoder->object_type) < 0)
        return -1;

    return bits;
}

/* Init the library using a DecoderSpecificInfo */
int8_t FAADAPI faacDecInit2(faacDecHandle hDecoder, uint8_t *pBuffer,
                            uint32_t SizeOfDecoderSpecificInfo,
                            uint32_t *samplerate, uint8_t *channels)
{
    int8_t rc;
    uint8_t frameLengthFlag;

    hDecoder->adif_header_present = 0;
    hDecoder->adts_header_present = 0;

    if((hDecoder == NULL)
        || (pBuffer == NULL)
        || (SizeOfDecoderSpecificInfo < 2)
        || (samplerate == NULL)
        || (channels == NULL))
    {
        return -1;
    }

    rc = AudioSpecificConfig(pBuffer, samplerate, channels,
        &hDecoder->sf_index, &hDecoder->object_type,
        &hDecoder->aacSectionDataResilienceFlag,
        &hDecoder->aacScalefactorDataResilienceFlag,
        &hDecoder->aacSpectralDataResilienceFlag,
        &frameLengthFlag);
    if (hDecoder->object_type < 4)
        hDecoder->object_type--; /* For AAC differs from MPEG-4 */
    if (rc != 0)
    {
        return rc;
    }
    hDecoder->channelConfiguration = *channels;
    if (frameLengthFlag)
        hDecoder->frameLength = 960;

    /* must be done before frameLength is divided by 2 for LD */
    hDecoder->fb = filter_bank_init(hDecoder->frameLength);

#ifdef LD_DEC
    if (hDecoder->object_type == LD)
        hDecoder->frameLength >>= 1;
#endif

    return 0;
}

void FAADAPI faacDecClose(faacDecHandle hDecoder)
{
    uint8_t i;

    for (i = 0; i < MAX_CHANNELS; i++)
    {
        if (hDecoder->time_state[i]) free(hDecoder->time_state[i]);
        if (hDecoder->time_out[i]) free(hDecoder->time_out[i]);
#ifdef MAIN_DEC
        if (hDecoder->pred_stat[i]) free(hDecoder->pred_stat[i]);
#endif
#ifdef LTP_DEC
        if (hDecoder->lt_pred_stat[i]) free(hDecoder->lt_pred_stat[i]);
#endif
    }

    filter_bank_end(hDecoder->fb);

    drc_end(hDecoder->drc);

    if (hDecoder->sample_buffer) free(hDecoder->sample_buffer);

    if (hDecoder) free(hDecoder);
}

#ifdef ERROR_RESILIENCE
#define decode_sce_lfe() \
    spec_data[channels]   = (int16_t*)malloc(frame_len*sizeof(int16_t)); \
    spec_coef[channels]   = (real_t*)malloc(frame_len*sizeof(real_t)); \
 \
    syntax_elements[ch_ele] = (element*)malloc(sizeof(element)); \
    memset(syntax_elements[ch_ele], 0, sizeof(element)); \
    syntax_elements[ch_ele]->ele_id  = id_syn_ele; \
    syntax_elements[ch_ele]->channel = channels; \
    syntax_elements[ch_ele]->paired_channel = -1; \
 \
    if ((hInfo->error = single_lfe_channel_element(syntax_elements[ch_ele], \
        ld, spec_data[channels], sf_index, object_type, frame_len, \
        aacSectionDataResilienceFlag, aacScalefactorDataResilienceFlag, \
        aacSpectralDataResilienceFlag)) > 0) \
    { \
        /* to make sure everything gets deallocated */ \
        channels++; ch_ele++; \
        goto error; \
    } \
 \
    channels++; \
    ch_ele++;
#else
#define decode_sce_lfe() \
    spec_data[channels]   = (int16_t*)malloc(frame_len*sizeof(int16_t)); \
    spec_coef[channels]   = (real_t*)malloc(frame_len*sizeof(real_t)); \
 \
    syntax_elements[ch_ele] = (element*)malloc(sizeof(element)); \
    memset(syntax_elements[ch_ele], 0, sizeof(element)); \
    syntax_elements[ch_ele]->ele_id  = id_syn_ele; \
    syntax_elements[ch_ele]->channel = channels; \
    syntax_elements[ch_ele]->paired_channel = -1; \
 \
    if ((hInfo->error = single_lfe_channel_element(syntax_elements[ch_ele], \
        ld, spec_data[channels], sf_index, object_type, frame_len)) > 0) \
    { \
        /* to make sure everything gets deallocated */ \
        channels++; ch_ele++; \
        goto error; \
    } \
 \
    channels++; \
    ch_ele++;
#endif

#ifdef ERROR_RESILIENCE
#define decode_cpe() \
    spec_data[channels]   = (int16_t*)malloc(frame_len*sizeof(int16_t)); \
    spec_data[channels+1] = (int16_t*)malloc(frame_len*sizeof(int16_t)); \
    spec_coef[channels]   = (real_t*)malloc(frame_len*sizeof(real_t)); \
    spec_coef[channels+1] = (real_t*)malloc(frame_len*sizeof(real_t)); \
 \
    syntax_elements[ch_ele] = (element*)malloc(sizeof(element)); \
    memset(syntax_elements[ch_ele], 0, sizeof(element)); \
    syntax_elements[ch_ele]->ele_id         = id_syn_ele; \
    syntax_elements[ch_ele]->channel        = channels; \
    syntax_elements[ch_ele]->paired_channel = channels+1; \
 \
    if ((hInfo->error = channel_pair_element(syntax_elements[ch_ele], \
        ld, spec_data[channels], spec_data[channels+1], \
        sf_index, object_type, frame_len, \
        aacSectionDataResilienceFlag, aacScalefactorDataResilienceFlag, \
        aacSpectralDataResilienceFlag)) > 0) \
    { \
        /* to make sure everything gets deallocated */ \
        channels+=2; ch_ele++; \
        goto error; \
    } \
 \
    channels += 2; \
    ch_ele++;
#else
#define decode_cpe() \
    spec_data[channels]   = (int16_t*)malloc(frame_len*sizeof(int16_t)); \
    spec_data[channels+1] = (int16_t*)malloc(frame_len*sizeof(int16_t)); \
    spec_coef[channels]   = (real_t*)malloc(frame_len*sizeof(real_t)); \
    spec_coef[channels+1] = (real_t*)malloc(frame_len*sizeof(real_t)); \
 \
    syntax_elements[ch_ele] = (element*)malloc(sizeof(element)); \
    memset(syntax_elements[ch_ele], 0, sizeof(element)); \
    syntax_elements[ch_ele]->ele_id         = id_syn_ele; \
    syntax_elements[ch_ele]->channel        = channels; \
    syntax_elements[ch_ele]->paired_channel = channels+1; \
 \
    if ((hInfo->error = channel_pair_element(syntax_elements[ch_ele], \
        ld, spec_data[channels], spec_data[channels+1], \
        sf_index, object_type, frame_len)) > 0) \
    { \
        /* to make sure everything gets deallocated */ \
        channels+=2; ch_ele++; \
        goto error; \
    } \
 \
    channels += 2; \
    ch_ele++;
#endif

void* FAADAPI faacDecDecode(faacDecHandle hDecoder,
                            faacDecFrameInfo *hInfo,
                            uint8_t *buffer)
{
    int32_t i;
    uint8_t id_syn_ele, ele, ch;
    adts_header adts;
    uint8_t channels, ch_ele;
    bitfile *ld = malloc(sizeof(bitfile));

    /* local copys of globals */
    uint8_t sf_index       =  hDecoder->sf_index;
    uint8_t object_type    =  hDecoder->object_type;
    uint8_t channelConfiguration = hDecoder->channelConfiguration;
#ifdef MAIN_DEC
    pred_state **pred_stat =  hDecoder->pred_stat;
#endif
#ifdef LTP_DEC
    real_t **lt_pred_stat  =  hDecoder->lt_pred_stat;
#endif
    real_t *iq_table       =  hDecoder->iq_table;
#if POW_TABLE_SIZE
    real_t *pow2_table     =  hDecoder->pow2_table;
#else
    real_t *pow2_table     =  NULL;
#endif
    uint8_t *window_shape_prev = hDecoder->window_shape_prev;
    real_t **time_state    =  hDecoder->time_state;
    real_t **time_out      =  hDecoder->time_out;
    fb_info *fb            =  hDecoder->fb;
    drc_info *drc          =  hDecoder->drc;
    uint8_t outputFormat   =  hDecoder->config.outputFormat;
#ifdef LTP_DEC
    uint16_t *ltp_lag      =  hDecoder->ltp_lag;
#endif
#ifdef ERROR_RESILIENCE
    uint8_t aacSectionDataResilienceFlag     = hDecoder->aacSectionDataResilienceFlag;
    uint8_t aacScalefactorDataResilienceFlag = hDecoder->aacScalefactorDataResilienceFlag;
    uint8_t aacSpectralDataResilienceFlag    = hDecoder->aacSpectralDataResilienceFlag;
#endif

    program_config pce;
    element *syntax_elements[MAX_SYNTAX_ELEMENTS];
    int16_t *spec_data[MAX_CHANNELS];
    real_t *spec_coef[MAX_CHANNELS];

    /* frame length is different for Low Delay AAC */
    uint16_t frame_len = hDecoder->frameLength;

    void *sample_buffer;

    ele = 0;
    channels = 0;
    ch_ele = 0;

    memset(hInfo, 0, sizeof(faacDecFrameInfo));

    /* initialize the bitstream */
    faad_initbits(ld, buffer);

    if (hDecoder->adts_header_present)
    {
        if ((hInfo->error = adts_frame(&adts, ld)) > 0)
            goto error;

        /* MPEG2 does byte_alignment() here,
         * but ADTS header is always multiple of 8 bits in MPEG2
         * so not needed to actually do it.
         */
    }

#ifdef ANALYSIS
    dbg_count = 0;
#endif

#ifdef ERROR_RESILIENCE
    if (object_type < ER_OBJECT_START)
    {
#endif
        /* Table 4.4.3: raw_data_block() */
        while ((id_syn_ele = (uint8_t)faad_getbits(ld, LEN_SE_ID
            DEBUGVAR(1,4,"faacDecDecode(): id_syn_ele"))) != ID_END)
        {
            switch (id_syn_ele) {
            case ID_SCE:
            case ID_LFE:
                decode_sce_lfe();
                break;
            case ID_CPE:
                decode_cpe();
                break;
            case ID_CCE: /* not implemented yet */
                hInfo->error = 6;
                goto error;
                break;
            case ID_DSE:
                data_stream_element(ld);
                break;
            case ID_PCE:
                if ((hInfo->error = program_config_element(&pce, ld)) > 0)
                    goto error;
                break;
            case ID_FIL:
                if ((hInfo->error = fill_element(ld, drc)) > 0)
                    goto error;
                break;
            }
            ele++;
        }
#ifdef ERROR_RESILIENCE
    } else {
        /* Table 262: er_raw_data_block() */
        switch (channelConfiguration)
        {
        case 1:
            id_syn_ele = ID_SCE;
            decode_sce_lfe();
            break;
        case 2:
            id_syn_ele = ID_CPE;
            decode_cpe();
            break;
        case 3:
            id_syn_ele = ID_SCE;
            decode_sce_lfe();
            id_syn_ele = ID_CPE;
            decode_cpe();
            break;
        case 4:
            id_syn_ele = ID_SCE;
            decode_sce_lfe();
            id_syn_ele = ID_CPE;
            decode_cpe();
            id_syn_ele = ID_SCE;
            decode_sce_lfe();
            break;
        case 5:
            id_syn_ele = ID_SCE;
            decode_sce_lfe();
            id_syn_ele = ID_CPE;
            decode_cpe();
            id_syn_ele = ID_CPE;
            decode_cpe();
            break;
        case 6:
            id_syn_ele = ID_SCE;
            decode_sce_lfe();
            id_syn_ele = ID_CPE;
            decode_cpe();
            id_syn_ele = ID_CPE;
            decode_cpe();
            id_syn_ele = ID_LFE;
            decode_sce_lfe();
            break;
        case 7:
            id_syn_ele = ID_SCE;
            decode_sce_lfe();
            id_syn_ele = ID_CPE;
            decode_cpe();
            id_syn_ele = ID_CPE;
            decode_cpe();
            id_syn_ele = ID_CPE;
            decode_cpe();
            id_syn_ele = ID_LFE;
            decode_sce_lfe();
            break;
        default:
            hInfo->error = 7;
            goto error;
        }
#if 0
        cnt = bits_to_decode() / 8;
        while (cnt >= 1)
        {
            cnt -= extension_payload(cnt);
        }
#endif
    }
#endif

    /* no more bit reading after this */
    faad_byte_align(ld);
    hInfo->bytesconsumed = bit2byte(faad_get_processed_bits(ld));
    if (ld) free(ld);
    ld = NULL;

    /* number of samples in this frame */
    hInfo->samples = frame_len*channels;
    /* number of samples in this frame */
    hInfo->channels = channels;

    if (hDecoder->sample_buffer == NULL)
        hDecoder->sample_buffer = malloc(frame_len*channels*sizeof(float32_t));

    sample_buffer = hDecoder->sample_buffer;

    /* noiseless coding is done, the rest of the tools come now */
    for (ch = 0; ch < channels; ch++)
    {
        ic_stream *ics;

        /* find the syntax element to which this channel belongs */
        for (i = 0; i < ch_ele; i++)
        {
            if (syntax_elements[i]->channel == ch)
                ics = &(syntax_elements[i]->ics1);
            else if (syntax_elements[i]->paired_channel == ch)
                ics = &(syntax_elements[i]->ics2);
        }

        /* inverse quantization */
        inverse_quantization(spec_coef[ch], spec_data[ch], iq_table,
            frame_len);

        /* apply scalefactors */
        apply_scalefactors(ics, spec_coef[ch], pow2_table, frame_len);

        /* deinterleave short block grouping */
        if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
            quant_to_spec(ics, spec_coef[ch], frame_len);
    }

    /* Because for ms and is both channels spectral coefficients are needed
       we have to restart running through all channels here.
    */
    for (ch = 0; ch < channels; ch++)
    {
        uint8_t pch = 0;
        uint8_t right_channel;
        ic_stream *ics, *icsr;
        ltp_info *ltp;

        /* find the syntax element to which this channel belongs */
        for (i = 0; i < ch_ele; i++)
        {
            if (syntax_elements[i]->channel == ch)
            {
                ics = &(syntax_elements[i]->ics1);
                icsr = &(syntax_elements[i]->ics2);
                ltp = &(ics->ltp);
                pch = syntax_elements[i]->paired_channel;
                right_channel = 0;
            } else if (syntax_elements[i]->paired_channel == ch) {
                ics = &(syntax_elements[i]->ics2);
                ltp = &(ics->ltp2);
                right_channel = 1;
            }
        }

        /* mid/side decoding */
        if (!right_channel)
            ms_decode(ics, icsr, spec_coef[ch], spec_coef[pch], frame_len);

        /* pns decoding */
        pns_decode(ics, spec_coef[ch], frame_len);

        /* intensity stereo decoding */
        if (!right_channel)
            is_decode(ics, icsr, spec_coef[ch], spec_coef[pch], frame_len);

#ifdef MAIN_DEC
        /* MAIN object type prediction */
        if (object_type == MAIN)
        {
            /* allocate the state only when needed */
            if (pred_stat[ch] == NULL)
            {
                pred_stat[ch] = malloc(frame_len * sizeof(pred_state));
                reset_all_predictors(pred_stat[ch], frame_len);
            }

            /* intra channel prediction */
            ic_prediction(ics, spec_coef[ch], pred_stat[ch], frame_len);

            /* In addition, for scalefactor bands coded by perceptual
               noise substitution the predictors belonging to the
               corresponding spectral coefficients are reset.
            */
            pns_reset_pred_state(ics, pred_stat[ch]);
        }
#endif
#ifdef LTP_DEC
        else if ((object_type == LTP)
#ifdef ERROR_RESILIENCE
            || (object_type == ER_LTP)
#endif
#ifdef LD_DEC
            || (object_type == LD)
#endif
            )
        {
#ifdef LD_DEC
            if (object_type == LD)
            {
                if (ltp->data_present)
                {
                    if (!ltp->lag_update)
                        ltp->lag = ltp_lag[ch];
                    else
                        ltp_lag[ch] = ltp->lag;
                }
            }
#endif

            /* allocate the state only when needed */
            if (lt_pred_stat[ch] == NULL)
            {
                lt_pred_stat[ch] = malloc(frame_len*4 * sizeof(real_t));
                memset(lt_pred_stat[ch], 0, frame_len*4 * sizeof(real_t));
            }

            /* long term prediction */
            lt_prediction(ics, ltp, spec_coef[ch], lt_pred_stat[ch], fb,
                ics->window_shape, window_shape_prev[ch],
                sf_index, object_type, frame_len);
        }
#endif

        /* tns decoding */
        tns_decode_frame(ics, &(ics->tns), sf_index, object_type,
            spec_coef[ch], frame_len);

        /* drc decoding */
        if (drc->present)
        {
            if (!drc->exclude_mask[ch] || !drc->excluded_chns_present)
                drc_decode(drc, spec_coef[ch]);
        }

        if (time_state[ch] == NULL)
        {
            real_t *tp;

            time_state[ch] = malloc(frame_len*sizeof(real_t));
            tp = time_state[ch];
            for (i = frame_len/16-1; i >= 0; --i)
            {
                *tp++ = 0; *tp++ = 0; *tp++ = 0; *tp++ = 0;
                *tp++ = 0; *tp++ = 0; *tp++ = 0; *tp++ = 0;
                *tp++ = 0; *tp++ = 0; *tp++ = 0; *tp++ = 0;
                *tp++ = 0; *tp++ = 0; *tp++ = 0; *tp++ = 0;
            }
        }
        if (time_out[ch] == NULL)
        {
            time_out[ch] = malloc(frame_len*2*sizeof(real_t));
        }

        /* filter bank */
        ifilter_bank(fb, ics->window_sequence, ics->window_shape,
            window_shape_prev[ch], spec_coef[ch], time_state[ch],
            time_out[ch], object_type, frame_len);
        /* save window shape for next frame */
        window_shape_prev[ch] = ics->window_shape;

#ifdef LTP_DEC
        if ((object_type == LTP)
#ifdef ERROR_RESILIENCE
            || (object_type == ER_LTP)
#endif
#ifdef LD_DEC
            || (object_type == LD)
#endif
            )
        {
            lt_update_state(lt_pred_stat[ch], time_out[ch], time_state[ch],
                frame_len, object_type);
        }
#endif
    }

    sample_buffer = output_to_PCM(time_out, sample_buffer, channels,
        frame_len, outputFormat);

    hDecoder->frame++;
#ifdef LD_DEC
    if (object_type != LD)
    {
#endif
        if (hDecoder->frame <= 1)
            hInfo->samples = 0;
#ifdef LD_DEC
    } else {
        /* LD encoders will give lower delay */
        if (hDecoder->frame <= 0)
            hInfo->samples = 0;
    }
#endif

    /* cleanup */
    for (ch = 0; ch < channels; ch++)
    {
        free(spec_coef[ch]);
        free(spec_data[ch]);
    }

    for (i = 0; i < ch_ele; i++)
    {
        free(syntax_elements[i]);
    }

#ifdef ANALYSIS
    fflush(stdout);
#endif

    return sample_buffer;

error:
    /* free all memory that could have been allocated */
    if (ld) free(ld);

    /* cleanup */
    for (ch = 0; ch < channels; ch++)
    {
        free(spec_coef[ch]);
        free(spec_data[ch]);
    }

    for (i = 0; i < ch_ele; i++)
    {
        free(syntax_elements[i]);
    }

#ifdef ANALYSIS
    fflush(stdout);
#endif

    return NULL;
}
