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
** $Id: output.c,v 1.2 2002/01/25 20:15:07 menno Exp $
**/

#ifdef __ICL
#include <mathf.h>
#else
#include <math.h>
#endif
#include "output.h"
#include "decoder.h"

#define ftol(A,B) {tmp = *(int*) & A - 0x4B7F8000; \
                   B = (short)((tmp==(short)tmp) ? tmp : (tmp>>31)^0x7FFF);}
#ifdef __ICL
#define ROUND(x) ((long)floorf((x) + 0.5f))
#else
#define ROUND(x) ((long)floor((x) + 0.5))
#endif

#define HAVE_IEEE754_FLOAT
#ifdef HAVE_IEEE754_FLOAT
#define ROUND32(x) (floattmp = (x) + (long)0x00FD8000L, *(long*)(&floattmp) - (long)0x4B7D8000L)
#else
#define ROUND32(x) ROUND(x)
#endif

#define FLOAT_SCALE (1.0f/(1<<15))


void* output_to_PCM(float **input, void *sample_buffer, int channels,
                    int format)
{
    int ch, i;

    short *short_sample_buffer = (short*)sample_buffer;
    int   *int_sample_buffer = (int*)sample_buffer;
    float *float_sample_buffer = (float*)sample_buffer;

    /* Copy output to a standard PCM buffer */
    switch (format)
    {
    case FAAD_FMT_16BIT:
        for (ch = 0; ch < channels; ch++)
        {
            for(i = 0; i < 1024; i++)
            {
                int tmp;
                float ftemp;

                ftemp = input[ch][i] + 0xff8000;
                ftol(ftemp, short_sample_buffer[(i*channels)+ch]);
            }
        }
        break;
    case FAAD_FMT_24BIT:
        for (ch = 0; ch < channels; ch++)
        {
            for(i = 0; i < 1024; i++)
            {
                int_sample_buffer[(i*channels)+ch] = ROUND(input[ch][i]*(1<<8));
            }
        }
        break;
    case FAAD_FMT_32BIT:
        for (ch = 0; ch < channels; ch++)
        {
            float floattmp;

            for(i = 0; i < 1024; i++)
            {
                int_sample_buffer[(i*channels)+ch] = ROUND32(input[ch][i]*(1<<16));
            }
        }
        break;
    case FAAD_FMT_FLOAT:
        for (ch = 0; ch < channels; ch++)
        {
            for(i = 0; i < 1024; i++)
            {
                float_sample_buffer[(i*channels)+ch] = input[ch][i]*FLOAT_SCALE;
            }
        }
        break;
    }

    return sample_buffer;
}