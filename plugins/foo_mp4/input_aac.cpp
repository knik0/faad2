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
** $Id: input_aac.cpp,v 1.2 2003/12/14 13:50:50 menno Exp $
**/

#include "foobar2000/SDK/foobar2000.h"
#include "foobar2000/foo_input_std/id3v2_hacks.h"
#include <faad.h>

//#define DBG_OUT(A) OutputDebugString(A)
#define DBG_OUT(A)

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

static service_factory_t<input, input_aac> foo_aac;


DECLARE_COMPONENT_VERSION ("MPEG-4 AAC decoder",
                           "2.0",
                           "Based on FAAD2 v" FAAD2_VERSION "\nCopyright (C) 2002-2003 http://www.audiocoding.com" );
