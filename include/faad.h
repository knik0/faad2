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
** $Id: faad.h,v 1.15 2002/10/11 18:05:01 menno Exp $
**/

#ifndef __AACDEC_H__
#define __AACDEC_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef _WIN32
  #pragma pack(push, 8)
  #ifndef FAADAPI
    #define FAADAPI __cdecl
  #endif
#else
  #ifndef FAADAPI
    #define FAADAPI
  #endif
#endif

#define FAAD2_VERSION "1.2 beta"

/* object types for AAC */
#define MAIN       0
#define LC         1
#define SSR        2
#define LTP        3
#define ER_LC     17
#define ER_LTP    19
#define LD        23
#define DRM_ER_LC 27 /* special object type for DRM */

/* header types */
#define RAW        0
#define ADIF       1
#define ADTS       2

/* library output formats */
#define FAAD_FMT_16BIT 1
#define FAAD_FMT_24BIT 2
#define FAAD_FMT_32BIT 3
#define FAAD_FMT_FLOAT 4
#define FAAD_FMT_16BIT_DITHER 5
#define FAAD_FMT_16BIT_L_SHAPE 6
#define FAAD_FMT_16BIT_M_SHAPE 7
#define FAAD_FMT_16BIT_H_SHAPE 8

/* A decode call can eat up to FAAD_MIN_STREAMSIZE octets per decoded channel,
   so at least so much octets per channel should be available in this stream */
#define FAAD_MIN_STREAMSIZE 768 /* 6144 bits/channel */


typedef void *faacDecHandle;


typedef struct faacDecConfiguration
{
    unsigned char defObjectType;
    unsigned long defSampleRate;
    unsigned char outputFormat;
} faacDecConfiguration, *faacDecConfigurationPtr;

typedef struct faacDecFrameInfo
{
    unsigned long bytesconsumed;
    unsigned long samples;
    unsigned char channels;
    unsigned char error;
} faacDecFrameInfo;

char* FAADAPI faacDecGetErrorMessage(unsigned char errcode);

faacDecHandle FAADAPI faacDecOpen();

faacDecConfigurationPtr FAADAPI faacDecGetCurrentConfiguration(faacDecHandle hDecoder);

unsigned char FAADAPI faacDecSetConfiguration(faacDecHandle hDecoder,
                                    faacDecConfigurationPtr config);

/* Init the library based on info from the AAC file (ADTS/ADIF) */
long FAADAPI faacDecInit(faacDecHandle hDecoder,
                        unsigned char *buffer,
                        unsigned long *samplerate,
                        unsigned char *channels);

/* Init the library using a DecoderSpecificInfo */
char FAADAPI faacDecInit2(faacDecHandle hDecoder, unsigned char *pBuffer,
                         unsigned long SizeOfDecoderSpecificInfo,
                         unsigned long *samplerate, unsigned char *channels);

void FAADAPI faacDecClose(faacDecHandle hDecoder);

void* FAADAPI faacDecDecode(faacDecHandle hDecoder,
                            faacDecFrameInfo *hInfo,
                            unsigned char *buffer);

char FAADAPI AudioSpecificConfig(unsigned char *pBuffer,
                                 unsigned long *samplerate,
                                 unsigned char *channels,
                                 unsigned char *sf_index,
                                 unsigned char *object_type,
                                 unsigned char *aacSectionDataResilienceFlag,
                                 unsigned char *aacScalefactorDataResilienceFlag,
                                 unsigned char *aacSpectralDataResilienceFlag,
                                 unsigned char *frameLengthFlag);

#ifdef _WIN32
  #pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
