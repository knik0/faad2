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
** $Id: sbr_syntax.c,v 1.2 2002/04/20 22:20:15 menno Exp $
**/

/*
   This is the initial support for MPEG-4 AAC+SBR

   All data is taken from the Working Draft Text for Backward Compatible
   Bandwidth Extension for General Audio Coding (N4611)
*/

/*
    Mind the sbr_extension() function, it is not defined in the text
    obviously it just reads some bytes.
 */


#include "common.h"

#ifdef SBR

#include "sbr_syntax.h"
#include "syntax.h"
#include "sbr_huff.h"
#include "bits.h"
#include "analysis.h"

/* table 2 */
uint8_t sbr_bitstream(bitfile *ld, sbr_info *sbr, uint8_t id_aac,
                      uint8_t bs_extension_type)
{
    sbr->bs_crc_flag = faad_get1bit(ld
        DEBUGVAR(1,200,"sbr_bitstream(): bs_crc_flag"));
    if (sbr->bs_crc_flag)
    {
        sbr->bs_sbr_crc_bits = faad_getbits(ld, 7
            DEBUGVAR(1,201,"sbr_bitstream(): bs_sbr_crc_bits"));
    }

    if (bs_extension_type == SBR_HDR)
    {
        sbr_header(ld, sbr, id_aac);
        sbr_data(ld, sbr, id_aac);
    } else if (bs_extension_type == SBR_STD) {
        sbr_data(ld, sbr, id_aac);
    }
}

/* table 3 */
static void sbr_header(bitfile *ld, sbr_info *sbr, uint8_t id_aac)
{
    uint8_t bs_header_extra_1, bs_header_extra_2;

    sbr->bs_protocol_version = faad_getbits(ld, 2
        DEBUGVAR(1,202,"sbr_header(): bs_protocol_version"));
    sbr->bs_amp_res = faad_get1bit(ld
        DEBUGVAR(1,203,"sbr_header(): bs_amp_res"));
    sbr->bs_start_freq = faad_getbits(ld, 4
        DEBUGVAR(1,204,"sbr_header(): bs_start_freq"));
    sbr->bs_stop_freq = faad_getbits(ld, 4
        DEBUGVAR(1,205,"sbr_header(): bs_stop_freq"));
    sbr->bs_xover_band = faad_getbits(ld, 3
        DEBUGVAR(1,206,"sbr_header(): bs_xover_band"));
    faad_getbits(ld, 3
        DEBUGVAR(1,207,"sbr_header(): bs_reserved"));
    bs_header_extra_1 = faad_get1bit(ld
        DEBUGVAR(1,208,"sbr_header(): bs_header_extra_1"));
    bs_header_extra_2 = faad_get1bit(ld
        DEBUGVAR(1,209,"sbr_header(): bs_header_extra_2"));

    if (id_aac == ID_SCE)
    {
        faad_getbits(ld, 2
           DEBUGVAR(1,210,"sbr_header(): bs_reserved"));
    }

    if (bs_header_extra_1)
    {
        sbr->bs_freq_scale = faad_getbits(ld, 2
            DEBUGVAR(1,211,"sbr_header(): bs_freq_scale"));
        sbr->bs_alter_scale = faad_get1bit(ld
            DEBUGVAR(1,212,"sbr_header(): bs_alter_scale"));
        sbr->bs_noise_bands = faad_getbits(ld, 2
            DEBUGVAR(1,213,"sbr_header(): bs_noise_bands"));
    }
    if (bs_header_extra_2)
    {
        sbr->bs_limiter_bands = faad_getbits(ld, 2
            DEBUGVAR(1,214,"sbr_header(): bs_limiter_bands"));
        sbr->bs_limiter_gains = faad_getbits(ld, 2
            DEBUGVAR(1,215,"sbr_header(): bs_limiter_gains"));
        sbr->bs_interpol_freq = faad_get1bit(ld
            DEBUGVAR(1,216,"sbr_header(): bs_interpol_freq"));
        sbr->bs_smoothing_mode = faad_get1bit(ld
            DEBUGVAR(1,217,"sbr_header(): bs_smoothing_mode"));
        faad_get1bit(ld
            DEBUGVAR(1,218,"sbr_header(): bs_reserved"));
    }
}

/* table 4 */
static void sbr_data(bitfile *ld, sbr_info *sbr, uint8_t id_aac)
{
    sbr->bs_samplerate_mode = faad_get1bit(ld
        DEBUGVAR(1,219,"sbr_data(): bs_samplerate_mode"));

    switch (id_aac)
    {
    case ID_SCE:
        sbr_single_channel_element(ld, sbr);
        break;
    case ID_CPE:
        sbr_channel_pair_element(ld, sbr);
        break;
    }
}

/* table 5 */
static void sbr_single_channel_element(bitfile *ld, sbr_info *sbr)
{
    faad_get1bit(ld
        DEBUGVAR(1,220,"sbr_single_channel_element(): bs_reserved"));

    sbr_grid(ld, sbr, 0);
    sbr_dtdf(ld, sbr, 0);
    invf_mode(ld, sbr, 0);

    faad_getbits(ld, 2
        DEBUGVAR(1,221,"sbr_single_channel_element(): bs_reserved"));

    sbr_envelope(ld, sbr, 0);
    sbr_noise(ld, sbr, 0);

    faad_get1bit(ld
        DEBUGVAR(1,222,"sbr_single_channel_element(): bs_reserved"));

    faad_get1bit(ld
        DEBUGVAR(1,222,"sbr_single_channel_element(): bs_reserved"));

    sbr->bs_add_harmonic_flag[0] = faad_get1bit(ld
        DEBUGVAR(1,223,"sbr_single_channel_element(): bs_add_harmonic_flag[0]"));
    if (sbr->bs_add_harmonic_flag[0])
        sinusoidal_coding(ld, sbr, 0);

    sbr->bs_extended_data[0] = faad_get1bit(ld
        DEBUGVAR(1,224,"sbr_single_channel_element(): bs_extended_data[0]"));
    if (sbr->bs_extended_data[0])
    {
        uint16_t nr_bits_left;
        uint16_t cnt = faad_getbits(ld, 4
            DEBUGVAR(1,225,"sbr_single_channel_element(): bs_extension_size"));
        if (cnt == 15)
        {
            cnt += faad_getbits(ld, 8
                DEBUGVAR(1,226,"sbr_single_channel_element(): bs_esc_count"));
        }

        nr_bits_left = 8 * cnt;
        while (nr_bits_left > 7)
        {
            sbr->bs_extension_id = faad_getbits(ld, 2
                DEBUGVAR(1,227,"sbr_single_channel_element(): bs_extension_id"));
            nr_bits_left -= 2;
            /* sbr_extension(ld, sbr, 0, nr_bits_left); */
            faad_getbits(ld, 6
                DEBUGVAR(1,279,"sbr_single_channel_element(): bs_extension_data"));
        }
    }
}

/* table 6 */
static void sbr_channel_pair_element(bitfile *ld, sbr_info *sbr)
{
    sbr->bs_coupling = faad_get1bit(ld
        DEBUGVAR(1,228,"sbr_channel_pair_element(): bs_coupling"));

    if (sbr->bs_coupling)
    {
        sbr_grid(ld, sbr, 0);
        sbr_dtdf(ld, sbr, 0);
        sbr_dtdf(ld, sbr, 1);
        invf_mode(ld, sbr, 0);

        faad_getbits(ld, 2
            DEBUGVAR(1,229,"sbr_channel_pair_element(): bs_reserved"));

        sbr_envelope(ld, sbr, 0);
        sbr_noise(ld, sbr, 0);
        sbr_envelope(ld, sbr, 1);
        sbr_noise(ld, sbr, 1);

        faad_getbits(ld, 2
            DEBUGVAR(1,230,"sbr_channel_pair_element(): bs_reserved"));

        sbr->bs_add_harmonic_flag[0] = faad_get1bit(ld
            DEBUGVAR(1,231,"sbr_channel_pair_element(): bs_add_harmonic_flag[0]"));
        if (sbr->bs_add_harmonic_flag[0])
            sinusoidal_coding(ld, sbr, 0);

        sbr->bs_add_harmonic_flag[1] = faad_get1bit(ld
            DEBUGVAR(1,232,"sbr_channel_pair_element(): bs_add_harmonic_flag[1]"));
        if (sbr->bs_add_harmonic_flag[1])
            sinusoidal_coding(ld, sbr, 1);

        sbr->bs_extended_data[0] = faad_get1bit(ld
            DEBUGVAR(1,233,"sbr_channel_pair_element(): bs_extended_data[0]"));
        if (sbr->bs_extended_data[0])
        {
            uint16_t nr_bits_left;
            uint16_t cnt = faad_getbits(ld, 4
                DEBUGVAR(1,234,"sbr_channel_pair_element(): bs_extension_size"));
            if (cnt == 15)
            {
                cnt += faad_getbits(ld, 8
                    DEBUGVAR(1,235,"sbr_channel_pair_element(): bs_esc_count"));
            }

            nr_bits_left = 8 * cnt;
            while (nr_bits_left > 7)
            {
                sbr->bs_extension_id = faad_getbits(ld, 2
                    DEBUGVAR(1,236,"sbr_channel_pair_element(): bs_extension_id"));
                nr_bits_left -= 2;
                /* sbr_extension(ld, sbr, 0, nr_bits_left); */
                faad_getbits(ld, 6
                    DEBUGVAR(1,280,"sbr_single_channel_element(): bs_extension_data"));
            }
        }
    } else {
        sbr_grid(ld, sbr, 0);
        sbr_grid(ld, sbr, 1);
        sbr_dtdf(ld, sbr, 0);
        sbr_dtdf(ld, sbr, 1);
        invf_mode(ld, sbr, 0);
        invf_mode(ld, sbr, 1);

        faad_getbits(ld, 4
            DEBUGVAR(1,237,"sbr_channel_pair_element(): bs_reserved"));

        sbr_envelope(ld, sbr, 0);
        sbr_envelope(ld, sbr, 1);
        sbr_noise(ld, sbr, 0);
        sbr_noise(ld, sbr, 1);

        faad_getbits(ld, 2
            DEBUGVAR(1,238,"sbr_channel_pair_element(): bs_reserved"));

        sbr->bs_add_harmonic_flag[0] = faad_get1bit(ld
            DEBUGVAR(1,239,"sbr_channel_pair_element(): bs_add_harmonic_flag[0]"));
        if (sbr->bs_add_harmonic_flag[0])
            sinusoidal_coding(ld, sbr, 0);

        sbr->bs_add_harmonic_flag[1] = faad_get1bit(ld
            DEBUGVAR(1,240,"sbr_channel_pair_element(): bs_add_harmonic_flag[1]"));
        if (sbr->bs_add_harmonic_flag[1])
            sinusoidal_coding(ld, sbr, 1);

        sbr->bs_extended_data[0] = faad_get1bit(ld
            DEBUGVAR(1,241,"sbr_channel_pair_element(): bs_extended_data[0]"));
        if (sbr->bs_extended_data[0])
        {
            uint16_t nr_bits_left;
            uint16_t cnt = faad_getbits(ld, 4
                DEBUGVAR(1,242,"sbr_channel_pair_element(): bs_extension_size"));
            if (cnt == 15)
            {
                cnt += faad_getbits(ld, 8
                    DEBUGVAR(1,243,"sbr_channel_pair_element(): bs_esc_count"));
            }

            nr_bits_left = 8 * cnt;
            while (nr_bits_left > 7)
            {
                sbr->bs_extension_id = faad_getbits(ld, 2
                    DEBUGVAR(1,244,"sbr_channel_pair_element(): bs_extension_id"));
                nr_bits_left -= 2;
                /* sbr_extension(ld, sbr, 0, nr_bits_left); */
                faad_getbits(ld, 6
                    DEBUGVAR(1,281,"sbr_single_channel_element(): bs_extension_data"));
            }
        }

        sbr->bs_extended_data[1] = faad_get1bit(ld
            DEBUGVAR(1,245,"sbr_channel_pair_element(): bs_extended_data[1]"));
        if (sbr->bs_extended_data[1])
        {
            uint16_t nr_bits_left;
            uint16_t cnt = faad_getbits(ld, 4
                DEBUGVAR(1,246,"sbr_channel_pair_element(): bs_extension_size"));
            if (cnt == 15)
            {
                cnt += faad_getbits(ld, 8
                    DEBUGVAR(1,247,"sbr_channel_pair_element(): bs_esc_count"));
            }

            nr_bits_left = 8 * cnt;
            while (nr_bits_left > 7)
            {
                sbr->bs_extension_id = faad_getbits(ld, 2
                    DEBUGVAR(1,248,"sbr_channel_pair_element(): bs_extension_id"));
                nr_bits_left -= 2;
                /* sbr_extension(ld, sbr, 0, nr_bits_left); */
                faad_getbits(ld, 6
                    DEBUGVAR(1,282,"sbr_single_channel_element(): bs_extension_data"));
            }
        }
    }
}

/* table 7 */
static void sbr_grid(bitfile *ld, sbr_info *sbr, uint8_t ch)
{
    uint8_t i, env, rel;

    sbr->bs_frame_class = faad_getbits(ld, 2
        DEBUGVAR(1,248,"sbr_grid(): bs_frame_class"));

    switch (sbr->bs_frame_class)
    {
    case FIXFIX:
        i = faad_getbits(ld, 2
            DEBUGVAR(1,249,"sbr_grid(): bs_num_env_raw"));

        sbr->bs_num_env[ch] = min(1 << i, 5);
        if (sbr->bs_num_env[ch] == 1)
            sbr->bs_amp_res = 0;

        i = faad_get1bit(ld
            DEBUGVAR(1,250,"sbr_grid(): bs_freq_res_flag"));
        for (env = 0; env < sbr->bs_num_env[ch]; env++)
            sbr->bs_freq_res[ch][env] = i;
        break;

    case FIXVAR:
        sbr->bs_abs_bord[ch] = faad_getbits(ld, 3
            DEBUGVAR(1,251,"sbr_grid(): bs_abs_bord")) + NO_TIME_SLOTS;
        sbr->bs_num_env[ch] = faad_getbits(ld, 2
            DEBUGVAR(1,252,"sbr_grid(): bs_num_env")) + 1;

        for (rel = 0; rel < sbr->bs_num_env[ch]-1; rel++)
        {
            sbr->bs_rel_bord[ch][rel] = 2 * faad_getbits(ld, 2
                DEBUGVAR(1,253,"sbr_grid(): bs_rel_bord")) + 2;
        }
        i = int_log2((uint32_t)(sbr->bs_num_env[ch] + 1));
        sbr->bs_pointer[ch] = faad_getbits(ld, i
            DEBUGVAR(1,254,"sbr_grid(): bs_pointer"));

        for (env = 0; env < sbr->bs_num_env[ch]; env++)
        {
            sbr->bs_freq_res[ch][sbr->bs_num_env[ch] - env - 1] = faad_get1bit(ld
                DEBUGVAR(1,255,"sbr_grid(): bs_freq_res"));
        }
        break;

    case VARFIX:
        sbr->bs_abs_bord[ch] = faad_getbits(ld, 3
            DEBUGVAR(1,256,"sbr_grid(): bs_abs_bord"));
        sbr->bs_num_env[ch] = faad_getbits(ld, 2
            DEBUGVAR(1,257,"sbr_grid(): bs_num_env")) + 1;

        for (rel = 0; rel < sbr->bs_num_env[ch]-1; rel++)
        {
            sbr->bs_rel_bord[ch][rel] = 2 * faad_getbits(ld, 2
                DEBUGVAR(1,258,"sbr_grid(): bs_rel_bord")) + 2;
        }
        i = int_log2((uint32_t)(sbr->bs_num_env[ch] + 1));
        sbr->bs_pointer[ch] = faad_getbits(ld, i
            DEBUGVAR(1,259,"sbr_grid(): bs_pointer"));

        for (env = 0; env < sbr->bs_num_env[ch]; env++)
        {
            sbr->bs_freq_res[ch][env] = faad_get1bit(ld
                DEBUGVAR(1,260,"sbr_grid(): bs_freq_res"));
        }
        break;

    case VARVAR:
        sbr->bs_abs_bord_0[ch] = faad_getbits(ld, 3
            DEBUGVAR(1,261,"sbr_grid(): bs_abs_bord_0"));
        sbr->bs_abs_bord_1[ch] = faad_getbits(ld, 3
            DEBUGVAR(1,262,"sbr_grid(): bs_abs_bord_1")) + NO_TIME_SLOTS;
        sbr->bs_num_rel_0[ch] = faad_getbits(ld, 2
            DEBUGVAR(1,263,"sbr_grid(): bs_num_rel_0"));
        sbr->bs_num_rel_1[ch] = faad_getbits(ld, 2
            DEBUGVAR(1,264,"sbr_grid(): bs_num_rel_1"));
        sbr->bs_num_env[ch] = sbr->bs_num_rel_0[ch] + sbr->bs_num_rel_1[ch] + 1;

        for (rel = 0; rel < sbr->bs_num_rel_0[ch]; rel++)
        {
            sbr->bs_rel_bord_0[ch][rel] = 2 * faad_getbits(ld, 2
                DEBUGVAR(1,265,"sbr_grid(): bs_rel_bord")) + 2;
        }
        for(rel = 0; rel < sbr->bs_num_rel_1[ch]; rel++)
        {
            sbr->bs_rel_bord_1[ch][rel] = 2 * faad_getbits(ld, 2
                DEBUGVAR(1,266,"sbr_grid(): bs_rel_bord")) + 2;
        }
        i = int_log2((uint32_t)(sbr->bs_num_rel_0[ch] + sbr->bs_num_rel_1[ch] + 2));
        sbr->bs_pointer[ch] = faad_getbits(ld, i
            DEBUGVAR(1,267,"sbr_grid(): bs_pointer"));

        for (env = 0; env < sbr->bs_num_env[ch]; env++)
        {
            sbr->bs_freq_res[ch][env] = faad_get1bit(ld
                DEBUGVAR(1,268,"sbr_grid(): bs_freq_res"));
        }
        break;
    }

    if (sbr->bs_num_env[ch] > 1)
        sbr->bs_num_noise[ch] = 2;
    else
        sbr->bs_num_noise[ch] = 1;
}

/* table 8 */
static void sbr_dtdf(bitfile *ld, sbr_info *sbr, uint8_t ch)
{
    uint8_t i;

    for (i = 0; i < sbr->bs_num_env[ch]; i++)
    {
        sbr->bs_df_env[ch][i] = faad_get1bit(ld
            DEBUGVAR(1,269,"sbr_dtdf(): bs_df_env"));
    }

    for (i = 0; i < sbr->bs_num_noise[ch]; i++)
    {
        sbr->bs_df_noise[ch][i] = faad_get1bit(ld
            DEBUGVAR(1,270,"sbr_dtdf(): bs_df_noise"));
    }
}

/* table 9 */
static void invf_mode(bitfile *ld, sbr_info *sbr, uint8_t ch)
{
    uint8_t n;

    for (n = 0; n < sbr->num_noise_bands[ch]; n++)
    {
        sbr->bs_invf_mode_vec[ch][n] = faad_getbits(ld, 2
            DEBUGVAR(1,271,"invf_mode(): bs_invf_mode_vec"));
    }
}

#if 0
/* table 10 */
static void sbr_envelope(bitfile *ld, sbr_info *sbr, uint8_t ch)
{
    if (sbr->bs_coupling)
    {
        if (ch)
        {
            if (sbr->bs_amp_res)
            {
                t_huff = t_huffman_env_bal_3_0dB;
                f_huff = f_huffman_env_bal_3_0dB;
            } else {
                t_huff = t_huffman_env_bal_1_5dB;
                f_huff = f_huffman_env_bal_1_5dB;
            }
        } else {
            if (sbr->bs_amp_res)
            {
                t_huff = t_huffman_env_3_0dB;
                f_huff = f_huffman_env_3_0dB;
            } else {
                t_huff = t_huffman_env_1_5dB;
                f_huff = f_huffman_env_1_5dB;
            }
        }
    } else {
        if (sbr->bs_amp_res)
        {
            t_huff = t_huffman_env_3_0dB;
            f_huff = f_huffman_env_3_0dB;
        } else {
            t_huff = t_huffman_env_1_5dB;
            f_huff = f_huffman_env_1_5dB;
        }
    }

    for (env = 0; env < sbr->bs_num_env[ch]; env++)
    {
        if (sbr->bs_df_env[ch][env] == 0)
        {
            if (sbr->bs_coupling && ch)
            {
                if (sbr->bs_amp_res)
                {
                    sbr->bs_data_env[ch][env][0] = faad_getbits(ld, 5
                        DEBUGVAR(1,272,"sbr_envelope(): bs_data_env"));
                } else {
                    sbr->bs_data_env[ch][env][0] = faad_getbits(ld, 6
                        DEBUGVAR(1,273,"sbr_envelope(): bs_data_env"));
                }
            } else {
                if (bs_amp_res)
                {
                    sbr->bs_data_env[ch][env][0] = faad_getbits(ld, 6
                        DEBUGVAR(1,274,"sbr_envelope(): bs_data_env"));
                } else {
                    sbr->bs_data_env[ch][env][0] = faad_getbits(ld, 7
                        DEBUGVAR(1,275,"sbr_envelope(): bs_data_env"));
                }
                for (band = 1; band < num_env_bands[bs_freq_res[ch][env]]; band++)
                {
                    sbr->bs_data_env[ch][env][band] = huff_dec(ld, f_huff, bs_codeword);
                }
            }
        } else {
            for (band = 0; band < sbr->num_env_bands[bs_freq_res[ch][env]]; band++)
                sbr->bs_data_env[ch][env][band] = huff_dec(ld, t_huff, bs_codeword);
        }
    }
}

/* table 11 */
static void sbr_noise(bitfile *ld, sbr_info *sbr, uint8_t ch)
{
    if (sbr->bs_coupling)
    {
        if (ch) {
            t_huff = t_huffman_noise_bal_3_0dB;
            f_huff = f_huffman_noise_bal_3_0dB;
        } else {
            t_huff = t_huffman_noise_3_0dB;
            f_huff = f_huffman_noise_3_0dB;
        }
    } else {
        t_huff = t_huffman_noise_3_0dB;
        f_huff = f_huffman_noise_3_0dB;
    }

    for(noise = 0; noise < sbr->bs_num_noise[ch]; noise++)
    {
        if(sbr->bs_df_noise[ch][noise] == 0)
        {
            if (sbr->bs_coupling && ch)
            {
                sbr->bs_data_noise[ch][noise][0] = faad_getbits(ld, 5
                    DEBUGVAR(1,276,"sbr_noise(): bs_data_noise"));
            } else {
                sbr->bs_data_noise[ch][noise][0] = faad_getbits(ld, 5
                    DEBUGVAR(1,277,"sbr_noise(): bs_data_noise"));
            }
            for (band = 1; band < sbr->num_noise_bands[ch]; band++)
            {
                sbr->bs_data_noise[ch][noise][band] = huff_dec(ld, f_huff, bs_codeword);
            }
        } else {
            for (band = 0; band < sbr->num_noise_bands[ch]; band++)
            {
                sbr->bs_data_noise[ch][noise][band] = huff_dec(ld, t_huff, bs_codeword);
            }
        }
    }
}
#endif

/* table 12 */
static void sinusoidal_coding(bitfile *ld, sbr_info *sbr, uint8_t ch)
{
    uint8_t n;

    for (n = 0; n < sbr->num_high_res[ch]; n++)
    {
        sbr->bs_add_harmonic[ch][n] = faad_get1bit(ld
            DEBUGVAR(1,278,"sinusoidal_coding(): bs_add_harmonic"));
    }
}


#endif /* SBR */