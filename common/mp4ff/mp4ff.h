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
** $Id: mp4ff.h,v 1.7 2003/11/22 15:30:19 menno Exp $
**/

#ifndef MP4FF_H
#define MP4FF_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* file callback structure */
typedef struct
{
    int (*read)(void *user_data, void *buffer, int length);
    int (*seek)(void *user_data, int position);
    void *user_data;
} mp4ff_callback_t;

/* mp4 main file structure */
typedef void *mp4ff_t;


/* API */

mp4ff_t *mp4ff_open_read(mp4ff_callback_t *f);
void mp4ff_close(mp4ff_t *ff);
static void mp4ff_track_add(mp4ff_t *f);
static int parse_sub_atoms(mp4ff_t *f, const int total_size);
static int parse_atoms(mp4ff_t *f);
int mp4ff_get_sample_duration(const mp4ff_t *f, const int track, const int sample);
int mp4ff_read_sample(mp4ff_t *f, const int track, const int sample,
                          unsigned char **audio_buffer,  unsigned int *bytes);
int mp4ff_get_decoder_config(const mp4ff_t *f, const int track,
                             unsigned char** ppBuf, unsigned int* pBufSize);
int mp4ff_total_tracks(const mp4ff_t *f);
int mp4ff_time_scale(const mp4ff_t *f, const int track);
int mp4ff_num_samples(const mp4ff_t *f, const int track);

/* metadata */
int mp4ff_meta_get_num_items(const mp4ff_t *f);
int mp4ff_meta_get_by_index(const mp4ff_t *f, unsigned int index,
                            char **item, char **value);
int mp4ff_meta_get_title(const mp4ff_t *f, char **value);
int mp4ff_meta_get_artist(const mp4ff_t *f, char **value);
int mp4ff_meta_get_writer(const mp4ff_t *f, char **value);
int mp4ff_meta_get_album(const mp4ff_t *f, char **value);
int mp4ff_meta_get_date(const mp4ff_t *f, char **value);
int mp4ff_meta_get_tool(const mp4ff_t *f, char **value);
int mp4ff_meta_get_comment(const mp4ff_t *f, char **value);
int mp4ff_meta_get_genre(const mp4ff_t *f, char **value);
int mp4ff_meta_get_track(const mp4ff_t *f, char **value);
int mp4ff_meta_get_disc(const mp4ff_t *f, char **value);
int mp4ff_meta_get_compilation(const mp4ff_t *f, char **value);
int mp4ff_meta_get_tempo(const mp4ff_t *f, char **value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif