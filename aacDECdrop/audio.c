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
** $Id: audio.c,v 1.3 2002/07/25 12:30:57 menno Exp $
**/

#include <fcntl.h>
#include <sndfile.h>
#include <faad.h>
#include "audio.h"


audio_file *open_audio_file(char *infile, int samplerate, int channels,
                            int outputFormat, int fileType)
{
    audio_file *aufile = malloc(sizeof(audio_file));

    aufile->outputFormat = outputFormat;

    aufile->sfinfo.samplerate  = samplerate;
    switch (outputFormat)
    {
    case FAAD_FMT_16BIT:
        aufile->sfinfo.format      = ((1<<(fileType+15)) | SF_FORMAT_PCM_16);
        break;
    case FAAD_FMT_24BIT:
        aufile->sfinfo.format      = ((1<<(fileType+15)) | SF_FORMAT_PCM_24);
        break;
    case FAAD_FMT_32BIT:
        aufile->sfinfo.format      = ((1<<(fileType+15)) | SF_FORMAT_PCM_32);
        break;
    case FAAD_FMT_FLOAT:
        aufile->sfinfo.format      = ((1<<(fileType+15)) | SF_FORMAT_FLOAT);
        break;
    }
    aufile->sfinfo.channels = channels;
    aufile->sfinfo.samples  = 0;

    aufile->sndfile = sf_open(infile, SFM_WRITE, &aufile->sfinfo);

    if (aufile->sndfile == NULL)
    {
        sf_perror(NULL);
        if (aufile) free(aufile);
        return NULL;
    }

    return aufile;
}

int write_audio_file(audio_file *aufile, void *sample_buffer, int samples)
{
    switch (aufile->outputFormat)
    {
    case FAAD_FMT_16BIT:
        return sf_write_short(aufile->sndfile, (short*)sample_buffer, samples);
    case FAAD_FMT_24BIT:
    case FAAD_FMT_32BIT:
        return sf_write_int(aufile->sndfile, (int*)sample_buffer, samples);
    case FAAD_FMT_FLOAT:
        return sf_write_float(aufile->sndfile, (float*)sample_buffer, samples);
    }
}

void close_audio_file(audio_file *aufile)
{
    sf_close(aufile->sndfile);

    if (aufile) free(aufile);
}
