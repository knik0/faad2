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
** $Id: output.c,v 1.13 2002/09/13 13:08:45 menno Exp $
**/

#include "common.h"

#include "output.h"
#include "decoder.h"

#ifndef FIXED_POINT

#include "dither.h"


#define ftol(A,B) {tmp = *(int32_t*) & A - 0x4B7F8000; \
                   B = (int16_t)((tmp==(int16_t)tmp) ? tmp : (tmp>>31)^0x7FFF);}

#define ROUND(x) ((x >= 0) ? (int32_t)floor((x) + 0.5) : (int32_t)ceil((x) + 0.5))

#define ROUND32(x) ROUND(x)

#define ROUND64(x) (doubletmp = (x) + Dither.Add + (int64_t)0x001FFFFD80000000L, *(int64_t*)(&doubletmp) - (int64_t)0x433FFFFD80000000L)

#define FLOAT_SCALE (1.0f/(1<<15))

dither_t Dither;
double doubletmp;

void* output_to_PCM(real_t **input, void *sample_buffer, uint8_t channels,
                    uint16_t frame_len, uint8_t format)
{
    uint8_t ch;
    uint16_t i, j = 0;

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
    case FAAD_FMT_16BIT_DITHER:
        for (ch = 0; ch < channels; ch++)
        {
            for(i = 0; i < frame_len; i++, j++)
            {
                double Sum = input[ch][i] * 65535.f;
                int64_t val;
                if(j > 31)
                   j = 0;
                val = dither_output(1, 0, j, Sum, ch) / 65536;
                if (val > (1<<15)-1)
                    val = (1<<15)-1;
                else if (val < -(1<<15))
                    val = -(1<<15);
                short_sample_buffer[(i*channels)+ch] = (int16_t)val;
            }
        }
        break;
    case FAAD_FMT_16BIT_L_SHAPE:
    case FAAD_FMT_16BIT_M_SHAPE:
    case FAAD_FMT_16BIT_H_SHAPE:
        for (ch = 0; ch < channels; ch++)
        {
            for(i = 0; i < frame_len; i++, j++)
            {
                double Sum = input[ch][i] * 65535.f;
                int64_t val;
                if(j > 31)
                   j = 0;
                val = dither_output(1, 1, j, Sum, ch) / 65536;
                if (val > (1<<15)-1)
                    val = (1<<15)-1;
                else if (val < -(1<<15))
                    val = -(1<<15);
                short_sample_buffer[(i*channels)+ch] = (int16_t)val;
            }
        }
        break;
    case FAAD_FMT_24BIT:
        for (ch = 0; ch < channels; ch++)
        {
            for(i = 0; i < frame_len; i++)
            {
                if (input[ch][i] > (1<<15)-1)
                    input[ch][i] = (1<<15)-1;
                else if (input[ch][i] < -(1<<15))
                    input[ch][i] = -(1<<15);
                int_sample_buffer[(i*channels)+ch] = ROUND(input[ch][i]*(1<<8));
            }
        }
        break;
    case FAAD_FMT_32BIT:
        for (ch = 0; ch < channels; ch++)
        {
            for(i = 0; i < frame_len; i++)
            {
                if (input[ch][i] > (1<<15)-1)
                    input[ch][i] = (1<<15)-1;
                else if (input[ch][i] < -(1<<15))
                    input[ch][i] = -(1<<15);
                int_sample_buffer[(i*channels)+ch] = ROUND32(input[ch][i]*(1<<16));
            }
        }
        break;
    case FAAD_FMT_FLOAT:
        for (ch = 0; ch < channels; ch++)
        {
            for(i = 0; i < frame_len; i++)
            {
                if (input[ch][i] > (1<<15)-1)
                    input[ch][i] = (1<<15)-1;
                else if (input[ch][i] < -(1<<15))
                    input[ch][i] = -(1<<15);
                float_sample_buffer[(i*channels)+ch] = input[ch][i]*FLOAT_SCALE;
            }
        }
        break;
    }

    return sample_buffer;
}


/* Dither output */
static int64_t dither_output(uint8_t dithering, uint8_t shapingtype, uint16_t i, double Sum, uint8_t k)
{
    double Sum2;
    int64_t val;
    if(dithering)
    {
        if(!shapingtype)
        {
            double tmp = Random_Equi(Dither.Dither);
            Sum2 = tmp - Dither.LastRandomNumber[k];
            Dither.LastRandomNumber[k] = tmp;
            Sum2 = Sum += Sum2;
            val = ROUND64(Sum2)&Dither.Mask;
        } else {
            Sum2 = Random_Triangular(Dither.Dither) - scalar16(Dither.DitherHistory[k], Dither.FilterCoeff + i);
            Sum += Dither.DitherHistory[k][(-1-i)&15] = Sum2;
            Sum2 = Sum + scalar16(Dither.ErrorHistory[k], Dither.FilterCoeff + i );
            val = ROUND64(Sum2)&Dither.Mask;
            Dither.ErrorHistory[k][(-1-i)&15] = (float)(Sum - val);
        }
        return val;
    }
    else
        return ROUND64 (Sum);
}

#else

void* output_to_PCM(real_t **input, void *sample_buffer, uint8_t channels,
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
