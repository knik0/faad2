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
**
** $Id: mp4util.c,v 1.1 2003/11/21 15:08:48 menno Exp $
**/

#include "mp4ff.h"

int32_t mp4ff_read_data(mp4ff_t *f, int8_t *data, int32_t size)
{
    int32_t result = 1;

    result = f->stream->read(f->stream->user_data, data, size);

    f->current_position += size;

    return result;
}

int32_t mp4ff_set_position(mp4ff_t *f, int32_t position)
{
    f->stream->seek(f->stream->user_data, position);
    f->current_position = position;

    return 0;
}

int32_t mp4ff_position(mp4ff_t *f)
{
    return f->current_position;
}

int32_t mp4ff_read_int32(mp4ff_t *f)
{
    uint32_t result;
    uint32_t a, b, c, d;
    int8_t data[4];
    
    mp4ff_read_data(f, data, 4);
    a = (uint8_t)data[0];
    b = (uint8_t)data[1];
    c = (uint8_t)data[2];
    d = (uint8_t)data[3];

    result = (a<<24) | (b<<16) | (c<<8) | d;
    return (int32_t)result;
}

int32_t mp4ff_read_int24(mp4ff_t *f)
{
    uint32_t result;
    uint32_t a, b, c;
    int8_t data[4];
    
    mp4ff_read_data(f, data, 3);
    a = (uint8_t)data[0];
    b = (uint8_t)data[1];
    c = (uint8_t)data[2];

    result = (a<<16) | (b<<8) | c;
    return (int32_t)result;
}

int16_t mp4ff_read_int16(mp4ff_t *f)
{
    uint32_t result;
    uint32_t a, b;
    int8_t data[2];
    
    mp4ff_read_data(f, data, 2);
    a = (uint8_t)data[0];
    b = (uint8_t)data[1];

    result = (a<<8) | b;
    return (int16_t)result;
}

int8_t mp4ff_read_char(mp4ff_t *f)
{
    int8_t output;
    mp4ff_read_data(f, &output, 1);
    return output;
}

uint32_t mp4ff_read_mp4_descr_length(mp4ff_t *f)
{
    uint8_t b;
    uint8_t numBytes = 0;
    uint32_t length = 0;

    do
    {
        b = mp4ff_read_char(f);
        numBytes++;
        length = (length << 7) | (b & 0x7F);
    } while ((b & 0x80) && numBytes < 4);

    return length;
}
