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
** $Id: syntax.c,v 1.1 2002/01/14 19:15:57 menno Exp $
**/

/*
   Reads the AAC bitstream as defined in 14496-3 (MPEG-4 Audio)

   (Note that there are some differences with 13818-7 (MPEG2), these
   are alse read correctly when the MPEG ID is known (can be found in
   an ADTS header)).
*/

#include <stdlib.h>
#include <memory.h>
#include "syntax.h"
#include "specrec.h"
#include "huffman.h"
#include "bits.h"
#include "data.h"
#include "pulse.h"

/* Table 4.4.2 */
/* An MPEG-4 Audio decoder is only required to follow the Program
   Configuration Element in GASpecificConfig(). The decoder shall ignore
   any Program Configuration Elements that may occur in raw data blocks.
   PCEs transmitted in raw data blocks cannot be used to convey decoder
   configuration information.
*/
int program_config_element(program_config *pce, bitfile *ld)
{
    int i;

    pce->channels = 0;

    pce->element_instance_tag = faad_getbits(ld, 4);

    pce->object_type = faad_getbits(ld, 2);
    pce->sf_index = faad_getbits(ld, 4);
    pce->num_front_channel_elements = faad_getbits(ld, 4);
    pce->num_side_channel_elements = faad_getbits(ld, 4);
    pce->num_back_channel_elements = faad_getbits(ld, 4);
    pce->num_lfe_channel_elements = faad_getbits(ld, 2);
    pce->num_assoc_data_elements = faad_getbits(ld, 3);
    pce->num_valid_cc_elements = faad_getbits(ld, 4);

    pce->mono_mixdown_present = faad_get1bit(ld);
    if (pce->mono_mixdown_present == 1)
        pce->mono_mixdown_element_number = faad_getbits(ld, 4);

    pce->stereo_mixdown_present = faad_get1bit(ld);
    if (pce->stereo_mixdown_present == 1)
        pce->stereo_mixdown_element_number = faad_getbits(ld, 4);

    pce->matrix_mixdown_idx_present = faad_get1bit(ld);
    if (pce->matrix_mixdown_idx_present == 1)
    {
        pce->matrix_mixdown_idx = faad_getbits(ld, 2);
        pce->pseudo_surround_enable = faad_get1bit(ld);
    }

    for (i = 0; i < pce->num_front_channel_elements; i++)
    {
        if ((pce->front_element_is_cpe[i] = faad_get1bit(ld)) & 1)
            pce->channels += 2;
        else
            pce->channels++;
        pce->front_element_tag_select[i] = faad_getbits(ld, 4);
    }

    for (i = 0; i < pce->num_side_channel_elements; i++)
    {
        if ((pce->side_element_is_cpe[i] = faad_get1bit(ld)) & 1)
            pce->channels += 2;
        else
            pce->channels++;
        pce->side_element_tag_select[i] = faad_getbits(ld, 4);
    }

    for (i = 0; i < pce->num_back_channel_elements; i++)
    {
        if ((pce->back_element_is_cpe[i] = faad_get1bit(ld)) & 1)
            pce->channels += 2;
        else
            pce->channels++;
        pce->back_element_tag_select[i] = faad_getbits(ld, 4);
    }

    for (i = 0; i < pce->num_lfe_channel_elements; i++)
    {
        pce->channels++;
        pce->lfe_element_tag_select[i] = faad_getbits(ld, 4);
    }

    for (i = 0; i < pce->num_assoc_data_elements; i++)
        pce->assoc_data_element_tag_select[i] = faad_getbits(ld, 4);

    for (i = 0; i < pce->num_valid_cc_elements; i++)
    {
        /* have to count these as channels too?? */
        pce->cc_element_is_ind_sw[i] = faad_get1bit(ld);
        pce->valid_cc_element_tag_select[i] = faad_getbits(ld, 4);
    }

    faad_byte_align(ld);

    pce->comment_field_bytes = faad_getbits(ld, 8);
    i = 0;
    for (i = 0; i < pce->comment_field_bytes; i++)
        pce->comment_field_data[i] = faad_getbits(ld, 8);
    pce->comment_field_data[i] = 0;

    return 0;
}

/* Table 4.4.4 and */
/* Table 4.4.9 */
int single_lfe_channel_element(element *sce, bitfile *ld, short *spec_data,
                               int sf_index, int object_type)
{
    ic_stream *ics = &sce->ics1;

    sce->element_instance_tag = faad_getbits(ld, LEN_TAG);

    return individual_channel_stream(sce, ld, ics, 0, spec_data, sf_index,
        object_type);
}

/* Table 4.4.5 */
int channel_pair_element(element *cpe, bitfile *ld, short *spec_data1,
                         short *spec_data2, int sf_index, int object_type)
{
    int result;

    ic_stream *ics1 = &cpe->ics1;
    ic_stream *ics2 = &cpe->ics2;

    cpe->element_instance_tag = faad_getbits(ld, LEN_TAG);

    if ((cpe->common_window = faad_get1bit(ld)) & 1)
    {
        /* both channels have common ics information */
        if ((result = ics_info(ics1, ld, cpe->common_window, sf_index,
            object_type)) > 0)
            return result;

        ics1->ms_mask_present = faad_getbits(ld, 2);
        if (ics1->ms_mask_present == 1)
        {
            int g, sfb;
            for (g = 0; g < ics1->num_window_groups; g++)
            {
                for (sfb = 0; sfb < ics1->max_sfb; sfb++)
                {
                    ics1->ms_used[g][sfb] = faad_get1bit(ld);
                }
            }
        }

        memcpy(ics2, ics1, sizeof(ic_stream));
    } else {
        ics1->ms_mask_present = 0;
    }

    if ((result = individual_channel_stream(cpe, ld, ics1, 0, spec_data1,
        sf_index, object_type)) > 0)
        return result;
    if ((result = individual_channel_stream(cpe, ld, ics2, 0, spec_data2,
        sf_index, object_type)) > 0)
        return result;

    return 0;
}

/* Table 4.4.6 */
static int ics_info(ic_stream *ics, bitfile *ld, int common_window,
                    int sf_index, int object_type)
{
    /* ics->ics_reserved_bit = */ faad_get1bit(ld);
    ics->window_sequence = faad_getbits(ld, 2);
    ics->window_shape = faad_get1bit(ld);

    if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
    {
        ics->max_sfb = faad_getbits(ld, 4);
        ics->scale_factor_grouping = faad_getbits(ld, 7);
    } else {
        ics->max_sfb = faad_getbits(ld, 6);

        if (object_type == LTP)
        {
            if ((ics->predictor_data_present = faad_get1bit(ld)) & 1)
            {
                if ((ics->ltp.data_present = faad_get1bit(ld)) & 1)
                    ltp_data(ics, &ics->ltp, ld);
                if (common_window)
                    if ((ics->ltp2.data_present = faad_get1bit(ld)) & 1)
                        ltp_data(ics, &ics->ltp2, ld);
            }
        } else { /* MPEG2 style AAC predictor */
            if ((ics->predictor_data_present = faad_get1bit(ld)) & 1)
            {
                int sfb;
                ics->pred.limit = min(ics->max_sfb, pred_sfb_max[sf_index]);

                if ((ics->pred.predictor_reset = faad_get1bit(ld)) & 1)
                    ics->pred.predictor_reset_group_number = faad_getbits(ld, 5);

                for (sfb = 0; sfb < ics->pred.limit; sfb++)
                    ics->pred.prediction_used[sfb] = faad_get1bit(ld);
            }
        }
    }

    /* get the grouping information */
    return window_grouping_info(ics, sf_index);
}

/* Table 4.4.7 */
static void pulse_data(pulse_info *pul, bitfile *ld)
{
    int i;

    pul->number_pulse = faad_getbits(ld, 2);
    pul->pulse_start_sfb = faad_getbits(ld, 6);

    for (i = 0; i < pul->number_pulse+1; i++) {
        pul->pulse_offset[i] = faad_getbits(ld, 5);
        pul->pulse_amp[i] = faad_getbits(ld, 4);
    }
}

/* Table 4.4.10 */
int data_stream_element(bitfile *ld)
{
    int i, byte_aligned, count;
    char data_stream_byte;

    /* element_instance_tag = */ faad_getbits(ld, LEN_TAG);
    byte_aligned = faad_get1bit(ld);
    count = faad_getbits(ld, 8);
    if (count == 255)
        count += faad_getbits(ld, 8);
    if (byte_aligned)
        faad_byte_align(ld);

    for (i = 0; i < count; i++)
        data_stream_byte = faad_getbits(ld, LEN_BYTE);

    return count;
}

/* Table 4.4.11 */
int fill_element(bitfile *ld, drc_info *drc)
{
    int count;

    count = faad_getbits(ld, 4);
    if (count == 15)
        count += faad_getbits(ld, 8) - 1;

    while (count > 0)
    {
        count -= extension_payload(ld, drc, count);
    }

    return 0;
}

/* Table 4.4.24 */
static int individual_channel_stream(element *ele, bitfile *ld,
                                     ic_stream *ics, int scal_flag,
                                     short *spec_data, int sf_index,
                                     int object_type)
{
    int result;

    ics->global_gain = faad_getbits(ld, 8);

    if (!ele->common_window && !scal_flag)
    {
        if ((result = ics_info(ics, ld, ele->common_window, sf_index,
            object_type)) > 0)
            return result;
    }
    section_data(ics, ld);
    if ((result = scale_factor_data(ics, ld)) > 0)
        return result;

    if (!scal_flag)
    {
        /* get pulse data */
        if ((ics->pulse_data_present = faad_get1bit(ld)) & 1)
            pulse_data(&ics->pul, ld);

        /* get tns data */
        if ((ics->tns_data_present = faad_get1bit(ld)) & 1)
            tns_data(ics, &ics->tns, ld);

        /* get gain control data */
        if ((ics->gain_control_data_present = faad_get1bit(ld)) & 1)
            return 1;
    }

    /* decode the spectral data */
    if ((result = spectral_data(ics, ld, spec_data)) > 0)
        return result;

    /* pulse coding reconstruction */
    if (ics->pulse_data_present)
    {
        if (ics->window_sequence != EIGHT_SHORT_SEQUENCE)
            pulse_decode(ics, spec_data);
        else
            return 2; /* pulse coding not allowed for long blocks */
    }

    return 0;
}

/* Table 4.4.25 */
static void section_data(ic_stream *ics, bitfile *ld)
{
    int g;
    int sect_esc_val, sect_bits;

    if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
        sect_bits = 3;
    else
        sect_bits = 5;
    sect_esc_val = (1<<sect_bits) - 1;

    for (g = 0; g < ics->num_window_groups; g++)
    {
        int k = 0;
        int i = 0;

        while (k < ics->max_sfb)
        {
            int sfb;
            int sect_len_incr;
            int sect_len = 0;

            ics->sect_cb[g][i] = faad_getbits(ld, 4);

            while ((sect_len_incr = faad_getbits(ld, sect_bits)) == sect_esc_val)
                sect_len += sect_esc_val;
            sect_len += sect_len_incr;

            ics->sect_start[g][i] = k;
            ics->sect_end[g][i] = k + sect_len;

            for (sfb = k; sfb < k + sect_len; sfb++)
                ics->sfb_cb[g][sfb] = ics->sect_cb[g][i];

            k += sect_len;
            i++;
        }
        ics->num_sec[g] = i;
    }
}

/*
  All scalefactors (and also the stereo positions and pns energies) are
  transmitted using Huffman coded DPCM relative to the previous active
  scalefactor (respectively previous stereo position or previous pns energy,
  see subclause 4.6.2 and 4.6.3). The first active scalefactor is
  differentially coded relative to the global gain.
*/
/* Table 4.4.26 */
static int scale_factor_data(ic_stream *ics, bitfile *ld)
{
    int g, sfb, t;
    int noise_pcm_flag = 1;
    int scale_factor = ics->global_gain;
    int is_position = 0;
    int noise_energy = ics->global_gain - 90;

    for (g = 0; g < ics->num_window_groups; g++)
    {
        for (sfb = 0; sfb < ics->max_sfb; sfb++)
        {
            switch (ics->sfb_cb[g][sfb])
            {
            case ZERO_HCB: /* zero book */
                ics->scale_factors[g][sfb] = 0;
                break;
            case INTENSITY_HCB: /* intensity books */
            case INTENSITY_HCB2:

                /* decode intensity position */
                t = huffman_scale_factor(ld) - 60;
                is_position += t;
                ics->scale_factors[g][sfb] = is_position;

                break;
            case NOISE_HCB: /* noise books */

                /* decode noise energy */
                if (noise_pcm_flag) {
                    noise_pcm_flag = 0;
                    t = faad_getbits(ld, 9) - 256;
                } else
                    t = huffman_scale_factor(ld) - 60;
                noise_energy += t;
                ics->scale_factors[g][sfb] = noise_energy;

                break;
            case BOOKSCL: /* invalid books */
                return 3;
            default: /* spectral books */

                /* decode scale factor */
                t = huffman_scale_factor(ld) - 60;
                scale_factor += t;
                if (scale_factor < 0)
                    return 4;
                ics->scale_factors[g][sfb] = scale_factor;

                break;
            }
        }
    }

    return 0;
}

/* Table 4.4.27 */
static void tns_data(ic_stream *ics, tns_info *tns, bitfile *ld)
{
    int w, filt, i, coef_bits;
    int n_filt_bits = 2;
    int length_bits = 6;
    int order_bits = 5;

    if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
    {
        n_filt_bits = 1;
        length_bits = 4;
        order_bits = 3;
    }

    for (w = 0; w < ics->num_windows; w++)
    {
        tns->n_filt[w] = faad_getbits(ld, n_filt_bits);

        if (tns->n_filt[w])
        {
            if ((tns->coef_res[w] = faad_get1bit(ld)) & 1)
                coef_bits = 4;
            else
                coef_bits = 3;
        }

        for (filt = 0; filt < tns->n_filt[w]; filt++)
        {
            tns->length[w][filt] = faad_getbits(ld, length_bits);
            tns->order[w][filt]  = faad_getbits(ld, order_bits);
            if (tns->order[w][filt])
            {
                tns->direction[w][filt] = faad_get1bit(ld);
                tns->coef_compress[w][filt] = faad_get1bit(ld);

                coef_bits -= tns->coef_compress[w][filt];
                for (i = 0; i < tns->order[w][filt]; i++)
                {
                    tns->coef[w][filt][i] = faad_getbits(ld, coef_bits);
                }
            }
        }
    }
}

/* Table 4.4.28 */
/*
   The limit MAX_LTP_SFB is not defined in 14496-3, this is a bug in the document
   and will be corrected in one of the corrigenda.
*/
static void ltp_data(ic_stream *ics, ltp_info *ltp, bitfile *ld)
{
    int sfb, w;

    ltp->lag = faad_getbits(ld, 11);
    ltp->coef = faad_getbits(ld, 3);

    if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
    {
        for (w = 0; w < ics->num_windows; w++)
        {
            if ((ltp->short_used[w] = faad_get1bit(ld)) & 1)
            {
                ltp->short_lag_present[w] = faad_get1bit(ld);
                if (ltp->short_lag_present[w])
                    ltp->short_lag[w] = faad_getbits(ld, 4);
			}
        }
    } else {
        ltp->last_band = (ics->max_sfb < MAX_LTP_SFB ? ics->max_sfb : MAX_LTP_SFB);

        for (sfb = 0; sfb < ltp->last_band; sfb++)
            ltp->long_used[sfb] = faad_get1bit(ld);
    }
}

/* defines whether a huffman codebook is unsigned or not */
/* Table 4.6.2 */
static int unsigned_cb[] = { 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0 };

/* Table 4.4.29 */
static int spectral_data(ic_stream *ics, bitfile *ld, short *spectral_data)
{
    int g, i, k, inc;
    short *sp;
    int p = 0;
    int groups = 0;

    sp = spectral_data;
    for (i = 1024/16-1; i >= 0; --i)
    {
        *sp++ = 0; *sp++ = 0; *sp++ = 0; *sp++ = 0;
        *sp++ = 0; *sp++ = 0; *sp++ = 0; *sp++ = 0;
        *sp++ = 0; *sp++ = 0; *sp++ = 0; *sp++ = 0;
        *sp++ = 0; *sp++ = 0; *sp++ = 0; *sp++ = 0;
    }

    for(g = 0; g < ics->num_window_groups; g++)
    {
        p = groups*128;

        for (i = 0; i < ics->num_sec[g]; i++)
        {
            switch (ics->sect_cb[g][i])
            {
            case ZERO_HCB:
            case NOISE_HCB:
            case INTENSITY_HCB:
            case INTENSITY_HCB2:
                p += (ics->sect_sfb_offset[g][ics->sect_end[g][i]] -
                    ics->sect_sfb_offset[g][ics->sect_start[g][i]]);
                break;
            default:
                for (k = ics->sect_sfb_offset[g][ics->sect_start[g][i]];
                     k < ics->sect_sfb_offset[g][ics->sect_end[g][i]]; )
                {
                    sp = spectral_data + p;

                    inc = (ics->sect_cb[g][i] < FIRST_PAIR_HCB)?QUAD_LEN:PAIR_LEN;

                    huffman_spectral_data(ics->sect_cb[g][i], ld, sp);
                    if (unsigned_cb[ics->sect_cb[g][i]])
                        huffman_sign_bits(ld, sp, inc);
                    k += inc;
                    p += inc;
                    if (ics->sect_cb[g][i] == ESC_HCB)
                    {
                        sp[0] = huffman_getescape(ld, sp[0]);
                        sp[1] = huffman_getescape(ld, sp[1]);
                    }
                }
                break;
            }
        }
        groups += ics->window_group_length[g];
    }

    return 0;
}

/* Table 4.4.30 */
static int extension_payload(bitfile *ld, drc_info *drc, int count)
{
    int i, n;
    int extension_type = faad_getbits(ld, 4);

    switch (extension_type)
    {
    case EXT_DYNAMIC_RANGE:
        drc->present = 1;
        n = dynamic_range_info(ld, drc);
        return n;
    case EXT_FILL_DATA:
        /* fill_nibble = */ faad_getbits(ld, 4); /* must be ‘0000’ */
        for (i = 0; i < count-1; i++)
            /* fill_byte[i] = */ faad_getbits(ld, 8); /* must be ‘10100101’ */
        return count;
    default:
        for (i = 0; i < 8*(count-1)+4; i++)
            /* other_bits[i] = */ faad_get1bit(ld);
        return count;
    }
}

/* Table 4.4.31 */
static int dynamic_range_info(bitfile *ld, drc_info *drc)
{
    int i, n = 1;
    int band_incr;

    drc->num_bands = 1;

    if (faad_get1bit(ld) & 1)
    {
        drc->pce_instance_tag = faad_getbits(ld, 4);
        /* drc->drc_tag_reserved_bits = */ faad_getbits(ld, 4);
        n++;
    }

    drc->excluded_chns_present = faad_get1bit(ld);
    if (drc->excluded_chns_present == 1)
    {
        n += excluded_channels(ld, drc);
    }

    if (faad_get1bit(ld) & 1)
    {
        band_incr = faad_getbits(ld, 4);
        /* drc->drc_bands_reserved_bits = */ faad_getbits(ld, 4);
        n++;
        drc->num_bands += band_incr;

        for (i = 0; i < drc->num_bands; i++);
        {
            drc->band_top[i] = faad_getbits(ld, 8);
            n++;
        }
    }

    if (faad_get1bit(ld) & 1)
    {
        drc->prog_ref_level = faad_getbits(ld, 7);
        /* drc->prog_ref_level_reserved_bits = */ faad_get1bit(ld);
        n++;
    }

    for (i = 0; i < drc->num_bands; i++)
    {
        drc->dyn_rng_sgn[i] = faad_get1bit(ld);
        drc->dyn_rng_ctl[i] = faad_getbits(ld, 7);
        n++;
    }

    return n;
}

/* Table 4.4.32 */
static int excluded_channels(bitfile *ld, drc_info *drc)
{
    int i, n = 0;
    int num_excl_chan = 7;

    for (i = 0; i < 7; i++)
        drc->exclude_mask[i] = faad_get1bit(ld);
    n++;

    while ((drc->additional_excluded_chns[n-1] = faad_get1bit(ld)) == 1)
    {
        for (i = num_excl_chan; i < num_excl_chan+7; i++)
            drc->exclude_mask[i] = faad_get1bit(ld);
        n++;
        num_excl_chan += 7;
    }

    return n;
}

/* Annex A: Audio Interchange Formats */

/* Table 1.A.2 */
void get_adif_header(adif_header *adif, bitfile *ld)
{
    int i;

    /* adif_id[0] = */ faad_getbits(ld, 8);
    /* adif_id[1] = */ faad_getbits(ld, 8);
    /* adif_id[2] = */ faad_getbits(ld, 8);
    /* adif_id[3] = */ faad_getbits(ld, 8);
    adif->copyright_id_present = faad_get1bit(ld);
    if(adif->copyright_id_present)
    {
        for (i = 0; i < 72/8; i++)
            adif->copyright_id[i] = faad_getbits(ld, 8);
        adif->copyright_id[i] = 0;
    }
    adif->original_copy  = faad_get1bit(ld);
    adif->home = faad_get1bit(ld);
    adif->bitstream_type = faad_get1bit(ld);
    adif->bitrate = faad_getbits(ld, 23);
    adif->num_program_config_elements = faad_getbits(ld, 4);

    for (i = 0; i < adif->num_program_config_elements + 1; i++)
    {
        if(adif->bitstream_type == 0)
            adif->adif_buffer_fullness = faad_getbits(ld, 20);
        else
            adif->adif_buffer_fullness = 0;

        program_config_element(&adif->pce, ld);
    }
}

/* Table 1.A.5 */
int adts_frame(adts_header *adts, bitfile *ld)
{
    /* faad_byte_align(ld); */
    if (adts_fixed_header(adts, ld))
        return 5;
    adts_variable_header(adts, ld);
    adts_error_check(adts, ld);

    return 0;
}

/* Table 1.A.6 */
static int adts_fixed_header(adts_header *adts, bitfile *ld)
{
    adts->syncword = faad_getbits(ld, 12);
    if (adts->syncword != 0xFFF)
        return 5;
    adts->id = faad_get1bit(ld);
    adts->layer = faad_getbits(ld, 2);
    adts->protection_absent = faad_get1bit(ld);
    adts->profile = faad_getbits(ld, 2);
    adts->sf_index = faad_getbits(ld, 4);
    adts->private_bit = faad_get1bit(ld);
    adts->channel_configuration = faad_getbits(ld, 3);
    adts->original = faad_get1bit(ld);
    adts->home = faad_get1bit(ld);
    if (adts->id == 0)
        adts->emphasis = faad_getbits(ld, 2);

    return 0;
}

/* Table 1.A.7 */
static void adts_variable_header(adts_header *adts, bitfile *ld)
{
    adts->copyright_identification_bit = faad_get1bit(ld);
    adts->copyright_identification_start = faad_get1bit(ld);
    adts->aac_frame_length = faad_getbits(ld, 13);
    adts->adts_buffer_fullness = faad_getbits(ld, 11);
    adts->no_raw_data_blocks_in_frame = faad_getbits(ld, 2);
}

/* Table 1.A.8 */
static void adts_error_check(adts_header *adts, bitfile *ld)
{
    if (adts->protection_absent == 0)
        adts->crc_check = faad_getbits(ld, 16);
}
