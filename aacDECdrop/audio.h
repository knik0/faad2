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
** $Id: audio.h,v 1.2 2002/04/14 16:31:02 menno Exp $
**/

#ifndef AUDIO_H_INCLUDED
#define AUDIO_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
    int outputFormat;
    SF_INFO sfinfo;
    SNDFILE *sndfile;
} audio_file;

audio_file *open_audio_file(char *infile, int samplerate, int channels,
                            int outputFormat, int fileType);
int write_audio_file(audio_file *aufile, void *sample_buffer, int samples);
void close_audio_file(audio_file *aufile);

#ifdef __cplusplus
}
#endif
#endif
