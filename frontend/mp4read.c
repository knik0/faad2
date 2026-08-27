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

#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include "unicode_support.h"
#include "mp4read.h"

enum ATOM_TYPE
{
    ATOM_STOP = 0 /* end of atoms */ ,
    ATOM_NAME /* plain atom */ ,
    ATOM_DESCENT,               /* starts group of children */
    ATOM_ASCENT,                /* ends group */
    ATOM_DATA,
};

typedef int (*parse_t)(int);

typedef struct
{
    uint16_t opcode;
    const char *name;
    parse_t parse;
} creator_t;

#define STOP() {ATOM_STOP, NULL, NULL}
#define NAME(N) {ATOM_NAME, N, NULL}
#define DESCENT() {ATOM_DESCENT, NULL, NULL}
#define ASCENT() {ATOM_ASCENT, NULL, NULL}
#define DATA(N, F) {ATOM_NAME, N, NULL}, {ATOM_DATA, NULL, F}

mp4config_t mp4config = { 0 };

static FILE *g_fin = NULL;

enum {ERR_OK = 0, ERR_FAIL = -1, ERR_UNSUPPORTED = -2};

#define freeMem(A) if (*(A)) {free(*(A)); *(A) = NULL;}
#define tag_fprintf(...) if (mp4config.verbose.tags) fprintf(__VA_ARGS__)

static size_t datain(void *data, size_t size)
{
    return fread(data, 1, size, g_fin);
}

static int stringin(char *txt, int sizemax)
{
    int size;
    for (size = 0; size < sizemax; size++)
    {
        if (fread(txt + size, 1, 1, g_fin) != 1)
            return ERR_FAIL;
        if (!txt[size])
            break;
    }
    txt[sizemax-1] = '\0';

    return size;
}

static uint32_t u32in(void)
{
    uint8_t u8[4];
    datain(&u8, 4);
    return (uint32_t)u8[3] | ((uint32_t)u8[2] << 8) | ((uint32_t)u8[1] << 16) | ((uint32_t)u8[0] << 24);
}

static uint16_t u16in(void)
{
    uint8_t u8[2];
    datain(&u8, 2);
    return (uint16_t)u8[1] | ((uint16_t)u8[0] << 8);
}

static int u8in(void)
{
    uint8_t u8;
    datain(&u8, 1);
    return u8;
}

static int ftypin(int size)
{
    enum {BUFSIZE = 40};
    char buf[BUFSIZE];
    uint32_t u32;

    buf[4] = 0;
    datain(buf, 4);
    u32 = u32in();

    if (mp4config.verbose.header)
        fprintf(stderr, "Brand:\t\t\t%s(version %d)\n", buf, u32);

    stringin(buf, BUFSIZE);

    if (mp4config.verbose.header)
        fprintf(stderr, "Compatible brands:\t%s\n", buf);

    return size;
}

enum
{ SECSINDAY = 24 * 60 * 60 };
static char *mp4time(time_t t)
{
    int y;

    // subtract some seconds from the start of 1904 to the start of 1970
    for (y = 1904; y < 1970; y++)
    {
        t -= 365 * SECSINDAY;
        if (!(y & 3))
            t -= SECSINDAY;
    }
    return ctime(&t);
}

static int mvhdin(int size)
{
    uint8_t version = u8in();
    // flags (3 bytes)
    u8in(); u8in(); u8in();

    if (version == 1)
    {
        // 64-bit creation and modification times
        u32in(); u32in();
        u32in(); u32in();
        mp4config.mvhd_timescale = u32in();
        u32in(); u32in(); // 64-bit duration
    }
    else
    {
        // 32-bit creation and modification times
        u32in();
        u32in();
        mp4config.mvhd_timescale = u32in();
        u32in(); // 32-bit duration
    }

    return size;
}

static int mdhdin(int size)
{
    uint8_t version = u8in();
    // flags (3 bytes)
    u8in(); u8in(); u8in();

    if (version == 1)
    {
        u32in(); u32in(); // creation time
        u32in(); u32in(); // modification time
        mp4config.samplerate = u32in();
        uint64_t dur_hi = u32in();
        uint64_t dur_lo = u32in();
        uint64_t dur64 = (dur_hi << 32) | dur_lo;
        mp4config.samples = (dur64 > UINT32_MAX) ? UINT32_MAX : (uint32_t)dur64;
    }
    else
    {
        mp4config.ctime = u32in();
        mp4config.mtime = u32in();
        mp4config.samplerate = u32in();
        mp4config.samples = u32in();
    }
    // Language & pre_defined
    u16in();
    u16in();

    return size;
}

static int elstin(int size)
{
    uint8_t version = u8in();
    // flags (3 bytes)
    u8in(); u8in(); u8in();

    uint32_t entry_count = u32in();
    int entry_size = (version == 1) ? 20 : 12;
    int available = size - 8;

    /* Some tracks start with an "empty edit" (media_time == -1) used only to
       offset the track's start for A/V sync; the real gapless priming delay
       is carried by the first non-empty entry that follows it. */
    while (entry_count > 0 && available >= entry_size)
    {
        uint64_t segment_duration;
        int64_t media_time;

        if (version == 1)
        {
            uint64_t dur_hi = u32in();
            uint64_t dur_lo = u32in();
            segment_duration = (dur_hi << 32) | dur_lo;

            uint64_t time_hi = u32in();
            uint64_t time_lo = u32in();
            media_time = (int64_t)((time_hi << 32) | time_lo);
        }
        else
        {
            segment_duration = u32in();
            media_time = (int32_t)u32in();
        }
        // media_rate_integer, media_rate_fraction
        u16in();
        u16in();

        available -= entry_size;
        entry_count--;

        if (media_time < 0)
            continue;

        if (!mp4config.has_elst)
        {
            /* elst_media_time is in media (sample rate) timescale = priming delay in samples */
            mp4config.elst_media_time = media_time;
            mp4config.elst_segment_duration = segment_duration;
            mp4config.has_elst = 1;
        }
        break;
    }

    return size;
}

static int hdlr1in(int size)
{
    uint8_t buf[5];

    buf[4] = 0;
    // version/flags
    u32in();
    // pre_defined
    u32in();
    // Component subtype
    datain(buf, 4);
    if (mp4config.verbose.header)
        fprintf(stderr, "*track media type: '%s': ", buf);
    if (memcmp("soun", buf, 4))
    {
        if (mp4config.verbose.header)
            fprintf(stderr, "unsupported, skipping\n");
        return ERR_UNSUPPORTED;
    }
    else
    {
        if (mp4config.verbose.header)
            fprintf(stderr, "OK\n");
    }
    // reserved
    u32in();
    u32in();
    u32in();
    // name
    // null terminate
    u8in();

    return size;
}

static int stsdin(int size)
{
    // version/flags
    u32in();
    // Number of entries(one 'mp4a')
    if (u32in() != 1) //fixme: error handling
        return ERR_FAIL;

    return size;
}

static int mp4ain(int size)
{
    // Reserved (6 bytes)
    u32in();
    u16in();
    // Data reference index
    u16in();
    // Version
    u16in();
    // Revision level
    u16in();
    // Vendor
    u32in();
    // Number of channels
    mp4config.channels = u16in();
    // Sample size (bits)
    mp4config.bits = u16in();
    // Compression ID
    u16in();
    // Packet size
    u16in();
    // Sample rate (16.16)
    // fractional framerate, probably not for audio
    // rate integer part
    u16in();
    // rate reminder part
    u16in();

    return size;
}


static uint32_t getsize(void)
{
    int cnt;
    uint32_t size = 0;
    for (cnt = 0; cnt < 4; cnt++)
    {
        int tmp = u8in();

        size <<= 7;
        size |= (tmp & 0x7f);
        if (!(tmp & 0x80))
            break;
    }
    return size;
}

static int esdsin(int size)
{
    // descriptor tree:
    // MP4ES_Descriptor
    //   MP4DecoderConfigDescriptor
    //      MP4DecSpecificInfoDescriptor
    //   MP4SLConfigDescriptor
    enum
    { TAG_ES = 3, TAG_DC = 4, TAG_DSI = 5, TAG_SLC = 6 };

    // version/flags
    u32in();
    if (u8in() != TAG_ES)
        return ERR_FAIL;
    getsize();
    // ESID
    u16in();
    // flags(url(bit 6); ocr(5); streamPriority (0-4)):
    u8in();

    if (u8in() != TAG_DC)
        return ERR_FAIL;
    getsize();
    if (u8in() != 0x40) /* not MPEG-4 audio */
        return ERR_FAIL;
    // flags
    u8in();
    // buffer size (24 bits)
    mp4config.buffersize = u16in() << 8;
    mp4config.buffersize |= u8in();
    // bitrate
    mp4config.bitratemax = u32in();
    mp4config.bitrateavg = u32in();

    if (u8in() != TAG_DSI)
        return ERR_FAIL;
    mp4config.asc.size = getsize();
    if (mp4config.asc.size > sizeof(mp4config.asc.buf))
        return ERR_FAIL;
    // get AudioSpecificConfig
    datain(mp4config.asc.buf, mp4config.asc.size);

    if (u8in() != TAG_SLC)
        return ERR_FAIL;
    getsize();
    // "predefined" (no idea)
    u8in();

    return size;
}

/* stbl "Sample Table" layout: 
 *  - stts "Time-to-Sample" - useless
 *  - stsc "Sample-to-Chunk" - condensed table chunk-to-num-samples
 *  - stsz "Sample Size" - size table
 *  - stco "Chunk Offset" - chunk starts
 *
 * When receiving stco we can combine stsc and stsz tables to produce final
 * sample offsets.
 */

static int sttsin(int size)
{
    uint32_t ntts;

    if (size < 8)
        return ERR_FAIL;

    // version/flags
    u32in();
    ntts = u32in();

    if (ntts < 1)
        return ERR_FAIL;

    /* 2 x uint32_t per entry */
    if (((size - 8u) / 8u) < ntts)
        return ERR_FAIL;

    return size;
}

static int stscin(int size)
{
    uint32_t i, tmp, firstchunk, prevfirstchunk, samplesperchunk;

    if (size < 8)
        return ERR_FAIL;

    // version/flags
    u32in();

    mp4config.frame.nsclices = u32in();

    if (!mp4config.frame.nsclices)
        return ERR_FAIL;

    if (mp4config.frame.nsclices > UINT32_MAX / sizeof(slice_info_t))
        return ERR_FAIL;
    tmp = sizeof(slice_info_t) * mp4config.frame.nsclices;
    mp4config.frame.map = malloc(tmp);
    if (!mp4config.frame.map)
        return ERR_FAIL;

    /* 3 x uint32_t per entry */
    if (((size - 8u) / 12u) < mp4config.frame.nsclices)
        return ERR_FAIL;

    prevfirstchunk = 0;
    for (i = 0; i < mp4config.frame.nsclices; ++i) {
      firstchunk = u32in();
      samplesperchunk = u32in();
      // id - unused
      u32in();
      if (firstchunk <= prevfirstchunk)
        return ERR_FAIL;
      if (samplesperchunk < 1)
        return ERR_FAIL;
      mp4config.frame.map[i].firstchunk = firstchunk;
      mp4config.frame.map[i].samplesperchunk = samplesperchunk;
      prevfirstchunk = firstchunk;
    }

    return size;
}

static int stszin(int size)
{
    uint32_t i, tmp;

    if (size < 12)
        return ERR_FAIL;

    // version/flags
    u32in();
    // (uniform) Sample size
    // TODO(eustas): add uniform sample size support?
    u32in();
    mp4config.frame.nsamples = u32in();

    if (!mp4config.frame.nsamples)
        return ERR_FAIL;

    if (mp4config.frame.nsamples > UINT32_MAX / sizeof(frame_info_t))
        return ERR_FAIL;
    tmp = sizeof(frame_info_t) * mp4config.frame.nsamples;
    mp4config.frame.info = malloc(tmp);
    if (!mp4config.frame.info)
        return ERR_FAIL;

    if ((size - 12u) / 4u < mp4config.frame.nsamples)
        return ERR_FAIL;

    for (i = 0; i < mp4config.frame.nsamples; i++)
    {
        mp4config.frame.info[i].len = u32in();
        mp4config.frame.info[i].offset = 0;
        if (mp4config.frame.maxsize < mp4config.frame.info[i].len)
            mp4config.frame.maxsize = mp4config.frame.info[i].len;
    }

    return size;
}

static int stcoin(int size)
{
    uint32_t numchunks, chunkn, slicen, samplesleft, i, offset;
    uint32_t nextoffset;

    if (size < 8)
        return ERR_FAIL;

    // version/flags
    u32in();

    // Number of entries
    numchunks = u32in();
    if ((numchunks < 1) || ((numchunks + 1) == 0))
        return ERR_FAIL;

    if ((size - 8u) / 4u < numchunks)
        return ERR_FAIL;

    chunkn = 0;
    samplesleft = 0;
    slicen = 0;
    offset = 0;

    for (i = 0; i < mp4config.frame.nsamples; ++i) {
        if (samplesleft == 0)
        {
            chunkn++;
            if (chunkn > numchunks)
                return ERR_FAIL;
            if (slicen < mp4config.frame.nsclices &&
                (slicen + 1) < mp4config.frame.nsclices) {
                if (chunkn == mp4config.frame.map[slicen + 1].firstchunk)
                    slicen++;
            }
            samplesleft = mp4config.frame.map[slicen].samplesperchunk;
            offset = u32in();
        }
        mp4config.frame.info[i].offset = offset;
        nextoffset = offset + mp4config.frame.info[i].len;
        if (nextoffset < offset)
            return ERR_FAIL;
        offset = nextoffset;
        samplesleft--;
    }

    freeMem(&mp4config.frame.map);

    return size;
}

#if 0
static int tagtxt(char *tagname, const char *tagtxt)
{
    //int txtsize = strlen(tagtxt);
    int size = 0;
    //int datasize = txtsize + 16;

#if 0
    size += u32out(datasize + 8);
    size += dataout(tagname, 4);
    size += u32out(datasize);
    size += dataout("data", 4);
    size += u32out(1);
    size += u32out(0);
    size += dataout(tagtxt, txtsize);
#endif

    return size;
}

static int tagu32(char *tagname, int n /*number of stored fields*/)
{
    //int numsize = n * 4;
    int size = 0;
    //int datasize = numsize + 16;

#if 0
    size += u32out(datasize + 8);
    size += dataout(tagname, 4);
    size += u32out(datasize);
    size += dataout("data", 4);
    size += u32out(0);
    size += u32out(0);
#endif

    return size;
}
#endif

static int metain(int size)
{
    (void)size;  /* why not used? */
    // version/flags
    u32in();

    return ERR_OK;
}

static int hdlr2in(int size)
{
    uint8_t buf[4];
    int original_size = size;

    /* 12 bytes = version/flags (4) + pre_defined (4) + handler type (4) */
    if (size < 12)
        return ERR_FAIL;

    // version/flags
    u32in();
    // Predefined
    u32in();
    // Handler type: ISOBMFF metadata containers use 'mdir' (iTunes metadata) or 'mdta' (Apple keys)
    datain(buf, 4);
    if (memcmp(buf, "mdir", 4) && memcmp(buf, "mdta", 4))
        return ERR_FAIL;

    size -= 12;
    while (size > 0)
    {
        u8in();
        size--;
    }

    /* Return original consumed payload size so atom parser maintains correct offsets */
    return original_size;
}

static int ilstin(int size)
{
    enum {NUMSET = 1, GENRE, EXTAG};
    int read = 0;

    static struct {
        char *name;
        char *id;
        int flag;
    } tags[] = {
        {"Album       ", "\xa9" "alb", 0},
        {"Album Artist", "aART", 0},
        {"Artist      ", "\xa9" "ART", 0},
        {"Comment     ", "\xa9" "cmt", 0},
        {"Cover image ", "covr", 0},
        {"Compilation ", "cpil", 0},
        {"Copyright   ", "cprt", 0},
        {"Date        ", "\xa9" "day", 0},
        {"Disc#       ", "disk", NUMSET},
        {"Genre       ", "gnre", GENRE},
        {"Genre       ", "\xa9" "gen", 0},
        {"Grouping    ", "\xa9" "grp", 0},
        {"Lyrics      ", "\xa9" "lyr", 0},
        {"Title       ", "\xa9" "nam", 0},
        {"Rating      ", "rtng", 0},
        {"BPM         ", "tmpo", 0},
        {"Encoder     ", "\xa9" "too", 0},
        {"Track       ", "trkn", NUMSET},
        {"Composer    ", "\xa9" "wrt", 0},
        {0, "----", EXTAG},
        {0, 0, 0},
    };

    static const char *genres[] = {
        "Blues", "Classic Rock", "Country", "Dance",
        "Disco", "Funk", "Grunge", "Hip-Hop",
        "Jazz", "Metal", "New Age", "Oldies",
        "Other", "Pop", "R&B", "Rap",
        "Reggae", "Rock", "Techno", "Industrial",
        "Alternative", "Ska", "Death Metal", "Pranks",
        "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop",
        "Vocal", "Jazz+Funk", "Fusion", "Trance",
        "Classical", "Instrumental", "Acid", "House",
        "Game", "Sound Clip", "Gospel", "Noise",
        "Alternative Rock", "Bass", "Soul", "Punk",
        "Space", "Meditative", "Instrumental Pop", "Instrumental Rock",
        "Ethnic", "Gothic", "Darkwave", "Techno-Industrial",
        "Electronic", "Pop-Folk", "Eurodance", "Dream",
        "Southern Rock", "Comedy", "Cult", "Gangsta",
        "Top 40", "Christian Rap", "Pop/Funk", "Jungle",
        "Native US", "Cabaret", "New Wave", "Psychadelic",
        "Rave", "Showtunes", "Trailer", "Lo-Fi",
        "Tribal", "Acid Punk", "Acid Jazz", "Polka",
        "Retro", "Musical", "Rock & Roll", "Hard Rock",
        "Folk", "Folk-Rock", "National Folk", "Swing",
        "Fast Fusion", "Bebob", "Latin", "Revival",
        "Celtic", "Bluegrass", "Avantgarde", "Gothic Rock",
        "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock",
        "Big Band", "Chorus", "Easy Listening", "Acoustic",
        "Humour", "Speech", "Chanson", "Opera",
        "Chamber Music", "Sonata", "Symphony", "Booty Bass",
        "Primus", "Porn Groove", "Satire", "Slow Jam",
        "Club", "Tango", "Samba", "Folklore",
        "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle",
        "Duet", "Punk Rock", "Drum Solo", "Acapella",
        "Euro-House", "Dance Hall", "Goa", "Drum & Bass",
        "Club - House", "Hardcore", "Terror", "Indie",
        "BritPop", "Negerpunk", "Polsk Punk", "Beat",
        "Christian Gangsta Rap", "Heavy Metal", "Black Metal", "Crossover",
        "Contemporary Christian", "Christian Rock", "Merengue", "Salsa",
        "Thrash Metal", "Anime", "JPop", "Synthpop",
        "Unknown",
    };

    tag_fprintf(stderr, "----------tag list-------------\n");
    while(read < size)
    {
        int asize;
        uint8_t id[5];
        int cnt;
        uint32_t type;

        id[4] = 0;

        asize = u32in();
        read += asize;
        asize -= 4;
        if (datain(id, 4) < 4)
            return ERR_FAIL;
        asize -= 4;

        for (cnt = 0; tags[cnt].id; cnt++)
        {
            if (!memcmp(id, tags[cnt].id, 4))
                break;
        }

        if (tags[cnt].name)
        {
            tag_fprintf(stderr, "%s :   ", tags[cnt].name);
        }
        else
        {
            if (tags[cnt].flag != EXTAG)
                tag_fprintf(stderr, "'%s'       :   ", id);
        }

        uint32_t sub_size = u32in();
        asize -= 4;
        if (datain(id, 4) < 4)
            return ERR_FAIL;
        asize -= 4;

        if (tags[cnt].flag != EXTAG)
        {
            if (memcmp(id, "data", 4))
                return ERR_FAIL;
        }
        else
        {
            char ext_name[256] = {0};
            char ext_data[512] = {0};
            uint8_t sub_id[5];
            memcpy(sub_id, id, 4);
            sub_id[4] = 0;

            /* Parse 'mean', 'name', and 'data' sub-atoms inside '----' freeform tag */
            while (1)
            {
                if (sub_size < 8)
                    break;
                uint32_t sub_payload_len = sub_size - 8;

                if (memcmp(sub_id, "mean", 4) == 0)
                {
                    while (sub_payload_len > 0 && asize > 0)
                    {
                        u8in();
                        asize--;
                        sub_payload_len--;
                    }
                }
                else if (memcmp(sub_id, "name", 4) == 0)
                {
                    /* Skip 4-byte FullBox version/flags header */
                    if (sub_payload_len >= 4 && asize >= 4)
                    {
                        u32in();
                        asize -= 4;
                        sub_payload_len -= 4;
                    }
                    int ext_name_len = 0;
                    while (sub_payload_len > 0 && asize > 0)
                    {
                        char ch = u8in();
                        if (ext_name_len < (int)sizeof(ext_name) - 1)
                            ext_name[ext_name_len++] = ch;
                        asize--;
                        sub_payload_len--;
                    }
                    ext_name[ext_name_len] = '\0';
                }
                else if (memcmp(sub_id, "data", 4) == 0)
                {
                    /* Skip 8-byte data header: 4-byte type/flags + 4-byte locale/reserved */
                    if (sub_payload_len >= 8 && asize >= 8)
                    {
                        u32in();
                        u32in();
                        asize -= 8;
                        sub_payload_len -= 8;
                    }
                    int ext_data_len = 0;
                    while (sub_payload_len > 0 && asize > 0)
                    {
                        char ch = u8in();
                        if (ext_data_len < (int)sizeof(ext_data) - 1)
                            ext_data[ext_data_len++] = ch;
                        asize--;
                        sub_payload_len--;
                    }
                    ext_data[ext_data_len] = '\0';
                }
                else
                {
                    while (sub_payload_len > 0 && asize > 0)
                    {
                        u8in();
                        asize--;
                        sub_payload_len--;
                    }
                }

                if (asize < 8)
                    break;
                sub_size = u32in();
                asize -= 4;
                if (datain(sub_id, 4) < 4)
                    return ERR_FAIL;
                asize -= 4;
            }

            tag_fprintf(stderr, "%-13s:   %s\n", ext_name[0] ? ext_name : "----", ext_data);

            if (strcmp(ext_name, "iTunSMPB") == 0 || strstr(ext_data, "iTunSMPB") != NULL)
            {
                char *smpb = strstr(ext_data, "iTunSMPB");
                if (!smpb) smpb = ext_data;
                uint32_t dummy = 0, delay = 0, padding = 0;
                uint64_t valid_samples = 0;
                if (sscanf(smpb, "iTunSMPB %x %x %x %llx", &dummy, &delay, &padding, (unsigned long long*)&valid_samples) >= 4 ||
                    sscanf(smpb, "%x %x %x %llx", &dummy, &delay, &padding, (unsigned long long*)&valid_samples) >= 4)
                {
                    if (!mp4config.has_gapless_info && !mp4config.has_elst)
                    {
                        mp4config.gapless_delay = delay;
                        mp4config.gapless_padding = padding;
                        mp4config.gapless_valid_samples = valid_samples;
                        mp4config.has_gapless_info = 1;
                    }
                }
            }

            goto skip;
        }
        type = u32in();
        asize -= 4;
        u32in();
        asize -= 4;

        switch(type)
        {
        case 1:
            {
                char val_buf[512];
                int val_len = 0;
                while (asize > 0)
                {
                    char ch = u8in();
                    if (val_len < (int)sizeof(val_buf) - 1)
                        val_buf[val_len++] = ch;
                    asize--;
                }
                val_buf[val_len] = '\0';
                tag_fprintf(stderr, "%s", val_buf);
            }
            break;
        case 0:
            switch(tags[cnt].flag)
            {
            case NUMSET:
                u16in();
                asize -= 2;

                tag_fprintf(stderr, "%d", u16in());
                asize -= 2;
                tag_fprintf(stderr, "/%d", u16in());
                asize -= 2;
                break;
            case GENRE:
                {
                    uint16_t gnum = u16in();
                    asize -= 2;
                    if (!gnum)
                       goto skip;
                    gnum--;
                    if (gnum >= sizeof(genres) / sizeof(genres[0]))
                        gnum = sizeof(genres) / sizeof(genres[0]) - 1;
                    tag_fprintf(stderr, "%s", genres[gnum]);
                }
                break;
            default:
                while(asize > 0)
                {
                    tag_fprintf(stderr, "%d/", u16in());
                    asize-=2;
                }
            }
            break;
        case 0x15:
            while(asize > 0)
            {
                tag_fprintf(stderr, "%d", u8in());
                asize--;
                if (asize)
                    tag_fprintf(stderr, "/");
            }
            break;
        case 0xd:
            tag_fprintf(stderr, "(image data)");
            break;
        default:
            tag_fprintf(stderr, "(unknown data type)");
            break;
        }
        tag_fprintf(stderr, "\n");

    skip:
        // skip to the end of atom
        while (asize > 0)
        {
            u8in();
            asize--;
        }
    }
    tag_fprintf(stderr, "-------------------------------\n");

    return size;
}

static creator_t *g_atom = 0;
static int parse(uint32_t *sizemax)
{
    long apos = 0;
    long aposmax = ftell(g_fin) + *sizemax;
    uint32_t size;

    if (g_atom->opcode != ATOM_NAME)
    {
        fprintf(stderr, "parse error: root is not a 'name' opcode\n");
        return ERR_FAIL;
    }
    //fprintf(stderr, "looking for '%s'\n", (char *)g_atom->name);

    // search for atom in the file
    while (1)
    {
        char name[4];
        uint32_t tmp;

        apos = ftell(g_fin);
        if (apos >= (aposmax - 8))
        {
            return ERR_FAIL;
        }
        if ((tmp = u32in()) < 8)
        {
            fprintf(stderr, "invalid atom size %x @%lx\n", tmp, ftell(g_fin));
            return ERR_FAIL;
        }

        size = tmp;
        if (datain(name, 4) != 4)
        {
            // EOF
            fprintf(stderr, "can't read atom name @%lx\n", ftell(g_fin));
            return ERR_FAIL;
        }

        //fprintf(stderr, "atom: '%c%c%c%c'(%x)", name[0],name[1],name[2],name[3], size);

        if (!memcmp(name, g_atom->name, 4))
        {
            //fprintf(stderr, "OK\n");
            break;
        }
        //fprintf(stderr, "\n");

        fseek(g_fin, apos + size, SEEK_SET);
    }
    *sizemax = size;
    g_atom++;
    if (g_atom->opcode == ATOM_DATA)
    {
        int err = g_atom->parse(size - 8);
        if (err < ERR_OK)
        {
            fseek(g_fin, apos + size, SEEK_SET);
            return err;
        }
        g_atom++;
    }
    if (g_atom->opcode == ATOM_DESCENT)
    {
        long apos2 = ftell(g_fin);

        //fprintf(stderr, "descent\n");
        g_atom++;
        while (g_atom->opcode != ATOM_STOP)
        {
            uint32_t subsize = size - 8;
            int ret;
            if (g_atom->opcode == ATOM_ASCENT)
            {
                g_atom++;
                break;
            }
            // TODO: does not feel well - we always return to the same point!
            fseek(g_fin, apos2, SEEK_SET);
            if ((ret = parse(&subsize)) < 0)
                return ret;
        }
        //fprintf(stderr, "ascent\n");
    }

    fseek(g_fin, apos + size, SEEK_SET);

    return ERR_OK;
}

static int moovin(int sizemax)
{
    long apos = ftell(g_fin);
    uint32_t atomsize;
    creator_t *old_atom = g_atom;
    int err, ret = sizemax;

    static creator_t mvhd[] = {
        DATA("mvhd", mvhdin),
        STOP()
    };
    static creator_t edts[] = {
        NAME("trak"),
        DESCENT(),
        NAME("edts"),
        DESCENT(),
        DATA("elst", elstin),
        STOP()
    };

    static creator_t trak[] = {
        NAME("trak"),
        DESCENT(),
        NAME("tkhd"),
        NAME("mdia"),
        DESCENT(),
        DATA("mdhd", mdhdin),
        DATA("hdlr", hdlr1in),
        NAME("minf"),
        DESCENT(),
        NAME("smhd"),
        NAME("dinf"),
        NAME("stbl"),
        DESCENT(),
        DATA("stsd", stsdin),
        DESCENT(),
        DATA("mp4a", mp4ain),
        DESCENT(),
        DATA("esds", esdsin),
        ASCENT(),
        ASCENT(),
        DATA("stts", sttsin),
        DATA("stsc", stscin),
        DATA("stsz", stszin),
        DATA("stco", stcoin),
        STOP()
    };

    g_atom = mvhd;
    atomsize = sizemax + apos - ftell(g_fin);
    if (parse(&atomsize) < 0) {
        g_atom = old_atom;
        return ERR_FAIL;
    }

    fseek(g_fin, apos, SEEK_SET);

    g_atom = edts;
    atomsize = sizemax + apos - ftell(g_fin);
    parse(&atomsize); /* optional edts atom */

    fseek(g_fin, apos, SEEK_SET);

    while (1)
    {
        //fprintf(stderr, "TRAK\n");
        g_atom = trak;
        atomsize = sizemax + apos - ftell(g_fin);
        if (atomsize < 8)
            break;
        //fprintf(stderr, "PARSE(%x)\n", atomsize);
        err = parse(&atomsize);
        //fprintf(stderr, "SIZE: %x/%x\n", atomsize, sizemax);
        if (err >= 0)
            break;
        if (err != ERR_UNSUPPORTED) {
            ret = err;
            break;
        }
        //fprintf(stderr, "UNSUPP\n");
    }

    g_atom = old_atom;
    return ret;
}


static creator_t g_head[] = {
    DATA("ftyp", ftypin),
    STOP()
};

static creator_t g_moov[] = {
    DATA("moov", moovin),
    //DESCENT(),
    //NAME("mvhd"),
    STOP()
};

static creator_t g_meta1[] = {
    NAME("moov"),
    DESCENT(),
    NAME("udta"),
    DESCENT(),
    DATA("meta", metain),
    DESCENT(),
    DATA("hdlr", hdlr2in),
    DATA("ilst", ilstin),
    STOP()
};

static creator_t g_meta2[] = {
    DATA("meta", metain),
    DESCENT(),
    DATA("hdlr", hdlr2in),
    DATA("ilst", ilstin),
    STOP()
};


int mp4read_frame(void)
{
    if (mp4config.frame.current >= mp4config.frame.nsamples)
        return ERR_FAIL;

    // TODO(eustas): avoid no-op seeks
    mp4read_seek(mp4config.frame.current);

    mp4config.bitbuf.size = mp4config.frame.info[mp4config.frame.current].len;

    if (fread(mp4config.bitbuf.data, 1, mp4config.bitbuf.size, g_fin)
        != mp4config.bitbuf.size)
    {
        fprintf(stderr, "can't read frame data(frame %d@0x%x)\n",
               mp4config.frame.current,
               mp4config.frame.info[mp4config.frame.current].offset);

        return ERR_FAIL;
    }

    mp4config.frame.current++;

    return ERR_OK;
}

int mp4read_seek(uint32_t framenum)
{
    if (framenum >= mp4config.frame.nsamples)
        return ERR_FAIL;
    if (fseek(g_fin, mp4config.frame.info[framenum].offset, SEEK_SET))
        return ERR_FAIL;

    mp4config.frame.current = framenum;

    return ERR_OK;
}

static void mp4info(void)
{
    fprintf(stderr, "Modification Time:\t\t%s\n", mp4time(mp4config.mtime));
    fprintf(stderr, "Samplerate:\t\t%d\n", mp4config.samplerate);
    fprintf(stderr, "Total samples:\t\t%d\n", mp4config.samples);
    fprintf(stderr, "Total channels:\t\t%d\n", mp4config.channels);
    fprintf(stderr, "Bits per sample:\t%d\n", mp4config.bits);
    fprintf(stderr, "Buffer size:\t\t%d\n", mp4config.buffersize);
    fprintf(stderr, "Max bitrate:\t\t%d\n", mp4config.bitratemax);
    fprintf(stderr, "Average bitrate:\t%d\n", mp4config.bitrateavg);
    fprintf(stderr, "Frames:\t\t\t%d\n", mp4config.frame.nsamples);
    fprintf(stderr, "ASC size:\t\t%d\n", mp4config.asc.size);
    fprintf(stderr, "Duration:\t\t%.1f sec\n", (float)mp4config.samples/mp4config.samplerate);
    if (mp4config.frame.nsamples)
        fprintf(stderr, "Data offset:\t%x\n", mp4config.frame.info[0].offset);
}

int mp4read_close(void)
{
    freeMem(&mp4config.frame.info);
    freeMem(&mp4config.frame.map);
    freeMem(&mp4config.bitbuf.data);

    return ERR_OK;
}

int mp4read_open(char *name)
{
    uint32_t atomsize;
    int ret;
    int v_header = mp4config.verbose.header;
    int v_tags = mp4config.verbose.tags;

    mp4read_close();
    memset(&mp4config, 0, sizeof(mp4config_t));
    mp4config.verbose.header = v_header;
    mp4config.verbose.tags = v_tags;

    g_fin = faad_fopen(name, "rb");
    if (!g_fin)
        return ERR_FAIL;

    if (mp4config.verbose.header)
        fprintf(stderr, "**** MP4 header ****\n");
    g_atom = g_head;
    atomsize = INT_MAX;
    if (parse(&atomsize) < 0)
        goto err;
    g_atom = g_moov;
    atomsize = INT_MAX;
    rewind(g_fin);
    if ((ret = parse(&atomsize)) < 0)
    {
        fprintf(stderr, "parse:%d\n", ret);
        goto err;
    }

    if (mp4config.has_elst)
    {
        if (mp4config.elst_media_time >= 0 && mp4config.samplerate > 0 && mp4config.mvhd_timescale > 0)
        {
            mp4config.gapless_delay = (uint32_t)mp4config.elst_media_time;
            /* Convert segment_duration (movie timescale) to audio sample frames */
            double valid_dur_samples = (double)mp4config.elst_segment_duration * (double)mp4config.samplerate / (double)mp4config.mvhd_timescale;
            mp4config.gapless_valid_samples = (uint64_t)(valid_dur_samples + 0.5);
            mp4config.has_gapless_info = 1;
        }
    }

    // alloc frame buffer
    mp4config.bitbuf.data = malloc(mp4config.frame.maxsize);

    if (!mp4config.bitbuf.data)
        goto err;

    if (mp4config.verbose.header)
    {
        mp4info();
        fprintf(stderr, "********************\n");
    }

    rewind(g_fin);
    g_atom = g_meta1;
    atomsize = INT_MAX;
    ret = parse(&atomsize);
    if (ret < 0)
    {
        rewind(g_fin);
        g_atom = g_meta2;
        atomsize = INT_MAX;
        ret = parse(&atomsize);
    }

    return ERR_OK;
err:
    mp4read_close();
    return ERR_FAIL;
}
