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
** $Id: aacinfo.c,v 1.1 2002/08/14 17:55:49 menno Exp $
**/

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include "aacinfo.h"
#include "utils.h"

#define ADIF_MAX_SIZE 30 /* Should be enough */
#define ADTS_MAX_SIZE 10 /* Should be enough */

static int sample_rates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000};

static int read_ADIF_header(FILE *file, faadAACInfo *info)
{
    int bitstream;
    unsigned char buffer[ADIF_MAX_SIZE];
    int skip_size = 0;
    int sf_idx;

    /* Get ADIF header data */
    info->headertype = 1;

    if (fread(buffer, 1, ADIF_MAX_SIZE, file) != ADIF_MAX_SIZE)
        return -1;

    /* copyright string */
    if(buffer[0] & 0x80)
        skip_size += 9; /* skip 9 bytes */

    bitstream = buffer[0 + skip_size] & 0x10;
    info->bitrate = ((unsigned int)(buffer[0 + skip_size] & 0x0F)<<19)|
        ((unsigned int)buffer[1 + skip_size]<<11)|
        ((unsigned int)buffer[2 + skip_size]<<3)|
        ((unsigned int)buffer[3 + skip_size] & 0xE0);

    if (bitstream == 0)
    {
        info->object_type =  ((buffer[6 + skip_size]&0x01)<<1)|((buffer[7 + skip_size]&0x80)>>7);
        sf_idx = (buffer[7 + skip_size]&0x78)>>3;
    } else {
        info->object_type = (buffer[4 + skip_size] & 0x18)>>3;
        sf_idx = ((buffer[4 + skip_size] & 0x07)<<1)|((buffer[5 + skip_size] & 0x80)>>7);
    }
    info->sampling_rate = sample_rates[sf_idx];

    return 0;
}

static int read_ADTS_header(FILE *file, faadAACInfo *info, unsigned long **seek_table,
                            int *seek_table_len, int use_seek_table)
{
    /* Get ADTS header data */
    unsigned char buffer[ADTS_MAX_SIZE];
    int frames, framesinsec=0, t_framelength = 0, frame_length, sr_idx = 0, ID;
    int second = 0, pos;
    float frames_per_sec = 0;
    unsigned long bytes;
    unsigned long *tmp_seek_table = NULL;

    info->headertype = 2;

    /* Read all frames to ensure correct time and bitrate */
    for (frames = 0; /* */; frames++, framesinsec++)
    {
        bytes = fread(buffer, 1, ADTS_MAX_SIZE, file);

        if (bytes != ADTS_MAX_SIZE)
            break;

        /* check syncword */
        if (!((buffer[0] == 0xFF)&&((buffer[1] & 0xF6) == 0xF0)))
            break;

        if (!frames)
        {
            /* fixed ADTS header is the same for every frame, so we read it only once */
            /* Syncword found, proceed to read in the fixed ADTS header */
            ID = buffer[1] & 0x08;
            info->object_type = (buffer[2]&0xC0)>>6;
            sr_idx = (buffer[2]&0x3C)>>2;
            info->channels = ((buffer[2]&0x01)<<2)|((buffer[3]&0xC0)>>6);

            frames_per_sec = sample_rates[sr_idx] / 1024.f;
        }

        /* ...and the variable ADTS header */
        if (ID == 0)
        {
            info->version = 4;
            frame_length = (((unsigned int)buffer[4]) << 5) |
                ((unsigned int)buffer[5] >> 3);
        } else { /* MPEG-2 */
            info->version = 2;
            frame_length = ((((unsigned int)buffer[3] & 0x3)) << 11)
                | (((unsigned int)buffer[4]) << 3) | (buffer[5] >> 5);
        }

        t_framelength += frame_length;

        if (framesinsec > 43)
            framesinsec = 0;

        pos = ftell(file) - ADTS_MAX_SIZE;

        if (framesinsec == 0 && use_seek_table)
        {
            tmp_seek_table = (unsigned long*)realloc(tmp_seek_table, (second+1) * sizeof(unsigned long));
            tmp_seek_table[second] = pos;
        }
        if (framesinsec == 0)
            second++;

        fseek(file, frame_length - ADTS_MAX_SIZE, SEEK_CUR);
    }

	if(seek_table_len)
	{
		*seek_table_len = second;
		*seek_table = tmp_seek_table;
	}

    if (frames > 0)
    {
        info->sampling_rate = sample_rates[sr_idx];
        info->bitrate = (int)(((t_framelength / frames) * (info->sampling_rate/1024.0)) +0.5)*8;
        info->length = (int)((float)(frames/frames_per_sec))*1000;
    } else {
        info->sampling_rate = 4;
        info->bitrate = 128000;
        info->length = 0;
        info->channels = 0;
    }

    return 0;
}

int get_AAC_format(char *filename, faadAACInfo *info,
                   unsigned long **seek_table, int *seek_table_len,
                   int use_seek_table)
{
    unsigned long tagsize;
    FILE *file;
	char buffer[10];
    unsigned long file_len;
    unsigned char adxx_id[5];
    unsigned long tmp;

    memset(info, 0, sizeof(faadAACInfo));

    file = fopen(filename, "rb");

    if(file == NULL)
        return -1;

    fseek(file, 0, SEEK_END);
    file_len = ftell(file);
    fseek(file, 0, SEEK_SET);

    /* Skip the tag, if it's there */
    tmp = fread(buffer, 10, 1, file);

    if (StringComp(buffer, "ID3", 3) == 0)
	{
        /* high bit is not used */
        tagsize = (buffer[6] << 21) | (buffer[7] << 14) |
            (buffer[8] <<  7) | (buffer[9] <<  0);

        fseek(file, tagsize, SEEK_CUR);
        tagsize += 10;
    } else {
        tagsize = 0;
        fseek(file, 0, SEEK_SET);
    }

    if (file_len)
        file_len -= tagsize;

    tmp = fread(adxx_id, 2, 1, file);
    adxx_id[5-1] = 0;
    info->length = 0;

    /* Determine the header type of the file, check the first two bytes */
    if (StringComp(adxx_id, "AD", 2) == 0)
    {
        /* We think its an ADIF header, but check the rest just to make sure */
		tmp = fread(adxx_id + 2, 2, 1, file);

        if (StringComp(adxx_id, "ADIF", 4) == 0)
        {
            read_ADIF_header(file, info);

            info->length = (int)((float)file_len*8000.0/((float)info->bitrate));
        }
    } else {
        /* No ADIF, check for ADTS header */
        if ((adxx_id[0] == 0xFF)&&((adxx_id[1] & 0xF6) == 0xF0))
        {
            /* ADTS  header located */
            fseek(file, tagsize, SEEK_SET);

            read_ADTS_header(file, info, seek_table, seek_table_len,
                use_seek_table);
        } else {
            /* Unknown/headerless AAC file, assume format: */
            info->version = 2;
            info->bitrate = 128000;
            info->sampling_rate = 44100;
            info->channels = 2;
            info->headertype = 0;
            info->object_type = 1;
        }
    }

    fclose(file);

    return 0;
}

