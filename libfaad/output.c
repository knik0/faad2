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
** $Id: output.c,v 1.24 2003/11/02 20:24:04 menno Exp $
**/

#include "common.h"
#include "structs.h"

#include "output.h"
#include "decoder.h"

#ifndef FIXED_POINT



#define ftol(A,B) {tmp = *(int32_t*) & A - 0x4B7F8000; \
                   B = (int16_t)((tmp==(int16_t)tmp) ? tmp : (tmp>>31)^0x7FFF);}

#define ROUND(x) ((x >= 0) ? (int32_t)floor((x) + 0.5) : (int32_t)ceil((x) + 0.5))

#define ROUND32(x) ROUND(x)

#define FLOAT_SCALE (1.0f/(1<<15))

#define DM_MUL ((real_t)1.0/((real_t)1.0+(real_t)sqrt(2.0)))

static INLINE real_t get_sample(real_t **input, uint8_t channel, uint16_t sample,
                                uint8_t downMatrix, uint8_t *internal_channel)
{
    if (!downMatrix)
        return input[internal_channel[channel]][sample];

    if (channel == 0)
    {
        return DM_MUL * (input[internal_channel[1]][sample] +
            input[internal_channel[0]][sample]/(real_t)sqrt(2.) +
            input[internal_channel[3]][sample]/(real_t)sqrt(2.));
    } else {
        return DM_MUL * (input[internal_channel[2]][sample] +
            input[internal_channel[0]][sample]/(real_t)sqrt(2.) +
            input[internal_channel[4]][sample]/(real_t)sqrt(2.));
    }
}

void* output_to_PCM(faacDecHandle hDecoder,
                    real_t **input, void *sample_buffer, uint8_t channels,
                    uint16_t frame_len, uint8_t format)
{
    uint8_t ch;
    uint16_t i, j = 0;
    uint8_t internal_channel;

    int16_t   *short_sample_buffer = (int16_t*)sample_buffer;
    int32_t   *int_sample_buffer = (int32_t*)sample_buffer;
    float32_t *float_sample_buffer = (float32_t*)sample_buffer;
    double    *double_sample_buffer = (double*)sample_buffer;

    /* Copy output to a standard PCM buffer */
    for (ch = 0; ch < channels; ch++)
    {
        internal_channel = hDecoder->internal_channel[ch];

        switch (format)
        {
        case FAAD_FMT_16BIT:
            for(i = 0; i < frame_len; i++)
            {
                int32_t tmp;
                real_t ftemp;
                //real_t inp = input[internal_channel][i];
                real_t inp = get_sample(input, ch, i, hDecoder->downMatrix, hDecoder->internal_channel);

                ftemp = inp + 0xff8000;
                ftol(ftemp, short_sample_buffer[(i*channels)+ch]);
            }
            break;
        case FAAD_FMT_24BIT:
            for(i = 0; i < frame_len; i++)
            {
                //real_t inp = input[internal_channel][i];
                real_t inp = get_sample(input, ch, i, hDecoder->downMatrix, hDecoder->internal_channel);
                if (inp > (1<<15)-1)
                    inp = (1<<15)-1;
                else if (inp < -(1<<15))
                    inp = -(1<<15);
                int_sample_buffer[(i*channels)+ch] = ROUND(inp*(1<<8));
            }
            break;
        case FAAD_FMT_32BIT:
            for(i = 0; i < frame_len; i++)
            {
                //real_t inp = input[internal_channel][i];
                real_t inp = get_sample(input, ch, i, hDecoder->downMatrix, hDecoder->internal_channel);
                if (inp > (1<<15)-1)
                    inp = (1<<15)-1;
                else if (inp < -(1<<15))
                    inp = -(1<<15);
                int_sample_buffer[(i*channels)+ch] = ROUND32(inp*(1<<16));
            }
            break;
        case FAAD_FMT_FLOAT:
            for(i = 0; i < frame_len; i++)
            {
                //real_t inp = input[internal_channel][i];
                real_t inp = get_sample(input, ch, i, hDecoder->downMatrix, hDecoder->internal_channel);
                float_sample_buffer[(i*channels)+ch] = inp*FLOAT_SCALE;
            }
            break;
        case FAAD_FMT_DOUBLE:
            for(i = 0; i < frame_len; i++)
            {
                //real_t inp = input[internal_channel][i];
                real_t inp = get_sample(input, ch, i, hDecoder->downMatrix, hDecoder->internal_channel);
                double_sample_buffer[(i*channels)+ch] = (double)inp*FLOAT_SCALE;
            }
            break;
        }
    }

    return sample_buffer;
}

#else

void* output_to_PCM(faacDecHandle hDecoder,
                    real_t **input, void *sample_buffer, uint8_t channels,
                    uint16_t frame_len, uint8_t format)
{
    uint8_t ch;
    uint16_t i;
    int16_t *short_sample_buffer = (int16_t*)sample_buffer;

    /* Copy output to a standard PCM buffer */
    for (ch = 0; ch < channels; ch++)
    {
        for(i = 0; i < frame_len; i++)
        {
            int32_t tmp = input[ch][i];
            tmp += (1 << (REAL_BITS-1));
            tmp >>= REAL_BITS;
            if (tmp > 0x7fff)       tmp = 0x7fff;
            else if (tmp <= -32768) tmp = -32768;
            short_sample_buffer[(i*channels)+ch] = (int16_t)tmp;
        }
    }

    return sample_buffer;
}

#endif
