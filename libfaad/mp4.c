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
** $Id: mp4.c,v 1.3 2002/02/25 19:58:33 menno Exp $
**/

#include "common.h"
#include "bits.h"
#include "mp4.h"
#include "data.h"
#include "syntax.h"

/* defines if an object type can be decoded by this library or not */
static uint8_t ObjectTypesTable[32] = {
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
    0, /* Algorithmic Synthesis and Audio FX */

    /* MPEG-4 Version 2 */
    0, /* ER AAC LC */
    0, /* (Reserved) */
    0, /* ER AAC LTP */
    0, /* ER AAC scalable */
    0, /* ER TwinVQ */
    0, /* ER BSAC */
    1, /* ER AAC LD */     /* !!! Supported, but only with ER turned off !!! */
    0, /* ER CELP */
    0, /* ER HVXC */
    0, /* ER HILN */
    0, /* ER Parametric */
    0, /* (Reserved) */
    0, /* (Reserved) */
    0, /* (Reserved) */
    0  /* (Reserved) */
};

/* Table 1.6.1 */
int8_t FAADAPI AudioSpecificConfig(uint8_t *pBuffer,
                                   uint32_t *samplerate,
                                   uint8_t *channels,
                                   uint8_t *sf_index,
                                   uint8_t *object_type)
{
    bitfile ld;
    uint8_t ObjectTypeIndex, SamplingFrequencyIndex, ChannelsConfiguration;

    faad_initbits(&ld, pBuffer);
    faad_byte_align(&ld);

    ObjectTypeIndex = (uint8_t)faad_getbits(&ld, 5
        DEBUGVAR(1,1,"parse_audio_decoder_specific_info(): ObjectTypeIndex"));

    SamplingFrequencyIndex = (uint8_t)faad_getbits(&ld, 4
        DEBUGVAR(1,2,"parse_audio_decoder_specific_info(): SamplingFrequencyIndex"));

    ChannelsConfiguration = (uint8_t)faad_getbits(&ld, 4
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
        ObjectTypeIndex == 6 || ObjectTypeIndex == 7)
    {
        return GASpecificConfig(&ld, channels, ObjectTypeIndex);
    } else if (ObjectTypeIndex == 23) { /* ER AAC LD */
        uint8_t result = GASpecificConfig(&ld, channels, ObjectTypeIndex);
        uint8_t ep_config = (uint8_t)faad_getbits(&ld, 2
            DEBUGVAR(1,143,"parse_audio_decoder_specific_info(): epConfig"));
        if (ep_config != 0)
            return -5;

        return result;
    } else {
        return -4;
    }

    return 0;
}
