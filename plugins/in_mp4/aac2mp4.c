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
** $Id: aac2mp4.c,v 1.2 2002/09/03 21:22:53 menno Exp $
**/

#include <mpeg4ip.h>
#include <mp4.h>
#include "mp4av.h"

#include "aac2mp4.h"

int covert_aac_to_mp4(char *inputFileName, char *mp4FileName)
{
    int Mp4TimeScale = 90000;
    int allMpeg4Streams = 0;
    MP4FileHandle mp4File;
    FILE* inFile;
    char *type;
    MP4TrackId createdTrackId = MP4_INVALID_TRACK_ID;

    mp4File = MP4Create(mp4FileName, 0, 0, 0);
    if (mp4File)
    {
        MP4SetTimeScale(mp4File, Mp4TimeScale);
    } else {
        return 1;
    }

    inFile = fopen(inputFileName, "rb");

	if (inFile == NULL)
    {
        MP4Close(mp4File);
        return 2;
    }

    createdTrackId = AacCreator(mp4File, inFile);

    if (createdTrackId == MP4_INVALID_TRACK_ID)
    {
        fclose(inFile);
        MP4Close(mp4File);
        return 3;
    }

    type = MP4GetTrackType(mp4File, createdTrackId);

    if (!strcmp(type, MP4_AUDIO_TRACK_TYPE))
    {
        allMpeg4Streams &=
            (MP4GetTrackAudioType(mp4File, createdTrackId)
            == MP4_MPEG4_AUDIO_TYPE);
    }

    if (inFile)
    {
        fclose(inFile);
    }

    MP4Close(mp4File);
    MP4MakeIsmaCompliant(mp4FileName, 0, allMpeg4Streams);

    return 0;
}