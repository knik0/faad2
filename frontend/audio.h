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
** $Id: audio.h,v 1.8 2003/07/09 15:11:25 menno Exp $
**/

#ifndef AUDIO_H_INCLUDED
#define AUDIO_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define MAXWAVESIZE     4294967040LU

#define OUTPUT_WAV 1
#define OUTPUT_RAW 2

typedef struct
{
    int outputFormat;
    FILE *sndfile;
    unsigned int fileType;
    unsigned long samplerate;
    unsigned int bits_per_sample;
    unsigned int channels;
    unsigned long total_samples;
    long channelMask;
} audio_file;

audio_file *open_audio_file(char *infile, int samplerate, int channels,
                            int outputFormat, int fileType, long channelMask);
int write_audio_file(audio_file *aufile, void *sample_buffer, int samples);
void close_audio_file(audio_file *aufile);
static int write_wav_header(audio_file *aufile);
static int write_wav_extensible_header(audio_file *aufile, long channelMask);
static int write_audio_16bit(audio_file *aufile, void *sample_buffer,
                             unsigned int samples);
static int write_audio_24bit(audio_file *aufile, void *sample_buffer,
                             unsigned int samples);
static int write_audio_32bit(audio_file *aufile, void *sample_buffer,
                             unsigned int samples);
static int write_audio_float(audio_file *aufile, void *sample_buffer,
                             unsigned int samples);


#ifdef __cplusplus
}
#endif
#endif
