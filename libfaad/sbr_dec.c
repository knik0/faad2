/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003 M. Bakker, Ahead Software AG, http://www.nero.com
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
** $Id: sbr_dec.c,v 1.18 2003/12/17 14:43:16 menno Exp $
**/


#include "common.h"
#include "structs.h"

#ifdef SBR_DEC

#include <string.h>
#include <stdlib.h>

#include "syntax.h"
#include "bits.h"
#include "sbr_syntax.h"
#include "sbr_qmf.h"
#include "sbr_hfgen.h"
#include "sbr_hfadj.h"


sbr_info *sbrDecodeInit(uint16_t framelength
#ifdef DRM
						, uint8_t IsDRM
#endif
                        )
{
    sbr_info *sbr = faad_malloc(sizeof(sbr_info));
    memset(sbr, 0, sizeof(sbr_info));

    sbr->bs_freq_scale = 2;
    sbr->bs_alter_scale = 1;
    sbr->bs_noise_bands = 2;
    sbr->bs_limiter_bands = 2;
    sbr->bs_limiter_gains = 2;
    sbr->bs_interpol_freq = 1;
    sbr->bs_smoothing_mode = 1;
    sbr->bs_start_freq = 5;
    sbr->bs_amp_res = 1;
    sbr->bs_samplerate_mode = 1;
    sbr->prevEnvIsShort[0] = -1;
    sbr->prevEnvIsShort[1] = -1;
    sbr->header_count = 0;

#ifdef DRM
    sbr->Is_DRM_SBR = IsDRM;
#endif
    sbr->bs_samplerate_mode = 1;
    sbr->tHFGen = T_HFGEN;
    sbr->tHFAdj = T_HFADJ;

    /* force sbr reset */
    sbr->bs_start_freq_prev = -1;

    if (framelength == 960)
    {
        sbr->numTimeSlotsRate = RATE * NO_TIME_SLOTS_960;
        sbr->numTimeSlots = NO_TIME_SLOTS_960;
    } else {
        sbr->numTimeSlotsRate = RATE * NO_TIME_SLOTS;
        sbr->numTimeSlots = NO_TIME_SLOTS;
    }

    return sbr;
}

void sbrDecodeEnd(sbr_info *sbr)
{
    uint8_t j;

    if (sbr)
    {
        qmfa_end(sbr->qmfa[0]);
        qmfs_end(sbr->qmfs[0]);
        if (sbr->id_aac == ID_CPE)
        {
            qmfa_end(sbr->qmfa[1]);
            qmfs_end(sbr->qmfs[1]);
        }

        for (j = 0; j < 5; j++)
        {
            if (sbr->G_temp_prev[0][j]) faad_free(sbr->G_temp_prev[0][j]);
            if (sbr->Q_temp_prev[0][j]) faad_free(sbr->Q_temp_prev[0][j]);
            if (sbr->G_temp_prev[1][j]) faad_free(sbr->G_temp_prev[1][j]);
            if (sbr->Q_temp_prev[1][j]) faad_free(sbr->Q_temp_prev[1][j]);
        }

        faad_free(sbr);
    }
}

void sbr_save_prev_data(sbr_info *sbr, uint8_t ch)
{
    uint8_t i;

    /* save data for next frame */
    sbr->kx_prev = sbr->kx;

    sbr->L_E_prev[ch] = sbr->L_E[ch];

    sbr->f_prev[ch] = sbr->f[ch][sbr->L_E[ch] - 1];
    for (i = 0; i < 64; i++)
    {
        sbr->E_prev[ch][i] = sbr->E[ch][i][sbr->L_E[ch] - 1];
        sbr->Q_prev[ch][i] = sbr->Q[ch][i][sbr->L_Q[ch] - 1];
    }

    for (i = 0; i < 64; i++)
    {
        sbr->bs_add_harmonic_prev[ch][i] = sbr->bs_add_harmonic[ch][i];
    }
    sbr->bs_add_harmonic_flag_prev[ch] = sbr->bs_add_harmonic_flag[ch];

    if (sbr->l_A[ch] == sbr->L_E[ch])
        sbr->prevEnvIsShort[ch] = 0;
    else
        sbr->prevEnvIsShort[ch] = -1;
}

void sbrDecodeFrame(sbr_info *sbr, real_t *left_channel,
                    real_t *right_channel,
                    const uint8_t just_seeked, const uint8_t upsample_only)
{
    int16_t i, k, l;

    uint8_t dont_process = 0;
    uint8_t ch, channels, ret;
    real_t *ch_buf;

    ALIGN qmf_t X[MAX_NTSR][64];
#ifdef SBR_LOW_POWER
    ALIGN real_t deg[64];
#endif

    bitfile *ld = NULL;

    channels = (sbr->id_aac == ID_SCE) ? 1 : 2;

    if (sbr->data == NULL || sbr->data_size == 0)
    {
        ret = 1;
    } else {
        ld = (bitfile*)faad_malloc(sizeof(bitfile));

        /* initialise and read the bitstream */
        faad_initbits(ld, sbr->data, sbr->data_size);

#ifdef DRM
        if (sbr->Is_DRM_SBR)
            faad_getbits(ld, 8); /* Skip 8-bit CRC */
#endif

        ret = sbr_extension_data(ld, sbr);

#ifdef DRM
        /* Check CRC */
        if (sbr->Is_DRM_SBR)
            ld->error = faad_check_CRC(ld, faad_get_processed_bits(ld) - 8);
#endif

        ret = ld->error ? ld->error : ret;
        faad_endbits(ld);
        if (ld) faad_free(ld);
        ld = NULL;
    }

    if (sbr->data) faad_free(sbr->data);
    sbr->data = NULL;

    if (ret || (sbr->header_count == 0))
    {
        /* don't process just upsample */
        dont_process = 1;

        /* Re-activate reset for next frame */
        if (ret && sbr->Reset)
            sbr->bs_start_freq_prev = -1;
    }

    if (just_seeked)
    {
        sbr->just_seeked = 1;
    } else {
        sbr->just_seeked = 0;
    }

    for (ch = 0; ch < channels; ch++)
    {
        if (sbr->frame == 0)
        {
            uint8_t j;
            sbr->qmfa[ch] = qmfa_init(32);
            sbr->qmfs[ch] = qmfs_init(64);

            for (j = 0; j < 5; j++)
            {
                sbr->G_temp_prev[ch][j] = faad_malloc(64*sizeof(real_t));
                sbr->Q_temp_prev[ch][j] = faad_malloc(64*sizeof(real_t));
            }

            memset(sbr->Xsbr[ch], 0, (sbr->numTimeSlotsRate+sbr->tHFGen)*64 * sizeof(qmf_t));
            memset(sbr->Xcodec[ch], 0, (sbr->numTimeSlotsRate+sbr->tHFGen)*32 * sizeof(qmf_t));
        }

        if (ch == 0)
            ch_buf = left_channel;
        else
            ch_buf = right_channel;

        /* subband analysis */
        if (dont_process)
            sbr_qmf_analysis_32(sbr, sbr->qmfa[ch], ch_buf, sbr->Xcodec[ch], sbr->tHFGen, 32);
        else
            sbr_qmf_analysis_32(sbr, sbr->qmfa[ch], ch_buf, sbr->Xcodec[ch], sbr->tHFGen, sbr->kx);

        if (!dont_process)
        {
#if 1
            /* insert high frequencies here */
            /* hf generation using patching */
            hf_generation(sbr, sbr->Xcodec[ch], sbr->Xsbr[ch]
#ifdef SBR_LOW_POWER
                ,deg
#endif
                ,ch);
#endif

#ifdef SBR_LOW_POWER
            for (l = sbr->t_E[ch][0]; l < sbr->t_E[ch][sbr->L_E[ch]]; l++)
            {
                for (k = 0; k < sbr->kx; k++)
                {
                    QMF_RE(sbr->Xsbr[ch][sbr->tHFAdj + l][k]) = 0;
                }
            }
#endif

#if 1
            /* hf adjustment */
            hf_adjustment(sbr, sbr->Xsbr[ch]
#ifdef SBR_LOW_POWER
                ,deg
#endif
                ,ch);
#endif
        }

        if ((sbr->just_seeked != 0) || dont_process)
        {
            for (l = 0; l < sbr->numTimeSlotsRate; l++)
            {
                for (k = 0; k < 32; k++)
                {
                    QMF_RE(X[l][k]) = QMF_RE(sbr->Xcodec[ch][l + sbr->tHFAdj][k]);
#ifndef SBR_LOW_POWER
                    QMF_IM(X[l][k]) = QMF_IM(sbr->Xcodec[ch][l + sbr->tHFAdj][k]);
#endif
                }
                for (k = 32; k < 64; k++)
                {
                    QMF_RE(X[l][k]) = 0;
#ifndef SBR_LOW_POWER
                    QMF_IM(X[l][k]) = 0;
#endif
                }
            }
        } else {
            for (l = 0; l < sbr->numTimeSlotsRate; l++)
            {
                uint8_t xover_band;

                if (l < sbr->t_E[ch][0])
                    xover_band = sbr->kx_prev;
                else
                    xover_band = sbr->kx;

                for (k = 0; k < xover_band; k++)
                {
                    QMF_RE(X[l][k]) = QMF_RE(sbr->Xcodec[ch][l + sbr->tHFAdj][k]);
#ifndef SBR_LOW_POWER
                    QMF_IM(X[l][k]) = QMF_IM(sbr->Xcodec[ch][l + sbr->tHFAdj][k]);
#endif
                }
                for (k = xover_band; k < 64; k++)
                {
                    QMF_RE(X[l][k]) = QMF_RE(sbr->Xsbr[ch][l + sbr->tHFAdj][k]);
#ifndef SBR_LOW_POWER
                    QMF_IM(X[l][k]) = QMF_IM(sbr->Xsbr[ch][l + sbr->tHFAdj][k]);
#endif
                }
#ifdef SBR_LOW_POWER
                QMF_RE(X[l][xover_band - 1]) += QMF_RE(sbr->Xsbr[ch][l + sbr->tHFAdj][xover_band - 1]);
#endif
            }
        }

        /* subband synthesis */
#ifndef USE_SSE
        sbr_qmf_synthesis_64(sbr, sbr->qmfs[ch], X, ch_buf);
#else
        sbr->qmfs[ch]->qmf_func(sbr, sbr->qmfs[ch], X, ch_buf);
#endif

        for (i = 0; i < sbr->tHFGen; i++)
        {
            memmove(sbr->Xcodec[ch][i], sbr->Xcodec[ch][i+sbr->numTimeSlotsRate], 32 * sizeof(qmf_t));
            memmove(sbr->Xsbr[ch][i], sbr->Xsbr[ch][i+sbr->numTimeSlotsRate], 64 * sizeof(qmf_t));
        }
    }

    if (sbr->bs_header_flag)
        sbr->just_seeked = 0;

    if (sbr->header_count != 0)
    {
        for (ch = 0; ch < channels; ch++)
            sbr_save_prev_data(sbr, ch);
    }

    sbr->frame++;
}

#endif
