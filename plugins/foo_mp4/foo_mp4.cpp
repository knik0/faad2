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
** $Id: foo_mp4.cpp,v 1.40 2003/08/07 17:26:26 menno Exp $
**/

#include <mp4.h>
#include <faad.h>
#include "pfc/pfc.h"
#include "foobar2000/SDK/input.h"
#include "foobar2000/SDK/console.h"
#include "foobar2000/SDK/componentversion.h"
#include "foobar2000/SDK/tagread.h"

//#define DBG_OUT(A) OutputDebugString(A)
#define DBG_OUT(A)

#if 0
char *STRIP_REVISION(const char *str)
{
    char *tmp = strchr(str, ' ');
    int len = lstrlen(tmp)-2;
    if (len > 0 && tmp)
        tmp[len] = '\0';
    else
        tmp = "000";
    return tmp;
}
#endif

DECLARE_COMPONENT_VERSION ("MPEG-4 AAC decoder",
                           "$Revision: 1.40 $",
                           "Based on FAAD2 v" FAAD2_VERSION "\nCopyright (C) 2002-2003 http://www.audiocoding.com" );

class input_mp4 : public input
{
public:

    virtual bool test_filename(const char * fn,const char * ext)
    {
        return (!stricmp(ext,"MP4") || !stricmp(ext,"M4A"));
    }

    virtual bool open(reader *r, file_info *info, unsigned flags)
    {
        unsigned __int8 *buffer;
        unsigned __int32 buffer_size;
        unsigned __int8 channels;
        unsigned __int32 samplerate;

        faacDecConfigurationPtr config;
        mp4AudioSpecificConfig mp4ASC;

        m_reader = r;

        if (!m_reader->can_seek())
        {
            console::error("MP4 file needs seeking.");
            return 0;
        }

        hDecoder = faacDecOpen();
        if (!hDecoder)
        {
            console::error("Failed to open FAAD2 library.");
            return 0;
        }

        config = faacDecGetCurrentConfiguration(hDecoder);
        config->outputFormat = FAAD_FMT_DOUBLE;
        faacDecSetConfiguration(hDecoder, config);

        hFile = MP4ReadCb(0, open_cb, close_cb, read_cb, write_cb,
            setpos_cb, getpos_cb, filesize_cb, (void*)m_reader);
        if (hFile == MP4_INVALID_FILE_HANDLE)
        {
            console::error("Failed to open MP4 file.");
            return 0;
        }

        track = GetAACTrack(hFile);
        if (track < 1)
        {
            console::error("No valid AAC track found.");
            return 0;
        }

        buffer = NULL;
        buffer_size = 0;
        MP4GetTrackESConfiguration(hFile, track, &buffer, &buffer_size);
        if (!buffer)
        {
            console::error("Unable to read track specific configuration.");
            return 0;
        }

        AudioSpecificConfig((unsigned char*)buffer, buffer_size, &mp4ASC);

        int rc = faacDecInit2(hDecoder, (unsigned char*)buffer, buffer_size,
            (unsigned long*)&samplerate, (unsigned char*)&channels);
        if (buffer) free(buffer);
        if (rc < 0)
        {
            console::error("Unable to initialise FAAD2 library.");
            return 0;
        }

        numSamples = MP4GetTrackNumberOfSamples(hFile, track);
        sampleId = 1;

        unsigned __int64 length = MP4GetTrackDuration(hFile, track);
        __int64 msDuration = MP4ConvertFromTrackDuration(hFile, track,
            length, MP4_MSECS_TIME_SCALE);
        info->set_length((double)msDuration/1000.0);

        info->info_set_int("bitrate",(__int64)(1.0/1000.0 *
            (double)(__int64)MP4GetTrackIntegerProperty(hFile,
            track, "mdia.minf.stbl.stsd.mp4a.esds.decConfigDescr.avgBitrate")) + 0.5);
        info->info_set_int("channels", (__int64)channels);
        info->info_set_int("samplerate", (__int64)samplerate);
        if (mp4ASC.sbr_present_flag == 1)
            info->info_set("codec", "AAC+SBR");
        else
            info->info_set("codec", "AAC");

        ReadMP4Tag(info);

        return 1;
    }

    input_mp4()
    {
        hFile = MP4_INVALID_FILE_HANDLE;
        hDecoder = NULL;
        m_samples = 0;
    }

    ~input_mp4()
    {
        if (hFile != MP4_INVALID_FILE_HANDLE)
            MP4Close(hFile);
        if (hDecoder)
            faacDecClose(hDecoder);
    }

    virtual int run(audio_chunk * chunk)
    {
        faacDecFrameInfo frameInfo;
        unsigned char *buffer;
        unsigned __int32 buffer_size;
        audio_sample *sample_buffer;

        if (sampleId == MP4_INVALID_SAMPLE_ID)
        {
            console::error("Invalid sampleId.");
            return 0;
        }

        do {
            buffer = NULL;
            buffer_size = 0;

            MP4ReadSample(hFile, track, sampleId,
                (unsigned __int8**)&buffer, &buffer_size,
                NULL, NULL, NULL, NULL);
            sampleId++;

            sample_buffer = (audio_sample*)faacDecDecode(hDecoder, &frameInfo, buffer, buffer_size);

            if (buffer) free(buffer);

        } while ((frameInfo.error == 0) && (frameInfo.samples == 0));

        if (frameInfo.error)
        {
            console::warning(faacDecGetErrorMessage(frameInfo.error));
            console::warning("Skipping frame");
        }
        if (sampleId > numSamples)
            return 0;

        if (frameInfo.channels == 6 && frameInfo.num_lfe_channels)
        {
            //channel order for 5.1: L/R/C/LF/BL/BR
            audio_sample r1, r2, r3, r4, r5, r6;
            for (int i = 0; i < frameInfo.samples; i += frameInfo.channels)
            {
                r1 = sample_buffer[i];
                r2 = sample_buffer[i+1];
                r3 = sample_buffer[i+2];
                r4 = sample_buffer[i+3];
                r5 = sample_buffer[i+4];
                r6 = sample_buffer[i+5];
                sample_buffer[i] = r2;
                sample_buffer[i+1] = r3;
                sample_buffer[i+2] = r1;
                sample_buffer[i+3] = r6;
                sample_buffer[i+4] = r4;
                sample_buffer[i+5] = r5;
            }
        }

        if (frameInfo.channels == 0)
        {
            chunk->set_data(sample_buffer, 0, frameInfo.channels, frameInfo.samplerate);
        } else {
            chunk->set_data(sample_buffer, frameInfo.samples/frameInfo.channels,
                frameInfo.channels, frameInfo.samplerate);
        }

        return 1;
    }

    virtual set_info_t set_info(reader *r, const file_info * info)
    {
        m_reader = r;

        hFile = MP4ModifyCb(0, 0, open_cb, close_cb, read_cb, write_cb,
            setpos_cb, getpos_cb, filesize_cb, (void*)m_reader);
        if (hFile == MP4_INVALID_FILE_HANDLE) return SET_INFO_FAILURE;

        MP4MetadataDelete(hFile);

        /* replay gain writing */
        const char *p = NULL;

        p = info->info_get("REPLAYGAIN_TRACK_PEAK");
        if (p)
            MP4SetMetadataFreeForm(hFile, "REPLAYGAIN_TRACK_PEAK", (unsigned __int8*)p, strlen(p));
        p = info->info_get("REPLAYGAIN_TRACK_GAIN");
        if (p)
            MP4SetMetadataFreeForm(hFile, "REPLAYGAIN_TRACK_GAIN", (unsigned __int8*)p, strlen(p));
        p = info->info_get("REPLAYGAIN_ALBUM_PEAK");
        if (p)
            MP4SetMetadataFreeForm(hFile, "REPLAYGAIN_ALBUM_PEAK", (unsigned __int8*)p, strlen(p));
        p = info->info_get("REPLAYGAIN_ALBUM_GAIN");
        if (p)
            MP4SetMetadataFreeForm(hFile, "REPLAYGAIN_ALBUM_GAIN", (unsigned __int8*)p, strlen(p));

        if (m_samples > 0)
        {
            unsigned __int8 length[4];

            length[0] = (unsigned __int8)(((unsigned int)m_samples >> 24) & 0xFF);
            length[1] = (unsigned __int8)(((unsigned int)m_samples >> 16) & 0xFF);
            length[2] = (unsigned __int8)(((unsigned int)m_samples >>  8) & 0xFF);
            length[3] = (unsigned __int8)(((unsigned int)m_samples      ) & 0xFF);

            MP4SetMetadataFreeForm(hFile, "NDFL", length, 4);
        }

        int numItems = info->meta_get_count();
        if (numItems > 0)
        {
            for (int i = 0; i < numItems; i++)
            {
                char *pName = (char*)info->meta_enum_name(i);
                const char *val = info->meta_enum_value(i);

                if (stricmp(pName, "TITLE") == 0)
                {
                    MP4SetMetadataName(hFile, val);
                } else if (stricmp(pName, "ARTIST") == 0) {
                    MP4SetMetadataArtist(hFile, val);
                } else if (stricmp(pName, "WRITER") == 0) {
                    MP4SetMetadataWriter(hFile, val);
                } else if (stricmp(pName, "ALBUM") == 0) {
                    MP4SetMetadataAlbum(hFile, val);
                } else if (stricmp(pName, "DATE") == 0) {
                    MP4SetMetadataYear(hFile, val);
                } else if (stricmp(pName, "TOOL") == 0) {
                    MP4SetMetadataTool(hFile, val);
                } else if (stricmp(pName, "COMMENT") == 0) {
                    MP4SetMetadataComment(hFile, val);
                } else if (stricmp(pName, "GENRE") == 0) {
                    MP4SetMetadataGenre(hFile, val);
                } else if (stricmp(pName, "TRACKNUMBER") == 0) {
                    unsigned __int16 trkn = 0, tot = 0;
                    sscanf(val, "%d", &trkn);
                    MP4SetMetadataTrack(hFile, trkn, tot);
                } else if (stricmp(pName, "DISKNUMBER") == 0) {
                    unsigned __int16 disk = 0, tot = 0;
                    sscanf(val, "%d", &disk);
                    MP4SetMetadataDisk(hFile, disk, tot);
                } else if (stricmp(pName, "COMPILATION") == 0) {
                    unsigned __int8 cpil = 0;
                    sscanf(val, "%d", &cpil);
                    MP4SetMetadataCompilation(hFile, cpil);
                } else if (stricmp(pName, "TEMPO") == 0) {
                    unsigned __int16 tempo = 0;
                    sscanf(val, "%d", &tempo);
                    MP4SetMetadataTempo(hFile, tempo);
                } else {
                    MP4SetMetadataFreeForm(hFile, pName, (unsigned __int8*)val, strlen(val));
                }
            }
        }

        /* end */
        return SET_INFO_SUCCESS;
    }

    virtual bool seek(double seconds)
    {
        MP4Duration duration;

        duration = MP4ConvertToTrackDuration(hFile,
            track, seconds, MP4_SECS_TIME_SCALE);
        sampleId = MP4GetSampleIdFromTime(hFile,
            track, duration, 0);

        return 1;
    }

    virtual bool is_our_content_type(const char *url, const char *type)
    {
        return !stricmp(type, "audio/mp4") || !stricmp(type, "audio/x-mp4");
    }

private:

    reader *m_reader;

    faacDecHandle hDecoder;

    MP4FileHandle hFile;
    MP4SampleId sampleId, numSamples;
    MP4TrackId track;
    unsigned int m_samples;

    int ReadMP4Tag(file_info *info)
    {
        unsigned __int32 valueSize = 0;
        unsigned __int8* pValue;
        char* pName;
        int i = 0;

        do {
            valueSize = 0;

            MP4GetMetadataByIndex(hFile, i, (const char**)&pName, &pValue, &valueSize);

            if (valueSize > 0)
            {
                char* val = (char*)malloc((valueSize+1)*sizeof(char));
                memset(val, 0, (valueSize+1)*sizeof(char));
                memcpy(val, pValue, valueSize*sizeof(char));

                if (pName[0] == '©')
                {
                    if (memcmp(pName, "©nam", 4) == 0)
                    {
                        info->meta_add("TITLE", val);
                    } else if (memcmp(pName, "©ART", 4) == 0) {
                        info->meta_add("ARTIST", val);
                    } else if (memcmp(pName, "©wrt", 4) == 0) {
                        info->meta_add("WRITER", val);
                    } else if (memcmp(pName, "©alb", 4) == 0) {
                        info->meta_add("ALBUM", val);
                    } else if (memcmp(pName, "©day", 4) == 0) {
                        info->meta_add("DATE", val);
                    } else if (memcmp(pName, "©too", 4) == 0) {
                        info->info_set("TOOL", val);
                    } else if (memcmp(pName, "©cmt", 4) == 0) {
                        info->meta_add("COMMENT", val);
                    } else if (memcmp(pName, "©gen", 4) == 0) {
                        info->meta_add("GENRE", val);
                    } else {
                        info->meta_add(pName, val);
                    }
                } else if (memcmp(pName, "gnre", 4) == 0) {
                    char *t = NULL;
                    MP4GetMetadataGenre(hFile, &t);
                    info->meta_add("GENRE", t);
                } else if (memcmp(pName, "trkn", 4) == 0) {
                    unsigned __int16 trkn = 0, tot = 0;
                    char t[200];
                    MP4GetMetadataTrack(hFile, &trkn, &tot);
                    wsprintf(t, "%d", trkn);
                    info->meta_add("TRACKNUMBER", t);
                } else if (memcmp(pName, "disk", 4) == 0) {
                    unsigned __int16 disk = 0, tot = 0;
                    char t[200];
                    MP4GetMetadataDisk(hFile, &disk, &tot);
                    wsprintf(t, "%d", disk);
                    info->meta_add("DISKNUMBER", t);
                } else if (memcmp(pName, "cpil", 4) == 0) {
                    unsigned __int8 cpil = 0;
                    char t[200];
                    MP4GetMetadataCompilation(hFile, &cpil);
                    wsprintf(t, "%d", cpil);
                    info->meta_add("COMPILATION", t);
                } else if (memcmp(pName, "tmpo", 4) == 0) {
                    unsigned __int16 tempo = 0;
                    char t[200];
                    MP4GetMetadataTempo(hFile, &tempo);
                    wsprintf(t, "%d BPM", tempo);
                    info->meta_add("TEMPO", t);
                } else if (memcmp(pName, "NDFL", 4) == 0) {
                    unsigned __int8 *data = NULL;
                    unsigned __int32 valueSize = 0;
                    MP4GetMetadataFreeForm(hFile, "NDFL", &data, &valueSize);
                    if (data && valueSize == 4)
                    {
                        // len = number of samples in whole file per channel
                        m_samples = ((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]);
                    }
                } else {
                    float f = 0;
                    if (!stricmp(pName, "REPLAYGAIN_TRACK_PEAK"))
                    {
                        sscanf(val, "%f", &f);
                        info->info_set_replaygain_track_peak((double)f);
                    } else if (!stricmp(pName, "REPLAYGAIN_TRACK_GAIN")) {
                        sscanf(val, "%f", &f);
                        info->info_set_replaygain_track_gain((double)f);
                    } else if (!stricmp(pName, "REPLAYGAIN_ALBUM_PEAK")) {
                        sscanf(val, "%f", &f);
                        info->info_set_replaygain_album_peak((double)f);
                    } else if (!stricmp(pName, "REPLAYGAIN_ALBUM_GAIN")) {
                        sscanf(val, "%f", &f);
                        info->info_set_replaygain_album_gain((double)f);
                    } else {
                        info->meta_add(pName, val);
                    }
                }
            }

            i++;

        } while (valueSize > 0);

        return 1;
    }

    int GetAACTrack(MP4FileHandle infile)
    {
        /* find AAC track */
        int i, rc;
        int numTracks = MP4GetNumberOfTracks(infile, NULL, /* subType */ 0);

        for (i = 0; i < numTracks; i++)
        {
            MP4TrackId trackId = MP4FindTrackId(infile, i, NULL, /* subType */ 0);
            const char* trackType = MP4GetTrackType(infile, trackId);

            if (!strcmp(trackType, MP4_AUDIO_TRACK_TYPE))
            {
                unsigned char *buff = NULL;
                int buff_size = 0;
                mp4AudioSpecificConfig mp4ASC;

                MP4GetTrackESConfiguration(infile, trackId,
                    (unsigned __int8**)&buff, (unsigned __int32*)&buff_size);

                if (buff)
                {
                    rc = AudioSpecificConfig(buff, buff_size, &mp4ASC);
                    free(buff);

                    if (rc < 0)
                        return -1;
                    return trackId;
                }
            }
        }

        /* can't decode this */
        return -1;
    }

    /* file callback stuff */
    static unsigned __int32 open_cb(const char *pName,
        const char *mode, void *userData)
    {
        DBG_OUT("open_cb");
        return 1;
    }

    static void close_cb(void *userData)
    {
        DBG_OUT("close_cb");
        return;
    }

    static unsigned __int32 read_cb(void *pBuffer, unsigned int nBytesToRead,
        void *userData)
    {
        DBG_OUT("read_cb");
        reader *r = (reader*)userData;
        return r->read(pBuffer, nBytesToRead);
    }

    static unsigned __int32 write_cb(void *pBuffer, unsigned int nBytesToWrite,
        void *userData)
    {
        DBG_OUT("write_cb");
        reader *r = (reader*)userData;
        return r->write(pBuffer, nBytesToWrite);
    }

    static __int64 getpos_cb(void *userData)
    {
        DBG_OUT("getpos_cb");
        reader *r = (reader*)userData;
        return r->get_position();
    }

    static __int32 setpos_cb(unsigned __int32 pos, void *userData)
    {
        DBG_OUT("setpos_cb");
        reader *r = (reader*)userData;
        return !(r->seek(pos));
    }

    static __int64 filesize_cb(void *userData)
    {
        DBG_OUT("filesize_cb");
        reader *r = (reader*)userData;
        return r->get_length();
    }
};

struct seek_list
{
    seek_list *next;
    __int64 offset;
};

class input_aac : public input
{
public:

    virtual bool test_filename(const char * fn,const char * ext)
    {
        return !stricmp(ext,"AAC");
    }

    virtual bool open(reader *r, file_info *info, unsigned flags)
    {
        int tagsize = 0, tmp = 0;
        int bread = 0;
        double length = 1.;
        __int64 bitrate = 128;
        unsigned char channels = 0;
        unsigned long samplerate = 0;

        faacDecConfigurationPtr config;

        m_reader = r;

        hDecoder = faacDecOpen();
        if (!hDecoder)
        {
            console::error("Failed to open FAAD2 library.");
            return 0;
        }

        config = faacDecGetCurrentConfiguration(hDecoder);
        config->outputFormat = FAAD_FMT_DOUBLE;
        faacDecSetConfiguration(hDecoder, config);

        tag_reader::g_run_multi(m_reader, info, "ape|id3v2|lyrics3|id3v1");

        m_at_eof = 0;

        if (!(m_aac_buffer = (unsigned char*)malloc(768*6)))
        {
            console::error("Memory allocation error.");
            return 0;
        }
        memset(m_aac_buffer, 0, 768*6);

        bread = m_reader->read(m_aac_buffer, 768*6);
        m_aac_bytes_into_buffer = bread;
        m_aac_bytes_consumed = 0;
        m_file_offset = 0;

        if (bread != 768*6)
            m_at_eof = 1;

        if (!memcmp(m_aac_buffer, "ID3", 3))
        {
            /* high bit is not used */
            tagsize = (m_aac_buffer[6] << 21) | (m_aac_buffer[7] << 14) |
                (m_aac_buffer[8] <<  7) | (m_aac_buffer[9] <<  0);

            tagsize += 10;
            advance_buffer(tagsize);
        }

        m_head = (struct seek_list*)malloc(sizeof(struct seek_list));
        m_tail = m_head;
        m_tail->next = NULL;

        m_header_type = 0;
        if ((m_aac_buffer[0] == 0xFF) && ((m_aac_buffer[1] & 0xF6) == 0xF0))
        {
            if (m_reader->can_seek())
            {
                adts_parse(&bitrate, &length);
                m_reader->seek(tagsize);

                bread = m_reader->read(m_aac_buffer, 768*6);
                if (bread != 768*6)
                    m_at_eof = 1;
                else
                    m_at_eof = 0;
                m_aac_bytes_into_buffer = bread;
                m_aac_bytes_consumed = 0;

                m_header_type = 1;
            }
        } else if (memcmp(m_aac_buffer, "ADIF", 4) == 0) {
            int skip_size = (m_aac_buffer[4] & 0x80) ? 9 : 0;
            bitrate = ((unsigned int)(m_aac_buffer[4 + skip_size] & 0x0F)<<19) |
                ((unsigned int)m_aac_buffer[5 + skip_size]<<11) |
                ((unsigned int)m_aac_buffer[6 + skip_size]<<3) |
                ((unsigned int)m_aac_buffer[7 + skip_size] & 0xE0);

            length = (double)m_reader->get_length();
            if (length == -1.)
            {
                length = 1;
            } else {
                length = ((double)length*8.)/((double)bitrate) + 0.5;
            }

            bitrate = (__int64)((double)bitrate/1000.0 + 0.5);

            m_header_type = 2;
        }

        if (!m_reader->can_seek())
        {
            length = 0;
        }

        fill_buffer();
        if ((m_aac_bytes_consumed = faacDecInit(hDecoder,
            m_aac_buffer, m_aac_bytes_into_buffer,
            &samplerate, &channels)) < 0)
        {
            console::error("Can't initialize decoder library.");
            return 0;
        }
        advance_buffer(m_aac_bytes_consumed);

        info->info_set("codec", "AAC");
        if (m_header_type == 0)
            info->info_set("stream type", "RAW");
        else if (m_header_type == 1)
            info->info_set("stream type", "ADTS");
        else if (m_header_type == 2)
            info->info_set("stream type", "ADIF");
        info->set_length(length);

        info->info_set_int("bitrate", bitrate);
        info->info_set_int("channels", (__int64)channels);
        info->info_set_int("samplerate", (__int64)samplerate);

        m_samplerate = samplerate;

        return 1;
    }

    input_aac()
    {
        m_head = NULL;
        m_tail = NULL;
        cur_pos_sec = 0.0;
        m_samplerate = 0;
        hDecoder = NULL;
        m_aac_buffer = NULL;
    }

    ~input_aac()
    {
        struct seek_list *target = m_head;

        if (hDecoder)
            faacDecClose(hDecoder);
        if (m_aac_buffer)
            free(m_aac_buffer);

        while (target)
        {
            struct seek_list *tmp = target;
            target = target->next;
            if (tmp) free(tmp);
        }
    }

    virtual int run(audio_chunk * chunk)
    {
        faacDecFrameInfo frameInfo;
        audio_sample *sample_buffer;

        memset(&frameInfo, 0, sizeof(faacDecFrameInfo));

        do
        {
            fill_buffer();

            if (m_aac_bytes_into_buffer != 0)
            {
                sample_buffer = (audio_sample*)faacDecDecode(hDecoder, &frameInfo,
                    m_aac_buffer, m_aac_bytes_into_buffer);

                if (m_header_type != 1)
                {
                    m_tail->offset = m_file_offset;
                    m_tail->next = (struct seek_list*)malloc(sizeof(struct seek_list));
                    m_tail = m_tail->next;
                    m_tail->next = NULL;
                }

                advance_buffer(frameInfo.bytesconsumed);
            } else {
                break;
            }

        } while (!frameInfo.samples && !frameInfo.error);

        if (frameInfo.error || (m_aac_bytes_into_buffer == 0))
        {
            if (frameInfo.error)
            {
                if (faacDecGetErrorMessage(frameInfo.error) != NULL)
                    console::error(faacDecGetErrorMessage(frameInfo.error));
            }
            return 0;
        }

        if (frameInfo.channels == 6 && frameInfo.num_lfe_channels)
        {
            //channel order for 5.1: L/R/C/LF/BL/BR
            audio_sample r1, r2, r3, r4, r5, r6;
            for (int i = 0; i < frameInfo.samples; i += frameInfo.channels)
            {
                r1 = sample_buffer[i];
                r2 = sample_buffer[i+1];
                r3 = sample_buffer[i+2];
                r4 = sample_buffer[i+3];
                r5 = sample_buffer[i+4];
                r6 = sample_buffer[i+5];
                sample_buffer[i] = r2;
                sample_buffer[i+1] = r3;
                sample_buffer[i+2] = r1;
                sample_buffer[i+3] = r6;
                sample_buffer[i+4] = r4;
                sample_buffer[i+5] = r5;
            }
        }

        if (chunk)
        {
            if (frameInfo.channels == 0)
            {
                chunk->set_data(sample_buffer, 0, frameInfo.channels, frameInfo.samplerate);
            } else {
                chunk->set_data(sample_buffer, frameInfo.samples/frameInfo.channels,
                    frameInfo.channels, frameInfo.samplerate);
            }
        }
        m_samplerate = frameInfo.samplerate;

        cur_pos_sec += 1024.0/(double)m_samplerate;

        return 1;
    }

    virtual set_info_t set_info(reader *r,const file_info * info)
    {
        return tag_writer::g_run(r,info,"ape") ? SET_INFO_SUCCESS : SET_INFO_FAILURE;
    }

    virtual bool seek(double seconds)
    {
        int i, frames;
        int bread;
        struct seek_list *target = m_head;

        if (m_reader->can_seek() && ((m_header_type == 1) || (seconds < cur_pos_sec)))
        {
            frames = (int)(seconds*((double)m_samplerate/1024.0) + 0.5);

            for (i = 0; i < frames; i++)
            {
                if (target->next)
                    target = target->next;
                else
                    return 1;
            }
            if (target->offset == 0 && frames > 0)
                return 1;
            m_reader->seek(target->offset);

            bread = m_reader->read(m_aac_buffer, 768*6);
            if (bread != 768*6)
                m_at_eof = 1;
            else
                m_at_eof = 0;
            m_aac_bytes_into_buffer = bread;
            m_aac_bytes_consumed = 0;

            return 1;
        } else {
            if (seconds > cur_pos_sec)
            {
                frames = (int)((seconds - cur_pos_sec)*((double)m_samplerate/1024.0) + 0.5);

                if (frames > 0)
                {
                    for (i = 0; i < frames; i++)
                    {
                        if (!run(NULL))
                            return 1;
                    }
                }

                return 1;
            } else {
                return 0;
            }
        }
    }

    virtual bool is_our_content_type(const char *url, const char *type)
    {
        return !stricmp(type, "audio/aac") || !stricmp(type, "audio/x-aac");
    }

private:

    reader *m_reader;

    faacDecHandle hDecoder;

    long m_aac_bytes_into_buffer;
    long m_aac_bytes_consumed;
    __int64 m_file_offset;
    unsigned char *m_aac_buffer;
    int m_at_eof;

    unsigned long m_samplerate;
    double cur_pos_sec;
    int m_header_type;

    struct seek_list *m_head;
    struct seek_list *m_tail;

    int fill_buffer()
    {
        int bread;

        if (m_aac_bytes_consumed > 0)
        {
            if (m_aac_bytes_into_buffer)
            {
                memmove((void*)m_aac_buffer, (void*)(m_aac_buffer + m_aac_bytes_consumed),
                    m_aac_bytes_into_buffer*sizeof(unsigned char));
            }

            if (!m_at_eof)
            {
                bread = m_reader->read((void*)(m_aac_buffer + m_aac_bytes_into_buffer),
                    m_aac_bytes_consumed);

                if (bread != m_aac_bytes_consumed)
                    m_at_eof = 1;

                m_aac_bytes_into_buffer += bread;
            }

            m_aac_bytes_consumed = 0;

            if (m_aac_bytes_into_buffer > 3)
            {
                if (memcmp(m_aac_buffer, "TAG", 3) == 0)
                    m_aac_bytes_into_buffer = 0;
            }
            if (m_aac_bytes_into_buffer > 11)
            {
                if (memcmp(m_aac_buffer, "LYRICSBEGIN", 11) == 0)
                    m_aac_bytes_into_buffer = 0;
            }
            if (m_aac_bytes_into_buffer > 8)
            {
                if (memcmp(m_aac_buffer, "APETAGEX", 8) == 0)
                    m_aac_bytes_into_buffer = 0;
            }
        }

        return 1;
    }

    void advance_buffer(int bytes)
    {
        m_file_offset += bytes;
        m_aac_bytes_consumed = bytes;
        m_aac_bytes_into_buffer -= bytes;
    }

    int adts_parse(__int64 *bitrate, double *length)
    {
        static int sample_rates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000};
        int frames, frame_length;
        int t_framelength = 0;
        int samplerate;
        double frames_per_sec, bytes_per_frame;

        /* Read all frames to ensure correct time and bitrate */
        for (frames = 0; /* */; frames++)
        {
            fill_buffer();

            if (m_aac_bytes_into_buffer > 7)
            {
                /* check syncword */
                if (!((m_aac_buffer[0] == 0xFF)&&((m_aac_buffer[1] & 0xF6) == 0xF0)))
                    break;

                m_tail->offset = m_file_offset;
                m_tail->next = (struct seek_list*)malloc(sizeof(struct seek_list));
                m_tail = m_tail->next;
                m_tail->next = NULL;

                if (frames == 0)
                    samplerate = sample_rates[(m_aac_buffer[2]&0x3c)>>2];

                frame_length = ((((unsigned int)m_aac_buffer[3] & 0x3)) << 11)
                    | (((unsigned int)m_aac_buffer[4]) << 3) | (m_aac_buffer[5] >> 5);

                t_framelength += frame_length;

                if (frame_length > m_aac_bytes_into_buffer)
                    break;

                advance_buffer(frame_length);
            } else {
                break;
            }
        }

        frames_per_sec = (double)samplerate/1024.0;
        if (frames != 0)
            bytes_per_frame = (double)t_framelength/(double)(frames*1000);
        else
            bytes_per_frame = 0;
        *bitrate = (__int64)(8. * bytes_per_frame * frames_per_sec + 0.5);
        if (frames_per_sec != 0)
            *length = (double)frames/frames_per_sec;
        else
            *length = 1;

        return 1;
    }
};

static service_factory_t<input,input_mp4> foo_mp4;
static service_factory_t<input,input_aac> foo_aac;
