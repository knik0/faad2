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

static int mdhdin(int size)
{
    // version/flags
    u32in();
    // Creation time
    mp4config.ctime = u32in();
    // Modification time
    mp4config.mtime = u32in();
    // Time scale
    mp4config.samplerate = u32in();
    // Duration
    mp4config.samples = u32in();
    // Language
    u16in();
    // pre_defined
    u16in();

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

    tmp = sizeof(slice_info_t) * mp4config.frame.nsclices;
    if (tmp < mp4config.frame.nsclices)
        return ERR_FAIL;
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

    tmp = sizeof(frame_info_t) * mp4config.frame.nsamples;
    if (tmp < mp4config.frame.nsamples)
        return ERR_FAIL;
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

    // version/flags
    u32in();
    // Predefined
    u32in();
    // Handler type
    datain(buf, 4);
    if (memcmp(buf, "mdir", 4))
        return ERR_FAIL;
    datain(buf, 4);
    if (memcmp(buf, "appl", 4))
        return ERR_FAIL;
    // Reserved
    u32in();
    u32in();
    // null terminator
    u8in();

    return size;
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
        {"Album       ", "\xa9" "alb"},
        {"Album Artist", "aART"},
        {"Artist      ", "\xa9" "ART"},
        {"Comment     ", "\xa9" "cmt"},
        {"Cover image ", "covr"},
        {"Compilation ", "cpil"},
        {"Copyright   ", "cprt"},
        {"Date        ", "\xa9" "day"},
        {"Disc#       ", "disk", NUMSET},
        {"Genre       ", "gnre", GENRE},
        {"Grouping    ", "\xa9" "grp"},
        {"Lyrics      ", "\xa9" "lyr"},
        {"Title       ", "\xa9" "nam"},
        {"Rating      ", "rtng"},
        {"BPM         ", "tmpo"},
        {"Encoder     ", "\xa9" "too"},
        {"Track       ", "trkn", NUMSET},
        {"Composer    ", "\xa9" "wrt"},
        {0, "----", EXTAG},
        {0},
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

    fprintf(stderr, "----------tag list-------------\n");
    while(read < size)
    {
        int asize, dsize;
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
            fprintf(stderr, "%s :   ", tags[cnt].name);
        else
        {
            if (tags[cnt].flag != EXTAG)
                fprintf(stderr, "'%s'       :   ", id);
        }

        dsize = u32in();
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
            int spc;

            if (memcmp(id, "mean", 4))
                goto skip;
            dsize -= 8;
            while (dsize > 0)
            {
                u8in();
                asize--;
                dsize--;
            }
            if (asize >= 8)
            {
                dsize = u32in() - 8;
                asize -= 4;
                if (datain(id, 4) < 4)
                    return ERR_FAIL;
                asize -= 4;
                if (memcmp(id, "name", 4))
                    goto skip;
                u32in();
                asize -= 4;
                dsize -= 4;
            }
            spc = 13 - dsize;
            if (spc < 0) spc = 0;
            while (dsize > 0)
            {
                fprintf(stderr, "%c",u8in());
                asize--;
                dsize--;
            }
            while (spc--)
                fprintf(stderr, " ");
            fprintf(stderr, ":   ");
            if (asize >= 8)
            {
                dsize = u32in() - 8;
                asize -= 4;
                if (datain(id, 4) < 4)
                    return ERR_FAIL;
                asize -= 4;
                if (memcmp(id, "data", 4))
                    goto skip;
                u32in();
                asize -= 4;
                dsize -= 4;
            }
            while (dsize > 0)
            {
                fprintf(stderr, "%c",u8in());
                asize--;
                dsize--;
            }
            fprintf(stderr, "\n");

            goto skip;
        }
        type = u32in();
        asize -= 4;
        u32in();
        asize -= 4;

        switch(type)
        {
        case 1:
            while (asize > 0)
            {
                fprintf(stderr, "%c",u8in());
                asize--;
            }
            break;
        case 0:
            switch(tags[cnt].flag)
            {
            case NUMSET:
                u16in();
                asize -= 2;

                fprintf(stderr, "%d", u16in());
                asize -= 2;
                fprintf(stderr, "/%d", u16in());
                asize -= 2;
                break;
            case GENRE:
                {
                    uint16_t gnum = u16in();
                    asize -= 2;
                    if (!gnum)
                       goto skip;
                    gnum--;
                    if (gnum >= 147)
                        gnum = 147;
                    fprintf(stderr, "%s", genres[gnum]);
                }
                break;
            default:
                while(asize > 0)
                {
                    fprintf(stderr, "%d/", u16in());
                    asize-=2;
                }
            }
            break;
        case 0x15:
            //fprintf(stderr, "(8bit data)");
            while(asize > 0)
            {
                fprintf(stderr, "%d", u8in());
                asize--;
                if (asize)
                    fprintf(stderr, "/");
            }
            break;
        case 0xd:
            fprintf(stderr, "(image data)");
            break;
        default:
            fprintf(stderr, "(unknown data type)");
            break;
        }
        fprintf(stderr, "\n");

    skip:
        // skip to the end of atom
        while (asize > 0)
        {
            u8in();
            asize--;
        }
    }
    fprintf(stderr, "-------------------------------\n");

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
            fprintf(stderr, "parse error: atom '%s' not found\n", g_atom->name);
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
        NAME("mvhd"),
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
    if (framenum > mp4config.frame.nsamples)
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

    mp4read_close();

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

    // alloc frame buffer
    mp4config.bitbuf.data = malloc(mp4config.frame.maxsize);

    if (!mp4config.bitbuf.data)
        goto err;

    if (mp4config.verbose.header)
    {
        mp4info();
        fprintf(stderr, "********************\n");
    }

    if (mp4config.verbose.tags)
    {
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
    }

    return ERR_OK;
err:
    mp4read_close();
    return ERR_FAIL;
}
