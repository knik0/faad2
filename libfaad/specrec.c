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
** $Id: specrec.c,v 1.2 2002/02/11 11:34:18 menno Exp $
**/

/*
  Spectral reconstruction:
   - grouping/sectioning
   - inverse quantization
   - applying scalefactors
*/

#ifdef __ICL
#include <mathf.h>
#else
#include <math.h>
#endif
#include "specrec.h"
#include "syntax.h"
#include "data.h"


#define bit_set(A, B) ((A) & (1<<(B)))

/* 4.5.2.3.4 */
/*
  - determine the number of windows in a window_sequence named num_windows
  - determine the number of window_groups named num_window_groups
  - determine the number of windows in each group named window_group_length[g]
  - determine the total number of scalefactor window bands named num_swb for
    the actual window type
  - determine swb_offset[swb], the offset of the first coefficient in
    scalefactor window band named swb of the window actually used
  - determine sect_sfb_offset[g][section],the offset of the first coefficient
    in section named section. This offset depends on window_sequence and
    scale_factor_grouping and is needed to decode the spectral_data().
*/
int window_grouping_info(ic_stream *ics, int fs_index)
{
    int i, g;

    switch (ics->window_sequence) {
    case ONLY_LONG_SEQUENCE:
    case LONG_START_SEQUENCE:
    case LONG_STOP_SEQUENCE:
        ics->num_windows = 1;
        ics->num_window_groups = 1;
        ics->window_group_length[ics->num_window_groups-1] = 1;
        ics->num_swb = num_swb_long_window[fs_index];

        /* preparation of sect_sfb_offset for long blocks */
        /* also copy the last value! */
        for (i = 0; i < ics->num_swb + 1; i++)
        {
            ics->sect_sfb_offset[0][i] = swb_offset_long_window[fs_index][i];
            ics->swb_offset[i] = swb_offset_long_window[fs_index][i];
        }
        return 0;
    case EIGHT_SHORT_SEQUENCE:
        ics->num_windows = 8;
        ics->num_window_groups = 1;
        ics->window_group_length[ics->num_window_groups-1] = 1;
        ics->num_swb = num_swb_short_window[fs_index];

        for (i = 0; i < ics->num_swb + 1; i++)
            ics->swb_offset[i] = swb_offset_short_window[fs_index][i];

        for (i = 0; i < ics->num_windows-1; i++) {
            if (bit_set(ics->scale_factor_grouping, 6-i) == 0)
            {
                ics->num_window_groups += 1;
                ics->window_group_length[ics->num_window_groups-1] = 1;
            } else {
                ics->window_group_length[ics->num_window_groups-1] += 1;
            }
        }

        /* preparation of sect_sfb_offset for short blocks */
        for (g = 0; g < ics->num_window_groups; g++)
        {
            int width;
            int sect_sfb = 0;
            int offset = 0;
            for (i = 0; i < ics->num_swb; i++)
            {
                width = swb_offset_short_window[fs_index][i+1] -
                    swb_offset_short_window[fs_index][i];
                width *= ics->window_group_length[g];
                ics->sect_sfb_offset[g][sect_sfb++] = offset;
                offset += width;
            }
            ics->sect_sfb_offset[g][sect_sfb] = offset;
        }
        return 0;
    default:
        return 1;
    }
}

/*
  For ONLY_LONG_SEQUENCE windows (num_window_groups = 1,
  window_group_length[0] = 1) the spectral data is in ascending spectral
  order.
  For the EIGHT_SHORT_SEQUENCE window, the spectral order depends on the
  grouping in the following manner:
  - Groups are ordered sequentially
  - Within a group, a scalefactor band consists of the spectral data of all
    grouped SHORT_WINDOWs for the associated scalefactor window band. To
    clarify via example, the length of a group is in the range of one to eight
    SHORT_WINDOWs.
  - If there are eight groups each with length one (num_window_groups = 8,
    window_group_length[0..7] = 1), the result is a sequence of eight spectra,
    each in ascending spectral order.
  - If there is only one group with length eight (num_window_groups = 1,
    window_group_length[0] = 8), the result is that spectral data of all eight
    SHORT_WINDOWs is interleaved by scalefactor window bands.
  - Within a scalefactor window band, the coefficients are in ascending
    spectral order.
*/
void quant_to_spec(ic_stream *ics, float *spec_data)
{
    int g, width, sfb, win, bin;
    float *start_inptr, *start_win_ptr, *win_ptr;

    float tmp_spec[1024];
    float *tmp_spec_ptr, *spec_ptr;

    tmp_spec_ptr = tmp_spec;
    for (g = 1024/16-1; g >= 0; --g)
    {
        *tmp_spec_ptr++ = 0; *tmp_spec_ptr++ = 0;
        *tmp_spec_ptr++ = 0; *tmp_spec_ptr++ = 0;
        *tmp_spec_ptr++ = 0; *tmp_spec_ptr++ = 0;
        *tmp_spec_ptr++ = 0; *tmp_spec_ptr++ = 0;
        *tmp_spec_ptr++ = 0; *tmp_spec_ptr++ = 0;
        *tmp_spec_ptr++ = 0; *tmp_spec_ptr++ = 0;
        *tmp_spec_ptr++ = 0; *tmp_spec_ptr++ = 0;
        *tmp_spec_ptr++ = 0; *tmp_spec_ptr++ = 0;
    }

    spec_ptr = spec_data;
    tmp_spec_ptr = tmp_spec;
    start_win_ptr = tmp_spec_ptr;

    for (g = 0; g < ics->num_window_groups; g++)
    {
        int j = 0;
        int win_inc = 0;

        start_inptr = spec_ptr;

        win_inc = ics->swb_offset[ics->num_swb];

        for (sfb = 0; sfb < ics->num_swb; sfb++)
        {
            width = ics->swb_offset[sfb+1] - ics->swb_offset[sfb];

            win_ptr = start_win_ptr;

            for (win = 0; win < ics->window_group_length[g]; win++)
            {
                tmp_spec_ptr = win_ptr + j;

                for (bin = 0; bin < width; bin += 4)
                {
                    *tmp_spec_ptr++ = *spec_ptr++;
                    *tmp_spec_ptr++ = *spec_ptr++;
                    *tmp_spec_ptr++ = *spec_ptr++;
                    *tmp_spec_ptr++ = *spec_ptr++;
                }

                win_ptr += win_inc;
            }
            j += width;
        }
        start_win_ptr += (spec_ptr - start_inptr);
    }

    spec_ptr = spec_data;
    tmp_spec_ptr = tmp_spec;

    for (g = 1024/16 - 1; g >= 0; --g)
    {
        *spec_ptr++ = *tmp_spec_ptr++; *spec_ptr++ = *tmp_spec_ptr++;
        *spec_ptr++ = *tmp_spec_ptr++; *spec_ptr++ = *tmp_spec_ptr++;
        *spec_ptr++ = *tmp_spec_ptr++; *spec_ptr++ = *tmp_spec_ptr++;
        *spec_ptr++ = *tmp_spec_ptr++; *spec_ptr++ = *tmp_spec_ptr++;
        *spec_ptr++ = *tmp_spec_ptr++; *spec_ptr++ = *tmp_spec_ptr++;
        *spec_ptr++ = *tmp_spec_ptr++; *spec_ptr++ = *tmp_spec_ptr++;
        *spec_ptr++ = *tmp_spec_ptr++; *spec_ptr++ = *tmp_spec_ptr++;
        *spec_ptr++ = *tmp_spec_ptr++; *spec_ptr++ = *tmp_spec_ptr++;
    }
}

void build_tables(float *iq_table, float *pow2_table)
{
    int i;

    /* build pow() table for inverse quantization */
    for(i = 0; i < IQ_TABLE_SIZE; i++)
    {
#ifdef __ICL
        iq_table[i] = powf(i, 4.0f/3.0f);
#else
        iq_table[i] = (float)pow(i, 4.0/3.0);
#endif
    }

    /* build pow(2, 0.25) table for scalefactors */
    for(i = 0; i < POW_TABLE_SIZE; i++)
    {
#ifdef __ICL
        pow2_table[i] = powf(2.0f, 0.25f * (i-100));
#else
        pow2_table[i] = (float)pow(2.0, 0.25 * (i-100));
#endif
    }
}

void inverse_quantization(float *x_invquant, short *x_quant, float *iq_table)
{
    int i;

    for(i = 0; i < 1024; i++)
    {
        short q = x_quant[i];

        if (q > 0)
        {
            if (q < IQ_TABLE_SIZE)
                x_invquant[i] = iq_table[q];
            else
#ifdef __ICL
                x_invquant[i] = powf(q, 4.0f/3.0f);
#else
                x_invquant[i] = (float)pow(q, 4.0/3.0);
#endif
        } else if (q < 0) {
            q = -q;
            if (q < IQ_TABLE_SIZE)
                x_invquant[i] = -iq_table[q];
            else
#ifdef __ICL
                x_invquant[i] = -powf(q, 4.0f/3.0f);
#else
                x_invquant[i] = -(float)pow(q, 4.0/3.0);
#endif
        } else {
            x_invquant[i] = 0.0f;
        }
    }
}

static __inline float get_scale_factor_gain(int scale_factor, float *pow2_table)
{
    if ((scale_factor >= 0) && (scale_factor < POW_TABLE_SIZE))
        return pow2_table[scale_factor];
    else
#ifdef __ICL
        return powf(2.0f, 0.25f * (scale_factor - 100));
#else
        return (float)pow(2.0, 0.25 * (scale_factor - 100));
#endif
}

void apply_scalefactors(ic_stream *ics, float *x_invquant, float *pow2_table)
{
    int g, sfb, top;
    float *fp, scale;
    int groups = 0;

    for (g = 0; g < ics->num_window_groups; g++)
    {
        int k = 0;

        /* using this 128*groups doesn't hurt long blocks, because
           long blocks only have 1 group, so that means 'groups' is
           always 0 for long blocks
        */
        fp = x_invquant + (groups*128);

        for (sfb = 0; sfb < ics->max_sfb; sfb++)
        {
            top = ics->sect_sfb_offset[g][sfb+1];

            scale = get_scale_factor_gain(ics->scale_factors[g][sfb], pow2_table);

            /* minimum size of a sf band is 4 and always a multiple of 4 */
            for ( ; k < top; k+=4)
            {
                *fp++ *= scale;
                *fp++ *= scale;
                *fp++ *= scale;
                *fp++ *= scale;
            }
        }
        groups += ics->window_group_length[g];
    }
}
