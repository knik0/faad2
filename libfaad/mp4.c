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
** $Id: mp4.c,v 1.9 2002/08/05 20:33:38 menno Exp $
**/

#include "common.h"
#include "bits.h"
#include "mp4.h"
#include "data.h"
#include "syntax.h"

/* defines if an object type can be decoded by this library or not */
static uint8_t ObjectTypesTable[32] = {
    0, /*  0 NULL */
#ifdef MAIN_DEC
    1, /*  1 AAC Main */
#else
    0, /*  1 AAC Main */
#endif
    1, /*  2 AAC LC */
    0, /*  3 AAC SSR */
#ifdef LTP_DEC
    1, /*  4 AAC LTP */
#else
    0, /*  4 AAC LTP */
#endif
    0, /*  5 Reserved */
    0, /*  6 AAC Scalable */
    0, /*  7 TwinVQ */
    0, /*  8 CELP */
    0, /*  9 HVXC */
    0, /* 10 Reserved */
    0, /* 11 Reserved */
    0, /* 12 TTSI */
    0, /* 13 Main synthetic */
    0, /* 14 Wavetable synthesis */
    0, /* 15 General MIDI */
    0, /* 16 Algorithmic Synthesis and Audio FX */

    /* MPEG-4 Version 2 */
#ifdef ERROR_RESILIENCE
    1, /* 17 ER AAC LC */
    0, /* 18 (Reserved) */
#ifdef LTP_DEC
    1, /* 19 ER AAC LTP */
#else
    0, /* 19 ER AAC LTP */
#endif
    0, /* 20 ER AAC scalable */
    0, /* 21 ER TwinVQ */
    0, /* 22 ER BSAC */
#ifdef LD_DEC
    1, /* 23 ER AAC LD */
#else
    0, /* 23 ER AAC LD */
#endif
    0, /* 24 ER CELP */
    0, /* 25 ER HVXC */
    0, /* 26 ER HILN */
    0, /* 27 ER Parametric */
#else /* No ER defined */
    0, /* 17 ER AAC LC */
    0, /* 18 (Reserved) */
    0, /* 19 ER AAC LTP */
    0, /* 20 ER AAC scalable */
    0, /* 21 ER TwinVQ */
    0, /* 22 ER BSAC */
    0, /* 23 ER AAC LD */
    0, /* 24 ER CELP */
    0, /* 25 ER HVXC */
    0, /* 26 ER HILN */
    0, /* 27 ER Parametric */
#endif
    0, /* 28 (Reserved) */
    0, /* 29 (Reserved) */
    0, /* 30 (Reserved) */
    0  /* 31 (Reserved) */
};

/* Table 1.6.1 */
int8_t FAADAPI AudioSpecificConfig(uint8_t *pBuffer,
                                   uint32_t *samplerate,
                                   uint8_t *channels,
                                   uint8_t *sf_index,
                                   uint8_t *object_type,
                                   uint8_t *aacSectionDataResilienceFlag,
                                   uint8_t *aacScalefactorDataResilienceFlag,
                                   uint8_t *aacSpectralDataResilienceFlag,
                                   uint8_t *frameLengthFlag)
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

    if (ChannelsConfiguration > 7)
    {
        return -3;
    }

    /* get GASpecificConfig */
    if (ObjectTypeIndex == 1 || ObjectTypeIndex == 2 ||
        ObjectTypeIndex == 3 || ObjectTypeIndex == 4 ||
        ObjectTypeIndex == 6 || ObjectTypeIndex == 7)
    {
        return GASpecificConfig(&ld, channels, ObjectTypeIndex,
            aacSectionDataResilienceFlag,
            aacScalefactorDataResilienceFlag,
            aacSpectralDataResilienceFlag,
            frameLengthFlag);
#ifdef ERROR_RESILIENCE
    } else if (ObjectTypeIndex >= ER_OBJECT_START) { /* ER */
        int8_t result = GASpecificConfig(&ld, channels, ObjectTypeIndex,
            aacSectionDataResilienceFlag,
            aacScalefactorDataResilienceFlag,
            aacSpectralDataResilienceFlag,
            frameLengthFlag);
        uint8_t ep_config = (uint8_t)faad_getbits(&ld, 2
            DEBUGVAR(1,143,"parse_audio_decoder_specific_info(): epConfig"));
        if (ep_config != 0)
            return -5;

        return result;
#endif
    } else {
        return -4;
    }

    return 0;
}
