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
** $Id: rvlc_scale_factors.c,v 1.1 2002/08/05 20:33:38 menno Exp $
**/

#include "common.h"
#include <stdlib.h>
#include "syntax.h"
#include "bits.h"
#include "rvlc_scale_factors.h"


#ifdef ERROR_RESILIENCE

#if 0
        uint32_t bits_used, length_of_rvlc_sf;
        uint8_t bits = 11;

        sf_concealment = faad_get1bit(ld
            DEBUGVAR(1,149,"scale_factor_data(): sf_concealment"));
        rev_global_gain = faad_getbits(ld, 8
            DEBUGVAR(1,150,"scale_factor_data(): rev_global_gain"));

        if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
            bits = 9;

        /* the number of bits used for the huffman codewords */
        length_of_rvlc_sf = faad_getbits(ld, bits
            DEBUGVAR(1,151,"scale_factor_data(): length_of_rvlc_sf"));

        /* check how many bits are used in decoding the scalefactors
           A better solution would be to read length_of_rvlc_sf ahead
           in a buffer and use that to decode the scale factors
        */
        bits_used = faad_get_processed_bits(ld);
        decode_scale_factors(ics, ld);
        bits_used = faad_get_processed_bits(ld) - bits_used;

        /* return an error if the number of decoded bits is not correct
           FAAD should be able to recover from this, for example by
           setting all scalefactors to 0 (e.g. muting the frame)
        */
        if (bits_used != length_of_rvlc_sf)
            return 8;

        sf_escapes_present; 1 uimsbf

        if (sf_escapes_present)
        {
            length_of_rvlc_escapes; 8 uimsbf

            for (g = 0; g < num_window_groups; g++)
            {
                for (sfb = 0; sfb < max_sfb; sfb++)
                {
                    if (sect_cb[g][sfb] != ZERO_HCB)
                    {
                        if (is_intensity(g, sfb) &&
                            dpcm_is_position[g][sfb] == ESC_FLAG)
                        {
                            rvlc_esc_sf[dpcm_is_position[g][sfb]]; 2..20 vlclbf
                        } else {
                            if (is_noise(g, sfb) &&
                                dpcm_noise_nrg[g][sfb] == ESC_FLAG)
                            {
                                rvlc_esc_sf[dpcm_noise_nrg[g][sfb]]; 2..20 vlclbf
                            } else {
                                if (dpcm_sf[g][sfb] == ESC_FLAG)
                                {
                                    rvlc_esc_sf[dpcm_sf[g][sfb]]; 2..20 vlclbf
                                }
                            }
                        }
                    }
                }
            }

            if (intensity_used &&
                dpcm_is_position[g][sfb] == ESC_FLAG)
            {
                rvlc_esc_sf[dpcm_is_last_position]; 2..20 vlclbf
            }
        }

        if (noise_used)
        {
            dpcm_noise_last_position; 9 uimsbf
        }
#endif

uint8_t rvlc_scale_factor_data(ic_stream *ics, bitfile *ld)
{
    uint8_t bits = 9;

    ics->sf_concealment = faad_get1bit(ld
        DEBUGVAR(1,149,"rvlc_scale_factor_data(): sf_concealment"));
    ics->rev_global_gain = faad_getbits(ld, 8
        DEBUGVAR(1,150,"rvlc_scale_factor_data(): rev_global_gain"));

    if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
        bits = 11;

    /* the number of bits used for the huffman codewords */
    ics->length_of_rvlc_sf = faad_getbits(ld, bits
        DEBUGVAR(1,151,"rvlc_scale_factor_data(): length_of_rvlc_sf"));

    if (ics->noise_used)
    {
        ics->dpcm_noise_nrg = faad_getbits(ld, 9
            DEBUGVAR(1,152,"rvlc_scale_factor_data(): dpcm_noise_nrg"));

        ics->length_of_rvlc_sf -= 9;
    }

    ics->sf_escapes_present = faad_get1bit(ld
        DEBUGVAR(1,153,"rvlc_scale_factor_data(): sf_escapes_present"));

    if (ics->sf_escapes_present)
    {
        ics->length_of_rvlc_escapes = faad_getbits(ld, 8
            DEBUGVAR(1,154,"rvlc_scale_factor_data(): length_of_rvlc_escapes"));
    }

    if (ics->noise_used)
    {
        ics->dpcm_noise_last_position = faad_getbits(ld, 9
            DEBUGVAR(1,155,"rvlc_scale_factor_data(): dpcm_noise_last_position"));
    }

    return 0;
}

uint8_t rvlc_decode_scale_factors(ic_stream *ics, bitfile *ld)
{
    void *rvlc_buffer = NULL;
    void *rvlc_esc_buffer = NULL;

    if (ics->length_of_rvlc_sf > 0)
    {
        rvlc_buffer = malloc((bit2byte(ics->length_of_rvlc_sf)+1)*sizeof(uint8_t));

        /* We read length_of_rvlc_sf bits here to put it in a
           seperate bitfile.
        */
        faad_getbitbuffer(ld, rvlc_buffer, ics->length_of_rvlc_sf
            DEBUGVAR(1,156,"rvlc_decode_scale_factors(): bitbuffer: length_of_rvlc_sf"));
    }

    if (ics->sf_escapes_present)
    {
        rvlc_esc_buffer = malloc((bit2byte(ics->length_of_rvlc_escapes)+1)*sizeof(uint8_t));

        /* We read length_of_rvlc_escapes bits here to put it in a
           seperate bitfile.
        */
        faad_getbitbuffer(ld, rvlc_esc_buffer, ics->length_of_rvlc_escapes
            DEBUGVAR(1,157,"rvlc_decode_scale_factors(): bitbuffer: length_of_rvlc_escapes"));
    }

    if (rvlc_esc_buffer) free(rvlc_esc_buffer);
    if (rvlc_buffer) free(rvlc_buffer);

    return 0;
}

#endif