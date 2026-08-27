/****************************************************************************
    MP4 input module

    Copyright (C) 2017 Krzysztof Nikiel

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#include <stdint.h>

typedef struct
{
    uint32_t len;
    uint32_t offset;
} frame_info_t;

typedef struct
{
    uint32_t firstchunk;
    uint32_t samplesperchunk;
} slice_info_t;

typedef struct
{
    uint32_t ctime, mtime;
    uint32_t samplerate;
    // total sound samples
    uint32_t samples;
    uint32_t mvhd_timescale;
    uint32_t channels;

    // gapless container trimming information (in audio sample frames per channel)
    uint32_t gapless_delay;
    uint64_t gapless_valid_samples;
    uint32_t gapless_padding;
    int has_gapless_info;

    // raw elst values preserved until samplerate is available
    int64_t elst_media_time;
    uint64_t elst_segment_duration;
    int has_elst;
    // sample depth
    uint32_t bits;
    // buffer config
    uint32_t buffersize;
    uint32_t bitratemax;
    uint32_t bitrateavg;
    // frame size / offsets
    struct
    {
        frame_info_t *info;
        slice_info_t *map;
        uint32_t nsamples;
        uint32_t nsclices;
        uint32_t current;
        uint32_t maxsize;
    } frame;
    // AudioSpecificConfig data:
    struct
    {
        // Some muxers (e.g. FFmpeg/libavformat) zero-pad the DecSpecificInfo
        // descriptor, so the declared ASC length can exceed the ~2 bytes of
        // meaningful data. A 10-byte buffer rejected such files in esdsin()
        // with "parse:-1"; 64 bytes safely holds any real AudioSpecificConfig.
        uint8_t buf[64];
        uint32_t size;
    } asc;
    struct {
        uint32_t size;
        uint8_t *data;
    } bitbuf;
    struct {
        int header;
        int tags;
    } verbose;
} mp4config_t;

extern mp4config_t mp4config;

int mp4read_open(char *name);
int mp4read_seek(uint32_t framenum);
int mp4read_frame(void);
int mp4read_close(void);
