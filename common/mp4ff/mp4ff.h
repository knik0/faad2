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
** $Id: mp4ff.h,v 1.5 2003/11/21 18:20:57 menno Exp $
**/

#ifndef MP4FF_H
#define MP4FF_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define MAX_TRACKS 1024
#define TRACK_UNKNOWN 0
#define TRACK_AUDIO   1
#define TRACK_VIDEO   2
#define TRACK_SYSTEM  3


#define SUBATOMIC 128

/* atoms without subatoms */
#define ATOM_FTYP 129
#define ATOM_MDAT 130
#define ATOM_MVHD 131
#define ATOM_TKHD 132
#define ATOM_TREF 133
#define ATOM_MDHD 134
#define ATOM_VMHD 135
#define ATOM_SMHD 136
#define ATOM_HMHD 137
#define ATOM_STSD 138
#define ATOM_STTS 139
#define ATOM_STSZ 140
#define ATOM_STZ2 141
#define ATOM_STCO 142
#define ATOM_STSC 143
#define ATOM_MP4A 144
#define ATOM_MP4V 145
#define ATOM_MP4S 146
#define ATOM_ESDS 147

#define ATOM_UNKNOWN 255
#define ATOM_FREE ATOM_UNKNOWN
#define ATOM_SKIP ATOM_UNKNOWN

/* atoms with subatoms */
#define ATOM_MOOV 1
#define ATOM_TRAK 2
#define ATOM_EDTS 3
#define ATOM_MDIA 4
#define ATOM_MINF 5
#define ATOM_STBL 6

#ifdef _WIN32
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#else
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#define u_int8_t uint8_t
#define u_int16_t uint16_t
#define u_int32_t uint32_t
#define u_int64_t uint64_t
#endif
#endif

/* file callback structure */
typedef struct
{
    int32_t (*read)(void *udata, void *buffer, int32_t length);
    int32_t (*seek)(void *udata, int32_t position);
    void *user_data;
} mp4ff_callback_t;

typedef struct
{
    int32_t type;
    int32_t channelCount;
    int32_t sampleSize;
    int32_t sampleRate;

    /* stsd */
    int32_t stsd_entry_count;

    /* stsz */
	int32_t stsz_sample_size;
	int32_t stsz_sample_count;
    int32_t *stsz_table;

    /* stts */
	int32_t stts_entry_count;
    int32_t *stts_sample_count;
    int32_t *stts_sample_delta;

    /* stsc */
	int32_t stsc_entry_count;
    int32_t *stsc_first_chunk;
    int32_t *stsc_samples_per_chunk;
    int32_t *stsc_sample_desc_index;

    /* stsc */
	int32_t stco_entry_count;
    int32_t *stco_chunk_offset;

    /* esde */
    uint8_t *decoderConfig;
    int32_t decoderConfigLen;

} mp4ff_track_t;

/* mp4 main file structure */
typedef struct
{
    /* stream to read from */
	mp4ff_callback_t *stream;
    int32_t current_position;

    int32_t moov_read;
    int32_t moov_offset;
    int32_t moov_size;
    int32_t mdat_read;
    int32_t mdat_offset;
    int32_t mdat_size;

    /* mvhd */
    int32_t time_scale;
    int32_t duration;

    /* incremental track index while reading the file */
    int32_t total_tracks;

    /* track data */
    mp4ff_track_t *track[MAX_TRACKS];
} mp4ff_t;




/* mp4util.c */
int32_t mp4ff_read_data(mp4ff_t *f, int8_t *data, int32_t size);
int32_t mp4ff_read_int32(mp4ff_t *f);
int32_t mp4ff_read_int24(mp4ff_t *f);
int16_t mp4ff_read_int16(mp4ff_t *f);
int8_t mp4ff_read_char(mp4ff_t *f);
uint32_t mp4ff_read_mp4_descr_length(mp4ff_t *f);
int32_t mp4ff_position(mp4ff_t *f);
int32_t mp4ff_set_position(mp4ff_t *f, int32_t position);

/* mp4atom.c */
int32_t mp4ff_atom_get_size(int8_t *data);
int32_t mp4ff_atom_compare(int8_t a1, int8_t b1, int8_t c1, int8_t d1,
                           int8_t a2, int8_t b2, int8_t c2, int8_t d2);
uint8_t mp4ff_atom_name_to_type(int8_t a, int8_t b, int8_t c, int8_t d);
int32_t mp4ff_atom_read_header(mp4ff_t *f, uint8_t *atom_type);
int32_t mp4ff_read_stsz(mp4ff_t *f);
int32_t mp4ff_read_esds(mp4ff_t *f);
int32_t mp4ff_read_mp4a(mp4ff_t *f);
int32_t mp4ff_read_stsd(mp4ff_t *f);
int32_t mp4ff_read_stsc(mp4ff_t *f);
int32_t mp4ff_read_stco(mp4ff_t *f);
int32_t mp4ff_read_stts(mp4ff_t *f);
int32_t mp4ff_atom_read(mp4ff_t *f, int32_t size, uint8_t atom_type);

/* mp4sample.c */
int32_t mp4ff_chunk_of_sample(mp4ff_t *f, int32_t track, int32_t sample,
                              int32_t *chunk_sample, int32_t *chunk);
int32_t mp4ff_chunk_to_offset(mp4ff_t *f, int32_t track, int32_t chunk);
int32_t mp4ff_sample_range_size(mp4ff_t *f, int32_t track, int32_t chunk_sample, int32_t sample);
int32_t mp4ff_sample_to_offset(mp4ff_t *f, int32_t track, int32_t sample);
int32_t mp4ff_audio_frame_size(mp4ff_t *f, int32_t track, int32_t sample);
int32_t mp4ff_set_sample_position(mp4ff_t *f, int32_t track, int32_t sample);


/* mp4ff.c */
mp4ff_t *mp4ff_open_read(mp4ff_callback_t *f);
void mp4ff_close(mp4ff_t *ff);
void mp4ff_track_add(mp4ff_t *f);
int32_t parse_sub_atoms(mp4ff_t *f, int32_t total_size);
int32_t parse_atoms(mp4ff_t *f);
int32_t mp4ff_get_sample_duration(mp4ff_t *f, int32_t track, int32_t sample);
int32_t mp4ff_read_sample(mp4ff_t *f, int32_t track, int32_t sample, uint8_t **audio_buffer,  uint32_t *bytes);
int32_t mp4ff_get_decoder_config(mp4ff_t *f, int32_t track, uint8_t** ppBuf, uint32_t* pBufSize);
int32_t mp4ff_total_tracks(mp4ff_t *f);
int32_t mp4ff_time_scale(mp4ff_t *f, int32_t track);
int32_t mp4ff_num_samples(mp4ff_t *f, int32_t track);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif