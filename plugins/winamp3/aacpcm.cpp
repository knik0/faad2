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
** $Id: aacpcm.cpp,v 1.1 2002/01/21 20:38:34 menno Exp $
**/

#include <stdio.h>
#include "aacpcm.h"

AacPcm::AacPcm()
{
    hDecoder = faacDecOpen();
    buffercount = 0;
    bytecount = 0;
    init_called = 0;

    samplerate = 44100;
    bps = 16;
    nch = 2;
}

AacPcm::~AacPcm()
{
    faacDecClose(hDecoder);
}

int AacPcm::getInfos(MediaInfo *infos)
{
    infos->setTitle(Std::filename(infos->getFilename()));
    infos->setInfo(StringPrintf("%ihz %ibps %dch", samplerate, bps, nch));

    return 0;
}

int AacPcm::processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch)
{
    unsigned long sr, ch;
    short *samplebuffer;
    faacDecFrameInfo frameInfo;
    int k, last_frame = 0;

    svc_fileReader *reader = infos->getReader();
    if (!reader)
        return 0;

    int eof = 0;

    // I assume that it lets me read from the beginning of the file here
    if (!init_called)
    {
        buffercount = 0;
        reader->read(buffer, 768*2);
        bytecount += 768*2;

        buffercount = faacDecInit(hDecoder, buffer, &sr, &ch);
        samplerate = sr;
        nch = ch;

        init_called = 1;
    }

    if (buffercount > 0)
    {
        bytecount += buffercount;

        for (k = 0; k < (768*2 - buffercount); k++)
            buffer[k] = buffer[k + buffercount];

        reader->read(buffer + (768*2) - buffercount, buffercount);
        buffercount = 0;
    }

    samplebuffer = faacDecDecode(hDecoder, &frameInfo, buffer);
    if (frameInfo.error)
    {
        last_frame = 1;
    }

    buffercount += frameInfo.bytesconsumed;
    bytecount += frameInfo.bytesconsumed;

    if (bytecount >= 2*reader->getLength())
        last_frame = 1;


    ChunkInfosI *ci = new ChunkInfosI();
    ci->addInfo("srate", samplerate);
    ci->addInfo("bps", bps);
    ci->addInfo("nch", frameInfo.channels);

    chunk_list->setChunk("PCM", samplebuffer, 2048*frameInfo.channels, ci);

    if (last_frame)
        return 0;
    return 1;
}
