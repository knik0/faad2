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
** $Id: mp4meta.c,v 1.1 2003/11/22 15:38:31 menno Exp $
**/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mp4ffint.h"

static int32_t mp4ff_tag_add_field(mp4ff_metadata_t *tags, const char *item, const char *value)
{
    void *backup = (void *)tags->tags;

    if (!item || (item && !*item) || !value) return 0;

    tags->tags = (mp4ff_tag_t*)realloc(tags->tags, (tags->count+1) * sizeof(mp4ff_tag_t));
    if (!tags->tags)
    {
        if (backup) free(backup);
        return 0;
    } else {
        int i_len = strlen(item);
        int v_len = strlen(value);

        tags->tags[tags->count].item = (char *)malloc(i_len+1);
        tags->tags[tags->count].value = (char *)malloc(v_len+1);

        if (!tags->tags[tags->count].item || !tags->tags[tags->count].value)
        {
            if (!tags->tags[tags->count].item) free (tags->tags[tags->count].item);
            if (!tags->tags[tags->count].value) free (tags->tags[tags->count].value);
            tags->tags[tags->count].item = NULL;
            tags->tags[tags->count].value = NULL;
            return 0;
        }

        memcpy(tags->tags[tags->count].item, item, i_len);
        memcpy(tags->tags[tags->count].value, value, v_len);
        tags->tags[tags->count].item[i_len] = '\0';
        tags->tags[tags->count].value[v_len] = '\0';

        tags->count++;
        return 1;
    }
}

static int32_t mp4ff_tag_set_field(mp4ff_metadata_t *tags, const char *item, const char *value)
{
    unsigned int i;

    if (!item || (item && !*item) || !value) return 0;

    for (i = 0; i < tags->count; i++)
    {
        if (!stricmp(tags->tags[i].item, item))
        {
            void *backup = (void *)tags->tags[i].value;
            int v_len = strlen(value);

            tags->tags[i].value = (char *)realloc(tags->tags[i].value, v_len+1);
            if (!tags->tags[i].value)
            {
                if (backup) free(backup);
                return 0;
            }

            memcpy(tags->tags[i].value, value, v_len);
            tags->tags[i].value[v_len] = '\0';

            return 1;
        }
    }

    return mp4ff_tag_add_field(tags, item, value);
}

int32_t mp4ff_tag_delete(mp4ff_metadata_t *tags)
{
    uint32_t i;

    for (i = 0; i < tags->count; i++)
    {
        if (tags->tags[i].item) free(tags->tags[i].item);
        if (tags->tags[i].value) free(tags->tags[i].value);
    }

    if (tags->tags) free(tags->tags);

    tags->tags = NULL;
    tags->count = 0;

    return 0;
}

static const char* ID3v1GenreList[] = {
    "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk",
    "Grunge", "Hip-Hop", "Jazz", "Metal", "New Age", "Oldies",
    "Other", "Pop", "R&B", "Rap", "Reggae", "Rock",
    "Techno", "Industrial", "Alternative", "Ska", "Death Metal", "Pranks",
    "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk",
    "Fusion", "Trance", "Classical", "Instrumental", "Acid", "House",
    "Game", "Sound Clip", "Gospel", "Noise", "AlternRock", "Bass",
    "Soul", "Punk", "Space", "Meditative", "Instrumental Pop", "Instrumental Rock",
    "Ethnic", "Gothic", "Darkwave", "Techno-Industrial", "Electronic", "Pop-Folk",
    "Eurodance", "Dream", "Southern Rock", "Comedy", "Cult", "Gangsta",
    "Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American", "Cabaret",
    "New Wave", "Psychadelic", "Rave", "Showtunes", "Trailer", "Lo-Fi",
    "Tribal", "Acid Punk", "Acid Jazz", "Polka", "Retro", "Musical",
    "Rock & Roll", "Hard Rock", "Folk", "Folk/Rock", "National Folk", "Swing",
    "Fast-Fusion", "Bebob", "Latin", "Revival", "Celtic", "Bluegrass", "Avantgarde",
    "Gothic Rock", "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock", "Big Band",
    "Chorus", "Easy Listening", "Acoustic", "Humour", "Speech", "Chanson",
    "Opera", "Chamber Music", "Sonata", "Symphony", "Booty Bass", "Primus",
    "Porn Groove", "Satire", "Slow Jam", "Club", "Tango", "Samba",
    "Folklore", "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle", "Duet",
    "Punk Rock", "Drum Solo", "A capella", "Euro-House", "Dance Hall",
    "Goa", "Drum & Bass", "Club House", "Hardcore", "Terror",
    "Indie", "BritPop", "NegerPunk", "Polsk Punk", "Beat",
    "Christian Gangsta", "Heavy Metal", "Black Metal", "Crossover", "Contemporary C",
    "Christian Rock", "Merengue", "Salsa", "Thrash Metal", "Anime", "JPop",
    "SynthPop",
};

static int32_t GenreToString(char** GenreStr, const uint16_t genre)
{
    if (genre > 0 && genre <= sizeof(ID3v1GenreList)/sizeof(*ID3v1GenreList))
    {
        *GenreStr = (char*)malloc((strlen(ID3v1GenreList[genre-1])+1)*sizeof(char));
        memset(*GenreStr, 0, (strlen(ID3v1GenreList[genre-1])+1)*sizeof(char));
        strcpy(*GenreStr, ID3v1GenreList[genre-1]);
        return 0;
    }

    *GenreStr = NULL;
    return 1;
}

static int32_t StringToGenre(const char* GenreStr)
{
    int32_t i;

    for (i = 0; i < sizeof(ID3v1GenreList)/sizeof(*ID3v1GenreList); i++)
    {
        if (stricmp(GenreStr, ID3v1GenreList[i]) == 0)
            return i+1;
    }
    return 0;
}

static int32_t TrackToString(char** str, const uint16_t track, const uint16_t totalTracks)
{
    *str = malloc(12);
    sprintf(*str, "%.5u of %.5u", track, totalTracks);

    return 0;
}

static int32_t StringToTrack(const char *str, uint16_t *track, uint16_t *totalTracks)
{
    sscanf(str, "%.5u of %.5u", &track, &totalTracks);

    return 0;
}

static int32_t TempoToString(char** str, const uint16_t tempo)
{
    *str = malloc(12);
    sprintf(*str, "%.5u BPM", tempo);

    return 0;
}

static int32_t mp4ff_set_metadata_name(mp4ff_t *f, const uint8_t atom_type, char **name)
{
    static char *tag_names[] = {
        "unknown", "title", "artist", "writer", "album",
        "date", "tool", "comment", "genre", "track",
        "disc", "compilation", "genre", "tempo"
    };
    uint8_t tag_idx = 0;
    uint16_t size;

    switch (atom_type)
    {
    case ATOM_TITLE: tag_idx = 1; break;
    case ATOM_ARTIST: tag_idx = 2; break;
    case ATOM_WRITER: tag_idx = 3; break;
    case ATOM_ALBUM: tag_idx = 4; break;
    case ATOM_DATE: tag_idx = 5; break;
    case ATOM_TOOL: tag_idx = 6; break;
    case ATOM_COMMENT: tag_idx = 7; break;
    case ATOM_GENRE1: tag_idx = 8; break;
    case ATOM_TRACK: tag_idx = 9; break;
    case ATOM_DISC: tag_idx = 10; break;
    case ATOM_COMPILATION: tag_idx = 11; break;
    case ATOM_GENRE2: tag_idx = 12; break;
    case ATOM_TEMPO: tag_idx = 13; break;
    default: tag_idx = 0; break;
    }

    size = strlen(tag_names[tag_idx])+1;
    *name = malloc(size+1);
    memset(*name, 0, size+1);
    memcpy(*name, tag_names[tag_idx], size);

    return 0;
}

static int32_t mp4ff_parse_tag(mp4ff_t *f, const uint8_t parent_atom_type, const int32_t size)
{
    uint8_t atom_type;
    int32_t subsize, sumsize = 0;
    char *name = NULL;
    char *data = NULL;

    while (sumsize < (size-8))
    {
        subsize = mp4ff_atom_read_header(f, &atom_type);
        if (atom_type == ATOM_DATA)
        {
            mp4ff_read_char(f); /* version */
            mp4ff_read_int24(f); /* flags */
            mp4ff_read_int32(f); /* reserved */
            data = malloc(subsize-16+1);
            mp4ff_read_data(f, data, subsize-16);
            data[subsize-16] = '\0';

            /* some need special attention */
            if (parent_atom_type == ATOM_GENRE2 || parent_atom_type == ATOM_TEMPO)
            {
                uint16_t val = 0;
                char *tmp;
                tmp = data;

                val = (uint16_t)(tmp[1]);
                val += (uint16_t)(tmp[0]<<8);

                if (data)
                {
                    free(data);
                    data = NULL;
                }
                if (parent_atom_type == ATOM_TEMPO)
                    TempoToString(&data, val);
                else
                    GenreToString(&data, val);
            } else if (parent_atom_type == ATOM_TRACK || parent_atom_type == ATOM_DISC) {
                uint16_t val1 = 0, val2 = 0;
                char *tmp;
                tmp = data;

                val1 = (uint16_t)(tmp[3]);
                val1 += (uint16_t)(tmp[2]<<8);
                val2 = (uint16_t)(tmp[5]);
                val2 += (uint16_t)(tmp[4]<<8);

                if (data)
                {
                    free(data);
                    data = NULL;
                }
                TrackToString(&data, val1, val2);
            }
        } else if (atom_type == ATOM_NAME) {
            mp4ff_read_char(f); /* version */
            mp4ff_read_int24(f); /* flags */
            name = malloc(subsize-12+1);
            mp4ff_read_data(f, name, subsize-12);
            name[subsize-12] = '\0';
        } else {
            mp4ff_set_position(f, mp4ff_position(f)+subsize-8);
        }
        sumsize += subsize;
    }

    if (name == NULL)
    {
        mp4ff_set_metadata_name(f, parent_atom_type, &name);
    }

    mp4ff_tag_set_field(&(f->tags), (const char*)name, (const char*)data);

    return 0;
}

int32_t mp4ff_parse_metadata(mp4ff_t *f, const int32_t size)
{
    int32_t subsize, sumsize = 0;
    uint8_t atom_type;

    while (sumsize < (size-12))
    {
        subsize = mp4ff_atom_read_header(f, &atom_type);
        mp4ff_parse_tag(f, atom_type, subsize);
        sumsize += subsize;
    }

    return 0;
}

/* find a metadata item by name */
/* returns 0 if item found, 1 if no such item */
static int32_t mp4ff_meta_find_by_name(const mp4ff_t *f, const char *item, char **value)
{
    uint32_t i;

    for (i = 0; i < f->tags.count; i++)
    {
        if (!stricmp(f->tags.tags[i].item, item))
        {
            int v_len = strlen(f->tags.tags[i].value);

            *value = (char*)malloc(v_len + 1);
            memcpy(*value, f->tags.tags[i].value, v_len);
            (*value)[v_len] = '\0';

            return 0;
        }
    }

    *value = NULL;

    /* not found */
    return 1;
}

int32_t mp4ff_meta_get_num_items(const mp4ff_t *f)
{
    return f->tags.count;
}

int32_t mp4ff_meta_get_by_index(const mp4ff_t *f, uint32_t index,
                                char **item, char **value)
{
    if (index >= f->tags.count)
    {
        *item = NULL;
        *value = NULL;
        return 1;
    } else {
        int i_len = strlen(f->tags.tags[index].item);
        int v_len = strlen(f->tags.tags[index].value);

        *item = (char*)malloc(i_len + 1);
        memcpy(*item, f->tags.tags[index].item, i_len);
        (*item)[i_len] = '\0';
        *value = (char*)malloc(v_len + 1);
        memcpy(*value, f->tags.tags[index].value, v_len);
        (*value)[v_len] = '\0';
    }

    return 0;
}

int32_t mp4ff_meta_get_title(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "title", value);
}

int32_t mp4ff_meta_get_artist(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "artist", value);
}

int32_t mp4ff_meta_get_writer(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "writer", value);
}

int32_t mp4ff_meta_get_album(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "album", value);
}

int32_t mp4ff_meta_get_date(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "date", value);
}

int32_t mp4ff_meta_get_tool(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "tool", value);
}

int32_t mp4ff_meta_get_comment(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "comment", value);
}

int32_t mp4ff_meta_get_genre(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "genre", value);
}

int32_t mp4ff_meta_get_track(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "track", value);
}

int32_t mp4ff_meta_get_disc(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "disc", value);
}

int32_t mp4ff_meta_get_compilation(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "compilation", value);
}

int32_t mp4ff_meta_get_tempo(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "tempo", value);
}
