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
** $Id: output.c,v 1.6 2002/03/16 13:38:36 menno Exp $
**/

#include "common.h"

#include "output.h"
#include "decoder.h"


#define ftol(A,B) {tmp = *(int32_t*) & A - 0x4B7F8000; \
                   B = (int16_t)((tmp==(int16_t)tmp) ? tmp : (tmp>>31)^0x7FFF);}
#define ROUND(x) ((int32_t)floor((x) + 0.5))

#define HAVE_IEEE754_FLOAT
#ifdef HAVE_IEEE754_FLOAT
#define ROUND32(x) (floattmp = (x) + (int32_t)0x00FD8000L, *(int32_t*)(&floattmp) - (int32_t)0x4B7D8000L)
#else
#define ROUND32(x) ROUND(x)
#endif

#define FLOAT_SCALE (1.0f/(1<<15))


void* output_to_PCM(real_t **input, void *sample_buffer, uint8_t channels,
                    uint16_t frame_len, uint8_t format)
{
    uint8_t ch;
    uint16_t i;

    uint8_t   *p = (uint8_t*)sample_buffer;
    int16_t   *short_sample_buffer = (int16_t*)sample_buffer;
    int32_t   *int_sample_buffer = (int32_t*)sample_buffer;
    float32_t *float_sample_buffer = (float32_t*)sample_buffer;

    /* Copy output to a standard PCM buffer */
    switch (format)
    {
    case FAAD_FMT_16BIT:
        for (ch = 0; ch < channels; ch++)
        {
            for(i = 0; i < frame_len; i++)
            {
                int32_t tmp;
                real_t ftemp;

                ftemp = input[ch][i] + 0xff8000;
                ftol(ftemp, short_sample_buffer[(i*channels)+ch]);
            }
        }
        break;
    case FAAD_FMT_24BIT:
        for (ch = 0; ch < channels; ch++)
        {
            for(i = 0; i < frame_len; i++)
            {
                int_sample_buffer[(i*channels)+ch] = ROUND(input[ch][i]*(1<<8));
            }
        }
        break;
    case FAAD_FMT_32BIT:
        for (ch = 0; ch < channels; ch++)
        {
            real_t floattmp;

            for(i = 0; i < frame_len; i++)
            {
                int_sample_buffer[(i*channels)+ch] = ROUND32(input[ch][i]*(1<<16));
            }
        }
        break;
    case FAAD_FMT_FLOAT:
        for (ch = 0; ch < channels; ch++)
        {
            for(i = 0; i < frame_len; i++)
            {
                float_sample_buffer[(i*channels)+ch] = input[ch][i]*FLOAT_SCALE;
            }
        }
        break;
    }

    return sample_buffer;
}