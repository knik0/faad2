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
** $Id: utils.c,v 1.4 2002/11/01 11:19:36 menno Exp $
**/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mp4.h>
#include <faad.h>
#include "utils.h"

int StringComp(char const *str1, char const *str2, unsigned long len)
{
    signed int c1 = 0, c2 = 0;

    while (len--)
    {
        c1 = tolower(*str1++);
        c2 = tolower(*str2++);

        if (c1 == 0 || c1 != c2)
            break;
    }

    return c1 - c2;
}

int GetAACTrack(MP4FileHandle infile)
{
    /* find AAC track */
    int i, rc;
	int numTracks = MP4GetNumberOfTracks(infile, NULL, 0);

	for (i = 0; i < numTracks; i++)
    {
        MP4TrackId trackId = MP4FindTrackId(infile, i, NULL, 0);
        const char* trackType = MP4GetTrackType(infile, trackId);

        if (!strcmp(trackType, MP4_AUDIO_TRACK_TYPE))
        {
            unsigned char *buff = NULL;
            int buff_size = 0;
            unsigned char dummy2_8, dummy3_8, dummy4_8, dummy5_8, dummy6_8,
                dummy7_8, dummy8_8;
            unsigned int dummy1_32;
            MP4GetTrackESConfiguration(infile, trackId, &buff, &buff_size);

            if (buff)
            {
                rc = AudioSpecificConfig(buff, buff_size, &dummy1_32, &dummy2_8,
                    &dummy3_8, &dummy4_8, &dummy5_8, &dummy6_8, &dummy7_8, &dummy8_8);
                free(buff);

                if (rc < 0)
                    return -1;
                return trackId;
            }
        }
    }

    /* can't decode this */
    return -1;
}

int GetAudioTrack(MP4FileHandle infile)
{
    /* find AAC track */
    int i;
	int numTracks = MP4GetNumberOfTracks(infile, NULL, 0);

	for (i = 0; i < numTracks; i++)
    {
        MP4TrackId trackId = MP4FindTrackId(infile, i, NULL, 0);
        const char* trackType = MP4GetTrackType(infile, trackId);

        if (!strcmp(trackType, MP4_AUDIO_TRACK_TYPE))
        {
            return trackId;
        }
    }

    /* can't decode this */
    return -1;
}

int GetVideoTrack(MP4FileHandle infile)
{
    /* find AAC track */
    int i;
	int numTracks = MP4GetNumberOfTracks(infile, NULL, 0);

	for (i = 0; i < numTracks; i++)
    {
        MP4TrackId trackId = MP4FindTrackId(infile, i, NULL, 0);
        const char* trackType = MP4GetTrackType(infile, trackId);

        if (!strcmp(trackType, MP4_VIDEO_TRACK_TYPE))
        {
            return trackId;
        }
    }

    /* can't decode this */
    return -1;
}

LPTSTR PathFindFileName(LPCTSTR pPath)
{
    LPCTSTR pT;

    for (pT = pPath; *pPath; pPath = CharNext(pPath)) {
        if ((pPath[0] == TEXT('\\') || pPath[0] == TEXT(':')) && pPath[1] && (pPath[1] != TEXT('\\')))
            pT = pPath + 1;
    }

    return (LPTSTR)pT;   // const -> non const
}

char *convert3in4to3in3(void *sample_buffer, int samples)
{
    int i;
    long *sample_buffer24 = (long*)sample_buffer;
    char *data = malloc(samples*3*sizeof(char));

    for (i = 0; i < samples; i++)
    {
        data[i*3] = sample_buffer24[i] & 0xFF;
        data[i*3+1] = (sample_buffer24[i] >> 8) & 0xFF;
        data[i*3+2] = (sample_buffer24[i] >> 16) & 0xFF;
    }

    return data;
}
