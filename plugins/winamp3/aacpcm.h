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
** $Id: aacpcm.h,v 1.1 2002/01/21 20:38:34 menno Exp $
**/

#ifndef _AACPCM_H
#define _AACPCM_H

#include <faad.h>
#include "sdk/studio/services/svc_mediaconverter.h"
#include "sdk/studio/services/servicei.h"
#include "sdk/studio/corecb.h"
#include "sdk/studio/wac.h"
#include "sdk/attribs/cfgitemi.h"
#include "sdk/attribs/attrint.h"

class AacPcm : public svc_mediaConverterI
{
public:
    AacPcm();
    virtual ~AacPcm();

    // service
    static const char *getServiceName() { return "AAC to PCM converter"; }

    virtual int canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype) {
        if(name && !STRICMP(Std::extension(name),"aac")) return 1; // only accepts *.aac files
        return 0;
    }
    virtual const char *getConverterTo() { return "PCM"; }

    virtual int getInfos(MediaInfo *infos);

    virtual int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch);

    virtual int getLatency(void) { return 0; }

    // callbacks
    virtual int corecb_onSeeked(int newpos) { return 0; } // do nothing on seek

private:
    faacDecHandle hDecoder;

    unsigned char buffer[768*2];
    long buffercount, bytecount;
    int init_called;

    int samplerate, bps, nch;
};
#endif
