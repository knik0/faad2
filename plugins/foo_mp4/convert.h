/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003 M. Bakker, Ahead Software AG, http://www.nero.com
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
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**/

#ifndef CONVERT_H__
#define CONVERT_H__

#include <mpeg4ip.h>
#include <mp4.h>
#include <mp4av.h>
#include <faad.h>
#include "foobar2000/SDK/foobar2000.h"

#define ADTS_HEADER_MAX_SIZE 10 // bytes

class converter
{
public:
    converter(const char *in_file, const char *out_file, const file_info *infos=0);
    ~converter();

    bool aac_to_mp4();
    bool mp4_to_aac();

private:
    reader *in, *out;
    file_info_i_full info;
    MP4FileHandle mp4File;
    u_int8_t firstHeader[ADTS_HEADER_MAX_SIZE];

    // AAC to MP4 conversion
    bool LoadNextAdtsHeader(u_int8_t *hdr);
    bool LoadNextAacFrame(u_int8_t *pBuf, u_int32_t *pBufSize, bool stripAdts);
    bool GetFirstHeader();
    MP4TrackId ConvertAAC();

    //MP4 to AAC conversion
    bool ExtractTrack(MP4TrackId trackId);

    bool WriteTagMP4();
    bool WriteTagAAC();

    // MP4 I/O callbacks
    static unsigned __int32 open_cb ( const char *pName, const char *mode, void *userData ) { return 1; }

    static void close_cb ( void *userData ) { return; }

    static unsigned __int32 read_cb ( void *pBuffer, unsigned int nBytesToRead, void *userData )
    {
        reader *r = (reader *)userData;
        return r->read ( pBuffer, nBytesToRead );
    }

    static unsigned __int32 write_cb ( void *pBuffer, unsigned int nBytesToWrite, void *userData )
    {
        reader *r = (reader *)userData;
        return r->write ( pBuffer, nBytesToWrite );
    }

    static __int64 getpos_cb ( void *userData )
    {
        reader *r = (reader *)userData;
        return r->get_position();
    }

    static __int32 setpos_cb ( unsigned __int32 pos, void *userData )
    {
        reader *r = (reader *)userData;
        return !r->seek ( pos );
    }

    static __int64 filesize_cb ( void *userData )
    {
        reader *r = (reader *)userData;
        return r->get_length();
    }
};

#endif
