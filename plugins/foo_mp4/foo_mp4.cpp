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
** $Id: foo_mp4.cpp,v 1.75 2003/11/22 18:08:05 ca5e Exp $
**/

#include <mp4.h>
#include <faad.h>
#include "foobar2000/SDK/foobar2000.h"
#include "foobar2000/foo_input_std/id3v2_hacks.h"
#include "convert.h"

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
                           "1.69",
                           "Based on FAAD2 v" FAAD2_VERSION "\nCopyright (C) 2002-2003 http://www.audiocoding.com" );

static const char *object_type_string(int type)
{
    static const char *types[31] = {
        "AAC Main",
        "AAC LC",
        "AAC SSR",
        "AAC LTP",
        "AAC HE",
        "AAC Scalable",
        "TwinVQ",
        "CELP",
        "HVXC",
        "Reserved",
        "Reserved",
        "TTSI",
        "Main synthetic",
        "Wavetable synthesis",
        "General MIDI",
        "Algorithmic Synthesis and Audio FX",
        "ER AAC LC",
        "Reserved",
        "ER AAC LTP",
        "ER AAC scalable",
        "ER TwinVQ",
        "ER BSAC",
        "ER AAC LD",
        "ER CELP",
        "ER HVXC",
        "ER HILN",
        "ER Parametric",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
    };

    if (type<1 || type>31) return NULL;

    return types[type-1];
}

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

        track = GetAudioTrack(hFile);
        if (track < 0)
        {
            console::error("No valid audio track found.");
            return 0;
        }

        unsigned __int8 audioType = MP4GetTrackEsdsObjectTypeId(hFile, track);
        if (!MP4_IS_AAC_AUDIO_TYPE(audioType))
        {
            console::error("No valid AAC track found.");
            return 0;
        }

        buffer = NULL;
        buffer_size = 0;
        MP4GetTrackESConfiguration(hFile, track, &buffer, &buffer_size);

        int ADIF_ASC = 0;
        if ((buffer_size >= 4) && ((buffer[0] == 'A') && (buffer[1] == 'D') &&
            (buffer[2] == 'I') && (buffer[3] == 'F')))
        {
            ADIF_ASC = 1;
        }

        if (buffer_size == 0 || ADIF_ASC == 1 /*MP4_IS_MPEG2_AAC_AUDIO_TYPE(audioType)*/)
        {
            int rc = faacDecInit(hDecoder, (unsigned char*)buffer, buffer_size,
                (unsigned long*)&samplerate, (unsigned char*)&channels);
            if (buffer) free(buffer);
            if (rc < 0)
            {
                console::error("Unable to initialise FAAD2 library.");
                return 0;
            }

            mp4ASC.frameLengthFlag = 0;
            mp4ASC.objectTypeIndex = audioType - 0x65;
        } else {
            AudioSpecificConfig((unsigned char*)buffer, buffer_size, &mp4ASC);

            int rc = faacDecInit2(hDecoder, (unsigned char*)buffer, buffer_size,
                (unsigned long*)&samplerate, (unsigned char*)&channels);
            if (buffer) free(buffer);
            if (rc < 0)
            {
                console::error("Unable to initialise FAAD2 library.");
                return 0;
            }
        }

        numSamples = MP4GetTrackNumberOfSamples(hFile, track);
        sampleId = 1;

        m_timescale = MP4GetTrackTimeScale(hFile, track);
        m_samplerate = samplerate;
        m_seekto = 0;
        m_framesize = 1024;
        if (mp4ASC.frameLengthFlag == 1) m_framesize = 960;
        useAacLength = false;

        {
            MP4Timestamp sample_pos;
            MP4Duration sample_dur;
            unsigned char *buf = NULL;
            unsigned __int32 buf_size = 0;
            MP4ReadSample(hFile, track, 1, (unsigned __int8**)&buf,
                &buf_size, &sample_pos, &sample_dur, NULL, NULL);
            if (buf) free(buf);
            m_delaydur = (unsigned int)sample_dur;
        }
        MP4Duration trackDuration = MP4GetTrackDuration(hFile, track) - m_delaydur;
        m_length = (double)(__int64)trackDuration / (double)m_timescale;
        info->set_length(m_length);
        info->info_set_int("bitrate",(__int64)(1.0/1000.0 *
            (double)(__int64)MP4GetTrackIntegerProperty(hFile,
            track, "mdia.minf.stbl.stsd.mp4a.esds.decConfigDescr.avgBitrate") + 0.5));
        info->info_set_int("channels", (__int64)channels);
        info->info_set_int("samplerate", (__int64)samplerate);

        const char *profile_str = object_type_string(mp4ASC.objectTypeIndex);
        if (profile_str)
            info->info_set("aac_profile", profile_str);

        if (mp4ASC.sbr_present_flag == 1) {
            info->info_set("codec", "AAC+SBR");
            m_framesize *= 2;
        } else {
            info->info_set("codec", "AAC");
        }

        if (flags & OPEN_FLAG_GET_INFO)
            ReadMP4Tag(info);

        return 1;
    }

    input_mp4()
    {
        hFile = MP4_INVALID_FILE_HANDLE;
        hDecoder = NULL;
        m_eof = false;
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
        audio_sample *sample_buffer;
        MP4Timestamp sample_pos;
        MP4Duration sample_dur;
        unsigned __int64 sample_count;
        unsigned __int64 delay = 0;
        bool initial = (sampleId == 1);

        do {
            if (m_eof || (sampleId > numSamples)) return 0;

            if (sampleId == MP4_INVALID_SAMPLE_ID)
            {
                console::error("Invalid sampleId.");
                return 0;//-1;
            }

            do {
                unsigned char *buffer = NULL;
                unsigned __int32 buffer_size = 0;
                delay = 0;

                MP4ReadSample(hFile, track, sampleId,
                    (unsigned __int8**)&buffer, &buffer_size,
                    &sample_pos, &sample_dur, NULL, NULL);
                if (sampleId == 1) sample_dur -= m_delaydur;
                sample_pos -= m_delaydur;
                sampleId++;

                sample_buffer = (audio_sample*)faacDecDecode(hDecoder, &frameInfo, buffer, buffer_size);

                if (buffer) free(buffer);

                if (useAacLength || (m_timescale != m_samplerate)) {
                    sample_count = frameInfo.channels ? frameInfo.samples/frameInfo.channels : 0;
                } else {
                    sample_count = sample_dur;

                    if (!useAacLength && !initial && m_seekto<sample_pos && (sampleId < numSamples/2) && (sample_dur*frameInfo.channels != frameInfo.samples))
                    {
                        console::info("MP4 seems to have incorrect frame duration, using values from AAC data");
                        useAacLength = true;
                    }
                }

                if (initial && (sample_count < m_framesize) && frameInfo.channels)
                    delay = (frameInfo.samples/frameInfo.channels) - sample_count;

                if (frameInfo.error || !sample_buffer)
                {
                    const char *msg;
                    if (frameInfo.error)
                        msg = faacDecGetErrorMessage(frameInfo.error);
                    else
                        msg = "faacDecDecode() error";
                    console::warning(msg);
                    if (sampleId > numSamples) return 0;//-1;
                    console::info("Skipping frame");
                    sample_count = 0;
                }
            } while (frameInfo.error || frameInfo.samples == 0 || frameInfo.channels == 0 || sample_count == 0);

            unsigned __int64 skip = (sample_pos < m_seekto) ? (m_seekto - sample_pos) : 0;

            if (skip < sample_count)
            {
                if (frameInfo.channels == 6 && frameInfo.num_lfe_channels)
                {
                    //channel order for 5.1: L/R/C/LF/BL/BR
                    audio_sample r1, r2, r3, r4, r5, r6;
                    for (unsigned int i = 0; i < frameInfo.samples; i += frameInfo.channels)
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

                unsigned int samples = (unsigned int)(sample_count - skip);

                chunk->set_data((audio_sample*)sample_buffer + (unsigned int)(skip+delay)*frameInfo.channels,
                    samples, frameInfo.channels, frameInfo.samplerate);

                m_seekto = 0;
            }
        } while (sample_pos + sample_dur <= m_seekto);

        return 1;
    }

    virtual set_info_t set_info(reader *r, const file_info * info)
    {
        m_reader = r;

        hFile = MP4ModifyCb(0, 0, open_cb, close_cb, read_cb, write_cb,
            setpos_cb, getpos_cb, filesize_cb, (void*)m_reader);
        if (hFile == MP4_INVALID_FILE_HANDLE) return SET_INFO_FAILURE;

        MP4MetadataDelete(hFile);
        MP4Close(hFile);

        hFile = MP4ModifyCb(0, 0, open_cb, close_cb, read_cb, write_cb,
            setpos_cb, getpos_cb, filesize_cb, (void*)m_reader);
        if (hFile == MP4_INVALID_FILE_HANDLE) return SET_INFO_FAILURE;

        const char *p = info->info_get("TOOL");
        if (p && *p)
            MP4SetMetadataTool(hFile, p);
        p = info->info_get("REPLAYGAIN_TRACK_PEAK");
        if (p && *p)
            MP4SetMetadataFreeForm(hFile, "REPLAYGAIN_TRACK_PEAK", (unsigned __int8*)p, strlen(p));
        p = info->info_get("REPLAYGAIN_TRACK_GAIN");
        if (p && *p)
            MP4SetMetadataFreeForm(hFile, "REPLAYGAIN_TRACK_GAIN", (unsigned __int8*)p, strlen(p));
        p = info->info_get("REPLAYGAIN_ALBUM_PEAK");
        if (p && *p)
            MP4SetMetadataFreeForm(hFile, "REPLAYGAIN_ALBUM_PEAK", (unsigned __int8*)p, strlen(p));
        p = info->info_get("REPLAYGAIN_ALBUM_GAIN");
        if (p && *p)
            MP4SetMetadataFreeForm(hFile, "REPLAYGAIN_ALBUM_GAIN", (unsigned __int8*)p, strlen(p));

        int numItems = info->meta_get_count();
        for (int i = 0; i < numItems; i++)
        {
            char *pName = (char *)info->meta_enum_name(i);
            const char *val = info->meta_enum_value(i);
            if (!pName || (pName && !*pName) || !val || (val && !*val)) continue;

            if (!stricmp(pName, "TOTALTRACKS") || !stricmp(pName, "TOTALDISCS")) continue;

            if (stricmp(pName, "TITLE") == 0)
            {
                MP4SetMetadataName(hFile, val);
            } else if (stricmp(pName, "ARTIST") == 0) {
                MP4SetMetadataArtist(hFile, val);
            } else if (stricmp(pName, "WRITER") == 0) {
                MP4SetMetadataWriter(hFile, val);
            } else if (stricmp(pName, "ALBUM") == 0) {
                MP4SetMetadataAlbum(hFile, val);
            } else if (stricmp(pName, "YEAR") == 0 || stricmp(pName, "DATE") == 0) {
                MP4SetMetadataYear(hFile, val);
            } else if (stricmp(pName, "COMMENT") == 0) {
                MP4SetMetadataComment(hFile, val);
            } else if (stricmp(pName, "GENRE") == 0) {
                MP4SetMetadataGenre(hFile, val);
            } else if (stricmp(pName, "TRACKNUMBER") == 0 || stricmp(pName, "TRACK") == 0) {
                int t1 = 0, t2 = 0;
                sscanf(val, "%d/%d", &t1, &t2);
                unsigned __int16 trkn = t1, tot = t2;
                const char *t = info->meta_get("TOTALTRACKS");
                if (t && *t) tot = atoi(t);
                MP4SetMetadataTrack(hFile, trkn, tot);
            } else if (stricmp(pName, "DISKNUMBER") == 0 || stricmp(pName, "DISC") == 0) {
                int t1 = 0, t2 = 0;
                sscanf(val, "%d/%d", &t1, &t2);
                unsigned __int16 disk = t1, tot = t2;
                const char *t = info->meta_get("TOTALDISCS");
                if (t && *t) tot = atoi(t);
                MP4SetMetadataDisk(hFile, disk, tot);
            } else if (stricmp(pName, "COMPILATION") == 0) {
                unsigned __int8 cpil = atoi(val);
                MP4SetMetadataCompilation(hFile, cpil);
            } else if (stricmp(pName, "TEMPO") == 0) {
                unsigned __int16 tempo = atoi(val);
                MP4SetMetadataTempo(hFile, tempo);
            } else {
                MP4SetMetadataFreeForm(hFile, pName, (unsigned __int8*)val, strlen(val));
            }
        }

        MP4Close(hFile);
        hFile = MP4_INVALID_FILE_HANDLE;
        /* end */
        return SET_INFO_SUCCESS;
    }

    virtual bool seek(double seconds)
    {
        if (seconds >= m_length) {
            m_eof = true;
            return true;
        }

        unsigned int frame = (unsigned int)((double)m_framesize * ((double)m_timescale / (double)m_samplerate) + 0.5);
        if (frame == 0) frame = 1;
        m_seekto = (unsigned __int64)(seconds * m_timescale + 0.5) + frame;
        MP4Duration target = m_seekto - frame + m_delaydur;

        while (1) {
            MP4Duration duration = MP4ConvertToTrackDuration(hFile, track, target, m_timescale);
            sampleId = MP4GetSampleIdFromTime(hFile, track, duration, 0);
            if (sampleId == MP4_INVALID_SAMPLE_ID) return false;
            MP4Timestamp position = MP4GetSampleTime(hFile, track, sampleId);
            if (position <= m_seekto + m_delaydur) break;
            if (target == 0) return false;
            if (target > frame) target -= frame; else target = 0;
        }

        /*
        unsigned int frame = (unsigned int)((double)m_framesize * ((double)m_samplerate / (double)m_timescale) + 0.5);
        if (frame == 0) frame = 1;

        m_seekto = (unsigned __int64)(seconds * m_timescale + 0.5) + frame;
        MP4Duration target = m_seekto - frame;

        while (1) {
            MP4Duration duration = MP4ConvertToTrackDuration(hFile, track, target, m_timescale);
            sampleId = MP4GetSampleIdFromTime(hFile, track, duration, 0);
            if (sampleId == MP4_INVALID_SAMPLE_ID) return false;
            MP4Timestamp position = MP4GetSampleTime(hFile, track, sampleId);
            if (position <= m_seekto) break;
            if (target == 0) return false;
            if (target > frame) target -= frame; else target = 0;
        }
        */

        faacDecPostSeekReset(hDecoder, -1);

        return true;
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
    unsigned int m_timescale;
    unsigned int m_samplerate;
    unsigned int m_framesize;
    unsigned int m_delaydur;
    unsigned __int64 m_seekto;
    double m_length;
    bool m_eof;
    bool useAacLength;

    int ReadMP4Tag(file_info *info)
    {
        unsigned __int32 valueSize;
        unsigned __int8 *pValue;
        char *pName;
        unsigned int i = 0;

        do {
            valueSize = 0;
            pValue = 0;
            pName = 0;

            if (MP4GetMetadataByIndex(hFile, i, (const char **)&pName, &pValue, &valueSize) &&
                valueSize > 0 && pName && pValue)
            {
                char *val = (char *)malloc((valueSize+1)*sizeof(char));

                if (val)
                {
                    memcpy(val, pValue, valueSize*sizeof(char));
                    val[valueSize] = '\0';

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
                            info->info_set("tool", val);
                        } else if (memcmp(pName, "©cmt", 4) == 0) {
                            info->meta_add("COMMENT", val);
                        } else if (memcmp(pName, "©gen", 4) == 0) {
                            info->meta_add("GENRE", val);
                        } else {
                            info->meta_add(pName, val);
                        }
                    } else if (memcmp(pName, "gnre", 4) == 0) {
                        char *t=0;
                        if (MP4GetMetadataGenre(hFile, &t) && t)
                            info->meta_add("GENRE", t);
                    } else if (memcmp(pName, "trkn", 4) == 0) {
                        unsigned __int16 trkn = 0, tot = 0;
                        if (MP4GetMetadataTrack(hFile, &trkn, &tot))
                        {
                            char t[64];
                            if (tot > 0)
                            {
                                wsprintf(t, "%d/%d", (int)trkn, (int)tot);
                                info->meta_add("TRACKNUMBER", t);
                                wsprintf(t, "%d", (int)tot);
                                info->meta_add("TOTALTRACKS", t);
                            }
                            else
                            {
                                wsprintf(t, "%d", (int)trkn);
                                info->meta_add("TRACKNUMBER", t);
                            }
                        }
                    } else if (memcmp(pName, "disk", 4) == 0) {
                        unsigned __int16 disk = 0, tot = 0;
                        if (MP4GetMetadataDisk(hFile, &disk, &tot))
                        {
                            char t[64];
                            if (tot > 0)
                            {
                                wsprintf(t, "%d/%d", (int)disk, (int)tot);
                                info->meta_add("DISC", t);
                                wsprintf(t, "%d", (int)tot);
                                info->meta_add("TOTALDISCS", t);
                            }
                            else
                            {
                                wsprintf(t, "%d", (int)disk);
                                info->meta_add("DISC", t);
                            }
                            //info->meta_add("DISKNUMBER", t);
                        }
                    } else if (memcmp(pName, "cpil", 4) == 0) {
                        unsigned __int8 cpil = 0;
                        if (MP4GetMetadataCompilation(hFile, &cpil))
                        {
                            char t[64];
                            wsprintf(t, "%d", (int)cpil);
                            info->meta_add("COMPILATION", t);
                        }
                    } else if (memcmp(pName, "tmpo", 4) == 0) {
                        unsigned __int16 tempo = 0;
                        if (MP4GetMetadataTempo(hFile, &tempo))
                        {
                            char t[64];
                            wsprintf(t, "%d BPM", (int)tempo);
                            info->meta_add("TEMPO", t);
                        }
                    } else if (memcmp(pName, "NDFL", 4) == 0) {
                        /* Removed */
                    } else {
                        if (!stricmp(pName, "REPLAYGAIN_TRACK_PEAK"))
                        {
                            info->info_set_replaygain_track_peak(pfc_string_to_float(val));
                        } else if (!stricmp(pName, "REPLAYGAIN_TRACK_GAIN")) {
                            info->info_set_replaygain_track_gain(pfc_string_to_float(val));
                        } else if (!stricmp(pName, "REPLAYGAIN_ALBUM_PEAK")) {
                            info->info_set_replaygain_album_peak(pfc_string_to_float(val));
                        } else if (!stricmp(pName, "REPLAYGAIN_ALBUM_GAIN")) {
                            info->info_set_replaygain_album_gain(pfc_string_to_float(val));
                        } else {
                            info->meta_add(pName, val);
                        }
                    }

                    free(val);
                }
            }

            i++;
        } while (valueSize > 0);

        return 1;
    }

    int GetAudioTrack(MP4FileHandle infile)
    {
        /* find AAC track */
        int i;
        int numTracks = MP4GetNumberOfTracks(infile, NULL, 0);

        for (i = 0; i < numTracks; i++)
        {
            MP4TrackId trackId = MP4FindTrackId(infile, i, NULL, 0);
            const char* trackType = MP4GetTrackType(infile, trackId);

            if (!strcmp(trackType, MP4_AUDIO_TRACK_TYPE))
            {
                return trackId;
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
        int sbr = 0;
        int header_type = 0;
        int profile = 0;

        m_reader = r;
        tagsize = (int)id3v2_calc_size(m_reader);
        if (tagsize<0) return 0;

        if (!(m_aac_buffer = (unsigned char*)malloc(768*6)))
        {
            console::error("Memory allocation error.");
            return 0;
        }

        for (int init=0; init<2; init++)
        {
            faacDecConfigurationPtr config;

            hDecoder = faacDecOpen();
            if (!hDecoder)
            {
                console::error("Failed to open FAAD2 library.");
                return 0;
            }

            config = faacDecGetCurrentConfiguration(hDecoder);
            config->outputFormat = FAAD_FMT_DOUBLE;
            faacDecSetConfiguration(hDecoder, config);

            memset(m_aac_buffer, 0, 768*6);
            bread = m_reader->read(m_aac_buffer, 768*6);
            m_aac_bytes_into_buffer = bread;
            m_aac_bytes_consumed = 0;
            m_file_offset = 0;
            m_last_offset = -1;
            m_at_eof = (bread != 768*6) ? 1 : 0;

            if (init==0)
            {
                faacDecFrameInfo frameInfo;

                fill_buffer();
                if ((m_aac_bytes_consumed = faacDecInit(hDecoder,
                    m_aac_buffer, m_aac_bytes_into_buffer,
                    &samplerate, &channels)) < 0)
                {
                    console::error("Can't initialize decoder library.");
                    return 0;
                }
                advance_buffer(m_aac_bytes_consumed);

                do {
                    memset(&frameInfo, 0, sizeof(faacDecFrameInfo));
                    fill_buffer();
                    faacDecDecode(hDecoder, &frameInfo, m_aac_buffer, m_aac_bytes_into_buffer);
                } while (!frameInfo.samples && !frameInfo.error);

                if (frameInfo.error)
                {
                    console::error(faacDecGetErrorMessage(frameInfo.error));
                    return 0;
                }

                m_samplerate = frameInfo.samplerate;
                m_framesize = (frameInfo.channels != 0) ? frameInfo.samples/frameInfo.channels : 0;
                sbr = frameInfo.sbr;
                profile = frameInfo.object_type;
                header_type = frameInfo.header_type;

                faacDecClose(hDecoder);
                m_reader->seek(tagsize);
            }
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

        m_length = length;
        info->set_length(m_length);

        if (flags & OPEN_FLAG_GET_INFO) {
            const char *profile_str = object_type_string(profile);
            const char *header_str = NULL;

            info->info_set_int("bitrate", bitrate);
            info->info_set_int("channels", (__int64)channels);
            info->info_set_int("samplerate", (__int64)m_samplerate);

            if (profile_str)
                info->info_set("aac_profile", profile_str);

            if (header_type == RAW)
                header_str = "RAW";
            else if (header_type == ADIF)
                header_str = "ADIF";
            else if (header_type == ADTS)
                header_str = "ADTS";

            if (header_str)
                info->info_set("aac_header_type", header_str);

            if (sbr == 1 || sbr == 2) /* SBR: 0: off, 1: on; upsample, 2: on; downsampled, 3: off; upsampled */
                info->info_set("codec", "AAC+SBR");
            else
                info->info_set("codec", "AAC");

            tag_reader::g_run_multi(m_reader, info, "ape|id3v2|lyrics3|id3v1");
        }

        return 1;
    }

    input_aac()
    {
        m_head = NULL;
        m_tail = NULL;
        m_samplerate = 0;
        hDecoder = NULL;
        m_aac_buffer = NULL;
        m_samples = 0;
        m_samplepos = 0;
        m_seekskip = 0;
        m_eof = false;
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
        while (1) {
            if (m_eof || (m_samples > 0 && m_samplepos >= m_samples)) return 0; // gapless playback

            if (m_aac_bytes_into_buffer == 0) return 0;

            faacDecFrameInfo frameInfo;
            audio_sample *sample_buffer = 0;

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
                        if (m_last_offset < m_file_offset)
                        {
                            m_tail->offset = m_file_offset;
                            m_tail->next = (struct seek_list*)malloc(sizeof(struct seek_list));
                            m_tail = m_tail->next;
                            m_tail->next = NULL;
                            m_last_offset = m_file_offset;
                        }
                    }

                    advance_buffer(frameInfo.bytesconsumed);
                }

                if (frameInfo.error || !sample_buffer)
                {
                    if (!frameInfo.error) return 0; // EOF
                    const char *msg = faacDecGetErrorMessage(frameInfo.error);
                    if (msg) console::error(msg);
                    return 0; //-1;
                }

                if (m_aac_bytes_into_buffer == 0) break;
            } while (!frameInfo.samples || !frameInfo.channels);

            if (!frameInfo.samples || !frameInfo.channels) return 0;

            unsigned int samples = frameInfo.samples/frameInfo.channels;

            m_samplerate = frameInfo.samplerate;
            m_framesize = samples;

            if (m_samples > 0) { // gapless playback
                if (m_samplepos + samples > m_samples) samples = (unsigned int)(m_samples - m_samplepos);
            }

            m_samplepos += samples;

            if ((unsigned)m_seekskip < samples) {
                if (frameInfo.channels == 6 && frameInfo.num_lfe_channels)
                {
                    //channel order for 5.1: L/R/C/LF/BL/BR
                    audio_sample r1, r2, r3, r4, r5, r6;
                    for (unsigned int i = 0; i < frameInfo.samples; i += frameInfo.channels)
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

                samples -= m_seekskip;
                if (chunk)
                {
                    chunk->set_data((audio_sample*)sample_buffer + m_seekskip*frameInfo.channels,
                        samples, frameInfo.channels, frameInfo.samplerate);
                }
                m_seekskip = 0;
                break;
            } else {
                m_seekskip -= samples;
            }
        }

        return 1;
    }

    virtual set_info_t set_info(reader *r,const file_info *info)
    {
        tag_remover::g_run(r);
        return tag_writer::g_run(r,info,"ape") ? SET_INFO_SUCCESS : SET_INFO_FAILURE;
    }

    virtual bool seek(double seconds)
    {
        unsigned int i, frames;
        int bread;
        struct seek_list *target = m_head;

        if (seconds >= m_length) {
            m_eof = true;
            return true;
        }

        double cur_pos_sec = (double)(__int64)m_samplepos / (double)(__int64)m_samplerate;

        if (m_reader->can_seek() && ((m_header_type == 1) || (seconds < cur_pos_sec)))
        {
            frames = (unsigned int)(seconds*((double)m_samplerate/(double)m_framesize));
            if (frames > 1) frames--;

            for (i = 0; i < frames; i++)
            {
                if (target->next)
                    target = target->next;
                else
                    return false;
            }
            if (target->offset == 0 && frames > 0)
                return false;
            m_file_offset = target->offset;
            m_reader->seek(m_file_offset);

            bread = m_reader->read(m_aac_buffer, 768*6);
            if (bread != 768*6)
                m_at_eof = 1;
            else
                m_at_eof = 0;
            m_aac_bytes_into_buffer = bread;
            m_aac_bytes_consumed = 0;
            m_file_offset += bread;
            m_samplepos =(frames > 1) ? (unsigned __int64)(frames-1) * m_framesize : 0;
            m_seekskip = (int)((unsigned __int64)(seconds * m_samplerate + 0.5) - m_samplepos);// + m_framesize;
            if (m_seekskip < 0) return false; // should never happen
            faacDecPostSeekReset(hDecoder, -1);

            faacDecFrameInfo frameInfo;
            memset(&frameInfo, 0, sizeof(faacDecFrameInfo));
            fill_buffer();
            faacDecDecode(hDecoder, &frameInfo, m_aac_buffer, m_aac_bytes_into_buffer);

            return true;
        } else {
            if (seconds > cur_pos_sec)
            {
                frames = (unsigned int)((seconds - cur_pos_sec)*((double)m_samplerate/(double)m_framesize));

                if (frames > 0)
                {
                    for (i = 0; i < frames; i++)
                    {
                        if (!run(NULL))
                            return false;
                    }
                }

                m_seekskip = (int)((unsigned __int64)(seconds * m_samplerate + 0.5) - m_samplepos);
                if (m_seekskip < 0) return false; // should never happen
                faacDecPostSeekReset(hDecoder, -1);
            }
            return true;
        }
        return false;
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
    __int64 m_last_offset;
    unsigned char *m_aac_buffer;
    int m_at_eof;

    unsigned long m_samplerate;
    int m_header_type;

    struct seek_list *m_head;
    struct seek_list *m_tail;

    unsigned __int64 m_samples;
    unsigned int m_framesize;
    unsigned __int64 m_samplepos;
    int m_seekskip;
    double m_length;
    bool m_eof;

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

class contextmenu_mp4 : public menu_item_context
{
private:
    int first_num;
    string8 path;

public:
    virtual int get_num_items() { return 1; }

    virtual const char *enum_item(int n)
    {
        if (n == 0) return "Converter (AAC <-> MP4)";
        return 0;
    }

    virtual bool context_get_display(int n, const ptr_list_base<metadb_handle> &data, string_base &out, bool is_playlist)
    {
        int count = data.get_count();
        if (count < 1) return false;
        int type = -1;

        for (int i = 0; i < count; i++)
        {
            metadb_handle *ptr = data.get_item(i);
            if (!ptr) return false;
            const char *p = ptr->handle_get_path();
            if (!p) return false;
            p = strrchr(p, '.');
            if (!p) return false;
            if (type == -1)
            {
                if (!stricmp(p, ".aac")) type = 0;
                else if (!stricmp(p, ".m4a") || !stricmp(p, ".mp4")) type = 1;
                else return false;
            }
            else
            {
                if (type == 0 && stricmp(p, ".aac")) return false;
                if (type == 1 && (stricmp(p, ".m4a") && stricmp(p, ".mp4"))) return false;
            }
        }

        if (type == 0)
        {
            out.set_string("Convert to MP4");
        }
        else
        {
            out.set_string("Extract AAC track");
            if (count > 1) out.add_char('s');
        }

        return true;
    }

    virtual void context_command(int n, const ptr_list_base<metadb_handle> &data, bool is_playlist)
    {
        const int count = data.get_count();

        for (int i = 0; i < count; i++) {
            metadb_handle *ptr = data.get_item(i);
            if (!ptr) return;

            file_info_i_full src_info;

            ptr->handle_lock();
            bool error = false;
            const file_info *info = ptr->handle_query_locked();
            if (info)
                src_info.copy(info);
            else
                error = true;
            ptr->handle_unlock();

            if (!error)
            {
                int type = 1;
                string8 temp = src_info.get_file_path();
                const char *p = strrchr((const char *)temp, '.');
                if (p)
                {
                    if (!stricmp(p, ".aac")) type = 0;
                    temp.truncate(p-(const char *)temp);
                }
                if (type == 0)
                    temp.add_string(".mp4");
                else
                    temp.add_string(".aac");

                const char *src = (const char *)src_info.get_file_path();
                const char *dst = (const char *)temp;

                if (file::g_exists(dst))
                {
                    console::info(string_printf("Destination file '%s' already exists", dst));
                    console::popup();
                }
                else
                {
                    converter *conv = new converter(src, dst, &src_info);

                    if (conv)
                    {
                        bool ret;

                        if (type == 0)
                            ret = conv->aac_to_mp4();
                        else
                            ret = conv->mp4_to_aac();

                        if (ret)
                            console::info(string_printf("'%s' converted to '%s'", src, dst));
                        else
                            console::error(string_printf("Failed to convert '%s' to '%s'", src, dst));

                        delete conv;
                    }
                    else
                    {
                        console::error("Failed to create new converter");
                    }
                }
            }
            else
            {
                console::error("Failed to get file infos");
            }
        }
    }
};

class contextmenu_mp4o : public menu_item_context
{
private:
    int first_num;
    string8 path;

public:
    virtual int get_num_items() { return 1; }

    virtual const char *enum_item(int n)
    {
        if (n == 0) return "MP4 Layout Optimiser";
        return 0;
    }

    virtual bool context_get_display(int n, const ptr_list_base<metadb_handle> &data, string_base &out, bool is_playlist)
    {
        int count = data.get_count();
        if (count < 1) return false;
        int type = -1;

        for (int i = 0; i < count; i++)
        {
            metadb_handle *ptr = data.get_item(i);
            if (!ptr) return false;
            const char *p = ptr->handle_get_path();
            if (!p) return false;
            p = strrchr(p, '.');
            if (!p) return false;
            if (type == -1)
            {
                if (!stricmp(p, ".m4a") || !stricmp(p, ".mp4")) type = 1;
                else return false;
            }
            else
            {
                if (type == 1 && (stricmp(p, ".m4a") && stricmp(p, ".mp4"))) return false;
            }
        }

        out.set_string("Optimise MP4 Layout");

        return true;
    }

    virtual void context_command(int n, const ptr_list_base<metadb_handle> &data, bool is_playlist)
    {
        const int count = data.get_count();

        for (int i = 0; i < count; i++) {
            metadb_handle *ptr = data.get_item(i);
            if (!ptr) return;

            file_info_i_full src_info;

            ptr->handle_lock();
            bool error = false;
            const file_info *info = ptr->handle_query_locked();
            if (info)
                src_info.copy(info);
            else
                error = true;
            ptr->handle_unlock();

            if (!error)
            {
                const char *src = (const char *)src_info.get_file_path();
                if (!src || (src && stricmp_utf8_partial(src, "file://")))
                {
                    console::error("Unsupported file location");
                    return;
                }

                string8 name = src + strlen("file://");
                string8 name_short;

                if (IsUnicode())
                {
                    string_wide_from_utf8 tname(name);

                    int len = wcslen(tname) + 1;
                    WCHAR *wide_fn = (WCHAR *)malloc((len+1) * sizeof(WCHAR));
                    if (!wide_fn) return;

                    int ret = GetShortPathNameW(tname, wide_fn, len+1);
                    if (ret == 0)
                    {
                        wcscpy(wide_fn, tname);
                    }
                    else if (ret > len)
                    {
                        len = ret;
                        free(wide_fn);
                        wide_fn = (WCHAR *)malloc((len+1) * sizeof (WCHAR));
                        if (!wide_fn) return;
                        if (GetShortPathNameW(tname, wide_fn, (len+1)) == 0)
                            wcscpy(wide_fn, tname);
                    }

                    string8 shortname = string_ansi_from_wide(wide_fn);
                    name = shortname;
                    name_short = string_utf8_from_wide(wide_fn);
                    free(wide_fn);
                } else {
                    string_ansi_from_utf8 ansi_fn(name);
                    name = ansi_fn;
                    name_short = string_utf8_from_ansi(ansi_fn);
                }

                MP4Optimize((const char*)name);

                if (IsUnicode())
                {
                    string_wide_from_utf8 short_fn(name_short);
                    string_wide_from_utf8 real_fn(name);
                    MoveFileW(short_fn, real_fn);
                }

                console::info(string_printf("'%s' optimised", src));
            }
            else
            {
                console::error("Failed to get file infos");
            }
        }
    }
};

static service_factory_t<input, input_mp4> foo_mp4;
static service_factory_t<input, input_aac> foo_aac;
static service_factory_single_t<menu_item, contextmenu_mp4> foo_mp4_context;
static service_factory_single_t<menu_item, contextmenu_mp4o> foo_mp4o_context;
