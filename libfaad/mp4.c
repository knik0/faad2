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
** $Id: mp4.c,v 1.1 2002/01/20 16:57:55 menno Exp $
**/

#include "bits.h"
#include "mp4.h"
#include "data.h"
#include "syntax.h"

/* defines if an object type can be decoded by this library or not */
static unsigned long ObjectTypesTable[32] = {
    0, /* NULL */
    1, /* AAC Main */
    1, /* AAC LC */
    0, /* AAC SSR */
    1, /* AAC LTP */
    0, /* Reserved */
    0, /* AAC Scalable */
    0, /* TwinVQ */
    0, /* CELP */
    0, /* HVXC */
    0, /* Reserved */
    0, /* Reserved */
    0, /* TTSI */
    0, /* Main synthetic */
    0, /* Wavetable synthesis */
    0, /* General MIDI */
    0  /* Algorithmic Synthesis and Audio FX */
};

/* Table 1.6.1 */
int FAADAPI AudioSpecificConfig(unsigned char *pBuffer,
                                unsigned long *samplerate,
                                unsigned long *channels,
                                unsigned long *sf_index,
                                unsigned long *object_type)
{
    bitfile ld;
    unsigned long ObjectTypeIndex, SamplingFrequencyIndex,
        ChannelsConfiguration;

    faad_initbits(&ld, pBuffer);
    faad_byte_align(&ld);

    ObjectTypeIndex = faad_getbits(&ld, 5
        DEBUGVAR(1,1,"parse_audio_decoder_specific_info(): ObjectTypeIndex"));

    SamplingFrequencyIndex = faad_getbits(&ld, 4
        DEBUGVAR(1,2,"parse_audio_decoder_specific_info(): SamplingFrequencyIndex"));

    ChannelsConfiguration = faad_getbits(&ld, 4
        DEBUGVAR(1,3,"parse_audio_decoder_specific_info(): ChannelsConfiguration"));

    *samplerate = sample_rates[SamplingFrequencyIndex];

    *channels = ChannelsConfiguration;

    *sf_index = SamplingFrequencyIndex;
    *object_type = ObjectTypeIndex;


    if (ObjectTypesTable[ObjectTypeIndex] != 1)
    {
        return -1;
    }

    if (*samplerate == 0)
    {
        return -2;
    }

    if(ChannelsConfiguration > 7)
    {
        return -3;
    }

    /* get GASpecificConfig */
    if (ObjectTypeIndex == 1 || ObjectTypeIndex == 2 ||
        ObjectTypeIndex == 3 || ObjectTypeIndex == 4 ||
        ObjectTypeIndex == 6 || ObjectTypeIndex == 7 )
    {
        return GASpecificConfig(&ld, channels);
    } else {
        return -4;
    }

    return 0;
}
