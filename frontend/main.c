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
** $Id: main.c,v 1.45 2003/09/03 20:19:29 menno Exp $
**/

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <faad.h>
#include <mp4.h>

#include "audio.h"

#ifndef min
#define min(a,b) ( (a) < (b) ? (a) : (b) )
#endif

#define MAX_CHANNELS 6 /* make this higher to support files with
                          more channels */

/* FAAD file buffering routines */
typedef struct {
    long bytes_into_buffer;
    long bytes_consumed;
    long file_offset;
    unsigned char *buffer;
    int at_eof;
    FILE *infile;
} aac_buffer;


int fill_buffer(aac_buffer *b)
{
    int bread;

    if (b->bytes_consumed > 0)
    {
        if (b->bytes_into_buffer)
        {
            memmove((void*)b->buffer, (void*)(b->buffer + b->bytes_consumed),
                b->bytes_into_buffer*sizeof(unsigned char));
        }

        if (!b->at_eof)
        {
            bread = fread((void*)(b->buffer + b->bytes_into_buffer), 1,
                b->bytes_consumed, b->infile);

            if (bread != b->bytes_consumed)
                b->at_eof = 1;

            b->bytes_into_buffer += bread;
        }

        b->bytes_consumed = 0;

        if (b->bytes_into_buffer > 3)
        {
            if (memcmp(b->buffer, "TAG", 3) == 0)
                b->bytes_into_buffer = 0;
        }
        if (b->bytes_into_buffer > 11)
        {
            if (memcmp(b->buffer, "LYRICSBEGIN", 11) == 0)
                b->bytes_into_buffer = 0;
        }
        if (b->bytes_into_buffer > 8)
        {
            if (memcmp(b->buffer, "APETAGEX", 8) == 0)
                b->bytes_into_buffer = 0;
        }
    }

    return 1;
}

void advance_buffer(aac_buffer *b, int bytes)
{
    b->file_offset += bytes;
    b->bytes_consumed = bytes;
    b->bytes_into_buffer -= bytes;
}

int adts_parse(aac_buffer *b, int *bitrate, float *length)
{
    static int sample_rates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000};
    int frames, frame_length;
    int t_framelength = 0;
    int samplerate;
    float frames_per_sec, bytes_per_frame;

    /* Read all frames to ensure correct time and bitrate */
    for (frames = 0; /* */; frames++)
    {
        fill_buffer(b);

        if (b->bytes_into_buffer > 7)
        {
            /* check syncword */
            if (!((b->buffer[0] == 0xFF)&&((b->buffer[1] & 0xF6) == 0xF0)))
                break;

            if (frames == 0)
                samplerate = sample_rates[(b->buffer[2]&0x3c)>>2];

            frame_length = ((((unsigned int)b->buffer[3] & 0x3)) << 11)
                | (((unsigned int)b->buffer[4]) << 3) | (b->buffer[5] >> 5);

            t_framelength += frame_length;

            if (frame_length > b->bytes_into_buffer)
                break;

            advance_buffer(b, frame_length);
        } else {
            break;
        }
    }

    frames_per_sec = (float)samplerate/1024.0f;
    if (frames != 0)
        bytes_per_frame = (float)t_framelength/(float)(frames*1000);
    else
        bytes_per_frame = 0;
    *bitrate = (int)(8. * bytes_per_frame * frames_per_sec + 0.5);
    if (frames_per_sec != 0)
        *length = (float)frames/frames_per_sec;
    else
        *length = 1;

    return 1;
}

/* MicroSoft channel definitions */
#define SPEAKER_FRONT_LEFT             0x1
#define SPEAKER_FRONT_RIGHT            0x2
#define SPEAKER_FRONT_CENTER           0x4
#define SPEAKER_LOW_FREQUENCY          0x8
#define SPEAKER_BACK_LEFT              0x10
#define SPEAKER_BACK_RIGHT             0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER   0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER  0x80
#define SPEAKER_BACK_CENTER            0x100
#define SPEAKER_SIDE_LEFT              0x200
#define SPEAKER_SIDE_RIGHT             0x400
#define SPEAKER_TOP_CENTER             0x800
#define SPEAKER_TOP_FRONT_LEFT         0x1000
#define SPEAKER_TOP_FRONT_CENTER       0x2000
#define SPEAKER_TOP_FRONT_RIGHT        0x4000
#define SPEAKER_TOP_BACK_LEFT          0x8000
#define SPEAKER_TOP_BACK_CENTER        0x10000
#define SPEAKER_TOP_BACK_RIGHT         0x20000
#define SPEAKER_RESERVED               0x80000000

long aacChannelConfig2wavexChannelMask(faacDecFrameInfo *hInfo)
{
    if (hInfo->channels == 6 && hInfo->num_lfe_channels)
    {
        return SPEAKER_FRONT_LEFT + SPEAKER_FRONT_RIGHT +
            SPEAKER_FRONT_CENTER + SPEAKER_LOW_FREQUENCY +
            SPEAKER_BACK_LEFT + SPEAKER_BACK_RIGHT;
    } else {
        return 0;
    }
}

char *position2string(int position)
{
    switch (position)
    {
    case FRONT_CHANNEL_CENTER: return "Center front";
    case FRONT_CHANNEL_LEFT:   return "Left front";
    case FRONT_CHANNEL_RIGHT:  return "Right front";
    case SIDE_CHANNEL_LEFT:    return "Left side";
    case SIDE_CHANNEL_RIGHT:   return "Right side";
    case BACK_CHANNEL_LEFT:    return "Left back";
    case BACK_CHANNEL_RIGHT:   return "Right back";
    case BACK_CHANNEL_CENTER:  return "Center back";
    case LFE_CHANNEL:          return "LFE";
    case UNKNOWN_CHANNEL:      return "Unknown";
    default: return "";
    }

    return "";
}

void print_channel_info(faacDecFrameInfo *frameInfo)
{
    /* print some channel info */
    int i;
    long channelMask = aacChannelConfig2wavexChannelMask(frameInfo);

    printf("  ---------------------\n");
    if (frameInfo->num_lfe_channels > 0)
    {
        printf(" | Config: %2d.%d Ch     |", frameInfo->channels-frameInfo->num_lfe_channels, frameInfo->num_lfe_channels);
    } else {
        printf(" | Config: %2d Ch       |", frameInfo->channels);
    }
    if (channelMask)
        printf(" WARNING: channels are reordered according to\n");
    else
        printf("\n");
    printf("  ---------------------");
    if (channelMask)
        printf("  MS defaults defined in WAVE_FORMAT_EXTENSIBLE\n");
    else
        printf("\n");
    printf(" | Ch |    Position    |\n");
    printf("  ---------------------\n");
    for (i = 0; i < frameInfo->channels; i++)
    {
        printf(" | %.2d | %-14s |\n", i, position2string((int)frameInfo->channel_position[i]));
    }
    printf("  ---------------------\n");
    printf("\n");
}


/* globals */
char *progName;

char *file_ext[] =
{
    NULL,
    ".wav",
    ".aif",
    ".au",
    ".au",
    ".pcm",
    NULL
};

void usage(void)
{
    fprintf(stdout, "\nUsage:\n");
    fprintf(stdout, "%s [options] infile.aac\n", progName);
    fprintf(stdout, "Options:\n");
    fprintf(stdout, " -h    Shows this help screen.\n");
    fprintf(stdout, " -i    Shows info about the input file.\n");
    fprintf(stdout, " -o X  Set output filename.\n");
    fprintf(stdout, " -f X  Set output format. Valid values for X are:\n");
    fprintf(stdout, "        1:  Microsoft WAV format (default).\n");
    fprintf(stdout, "        2:  RAW PCM data.\n");
    fprintf(stdout, " -b X  Set output sample format. Valid values for X are:\n");
    fprintf(stdout, "        1:  16 bit PCM data (default).\n");
    fprintf(stdout, "        2:  24 bit PCM data.\n");
    fprintf(stdout, "        3:  32 bit PCM data.\n");
    fprintf(stdout, "        4:  32 bit floating point data.\n");
    fprintf(stdout, "        5:  64 bit floating point data.\n");
    fprintf(stdout, "        6:  16 bit PCM data (dithered).\n");
    fprintf(stdout, "        7:  16 bit PCM data (dithered with LIGHT noise shaping).\n");
    fprintf(stdout, "        8:  16 bit PCM data (dithered with MEDIUM noise shaping).\n");
    fprintf(stdout, "        9:  16 bit PCM data (dithered with HEAVY noise shaping).\n");
    fprintf(stdout, " -s X  Force the samplerate to X (for RAW files).\n");
    fprintf(stdout, " -l X  Set object type. Supported object types:\n");
    fprintf(stdout, "        0:  Main object type.\n");
    fprintf(stdout, "        1:  LC (Low Complexity) object type.\n");
    fprintf(stdout, "        3:  LTP (Long Term Prediction) object type.\n");
    fprintf(stdout, "        23: LD (Low Delay) object type.\n");
    fprintf(stdout, " -d    Down matrix 5.1 to 2 channels\n");
    fprintf(stdout, " -w    Write output to stdio instead of a file.\n");
    fprintf(stdout, " -g    Disable gapless decoding.\n");
    fprintf(stdout, "Example:\n");
    fprintf(stdout, "       faad infile.aac\n");
    fprintf(stdout, "       faad infile.mp4\n");
    fprintf(stdout, "       faad -o outfile.wav infile.aac\n");
    fprintf(stdout, "       faad -w infile.aac > outfile.wav\n");
    return;
}

int decodeAACfile(char *aacfile, char *sndfile, int to_stdout,
                  int def_srate, int object_type, int outputFormat, int fileType,
                  int downMatrix, int infoOnly)
{
    int tagsize;
    unsigned long samplerate;
    unsigned char channels;
    void *sample_buffer;

    audio_file *aufile;

    faacDecHandle hDecoder;
    faacDecFrameInfo frameInfo;
    faacDecConfigurationPtr config;

    char percents[200];
    int percent, old_percent = -1;
    int bread, fileread;
    int header_type = 0;
    int bitrate = 0;
    float length = 0;

    int first_time = 1;

    aac_buffer b;

    memset(&b, 0, sizeof(aac_buffer));

    b.infile = fopen(aacfile, "rb");
    if (b.infile == NULL)
    {
        /* unable to open file */
        fprintf(stderr, "Error opening file: %s\n", aacfile);
        return 1;
    }

    fseek(b.infile, 0, SEEK_END);
    fileread = ftell(b.infile);
    fseek(b.infile, 0, SEEK_SET);

    if (!(b.buffer = (unsigned char*)malloc(FAAD_MIN_STREAMSIZE*MAX_CHANNELS)))
    {
        fprintf(stderr, "Memory allocation error\n");
        return 0;
    }
    memset(b.buffer, 0, FAAD_MIN_STREAMSIZE*MAX_CHANNELS);

    bread = fread(b.buffer, 1, FAAD_MIN_STREAMSIZE*MAX_CHANNELS, b.infile);
    b.bytes_into_buffer = bread;
    b.bytes_consumed = 0;
    b.file_offset = 0;

    if (bread != FAAD_MIN_STREAMSIZE*MAX_CHANNELS)
        b.at_eof = 1;

    tagsize = 0;
    if (!memcmp(b.buffer, "ID3", 3))
    {
        /* high bit is not used */
        tagsize = (b.buffer[6] << 21) | (b.buffer[7] << 14) |
            (b.buffer[8] <<  7) | (b.buffer[9] <<  0);

        tagsize += 10;
        advance_buffer(&b, tagsize);
        fill_buffer(&b);
    }

    hDecoder = faacDecOpen();

    /* Set the default object type and samplerate */
    /* This is useful for RAW AAC files */
    config = faacDecGetCurrentConfiguration(hDecoder);
    if (def_srate)
        config->defSampleRate = def_srate;
    config->defObjectType = object_type;
    config->outputFormat = outputFormat;
    config->downMatrix = downMatrix;
    faacDecSetConfiguration(hDecoder, config);

    /* get AAC infos for printing */
    header_type = 0;
    if ((b.buffer[0] == 0xFF) && ((b.buffer[1] & 0xF6) == 0xF0))
    {
        adts_parse(&b, &bitrate, &length);
        fseek(b.infile, tagsize, SEEK_SET);

        bread = fread(b.buffer, 1, FAAD_MIN_STREAMSIZE*MAX_CHANNELS, b.infile);
        if (bread != FAAD_MIN_STREAMSIZE*MAX_CHANNELS)
            b.at_eof = 1;
        else
            b.at_eof = 0;
        b.bytes_into_buffer = bread;
        b.bytes_consumed = 0;
        b.file_offset = tagsize;

        header_type = 1;
    } else if (memcmp(b.buffer, "ADIF", 4) == 0) {
        int skip_size = (b.buffer[4] & 0x80) ? 9 : 0;
        bitrate = ((unsigned int)(b.buffer[4 + skip_size] & 0x0F)<<19) |
            ((unsigned int)b.buffer[5 + skip_size]<<11) |
            ((unsigned int)b.buffer[6 + skip_size]<<3) |
            ((unsigned int)b.buffer[7 + skip_size] & 0xE0);

        length = (float)fileread;
        if (length != 0)
        {
            length = ((float)length*8.f)/((float)bitrate) + 0.5f;
        }

        bitrate = (int)((float)bitrate/1000.0f + 0.5f);

        header_type = 2;
    }

    fill_buffer(&b);
    if ((bread = faacDecInit(hDecoder, b.buffer,
        b.bytes_into_buffer, &samplerate, &channels)) < 0)
    {
        /* If some error initializing occured, skip the file */
        fprintf(stderr, "Error initializing decoder library.\n");
        if (b.buffer)
            free(b.buffer);
        faacDecClose(hDecoder);
        fclose(b.infile);
        return 1;
    }
    advance_buffer(&b, bread);
    fill_buffer(&b);

    /* print AAC file info */
    fprintf(stderr, "%s file info:\n", aacfile);
    switch (header_type)
    {
    case 0:
        fprintf(stderr, "RAW\n\n");
        break;
    case 1:
        fprintf(stderr, "ADTS, %.3f sec, %d kbps, %d Hz\n\n",
            length, bitrate, samplerate);
        break;
    case 2:
        fprintf(stderr, "ADIF, %.3f sec, %d kbps, %d Hz\n\n",
            length, bitrate, samplerate);
        break;
    }

    if (infoOnly)
    {
        faacDecClose(hDecoder);
        fclose(b.infile);
        if (b.buffer)
            free(b.buffer);
        return 0;
    }

    do
    {
        sample_buffer = faacDecDecode(hDecoder, &frameInfo,
            b.buffer, b.bytes_into_buffer);

        /* update buffer indices */
        advance_buffer(&b, frameInfo.bytesconsumed);

        if (frameInfo.error > 0)
        {
            fprintf(stderr, "Error: %s\n",
                faacDecGetErrorMessage(frameInfo.error));
        }

        /* open the sound file now that the number of channels are known */
        if (first_time && !frameInfo.error)
        {
            /* print some channel info */
            print_channel_info(&frameInfo);

            /* open output file */
            if (!to_stdout)
            {
                aufile = open_audio_file(sndfile, frameInfo.samplerate, frameInfo.channels,
                    outputFormat, fileType, aacChannelConfig2wavexChannelMask(&frameInfo));
            } else {
                aufile = open_audio_file("-", frameInfo.samplerate, frameInfo.channels,
                    outputFormat, fileType, aacChannelConfig2wavexChannelMask(&frameInfo));
            }
            if (aufile == NULL)
            {
                if (b.buffer)
                    free(b.buffer);
                faacDecClose(hDecoder);
                fclose(b.infile);
                return 0;
            }
            first_time = 0;
        }

        percent = min((int)(b.file_offset*100)/fileread, 100);
        if (percent > old_percent)
        {
            old_percent = percent;
            sprintf(percents, "%d%% decoding %s.", percent, aacfile);
            fprintf(stderr, "%s\r", percents);
#ifdef _WIN32
            SetConsoleTitle(percents);
#endif
        }

        if ((frameInfo.error == 0) && (frameInfo.samples > 0))
        {
            write_audio_file(aufile, sample_buffer, frameInfo.samples, 0);
        }

        /* fill buffer */
        fill_buffer(&b);

        if (b.bytes_into_buffer == 0)
            sample_buffer = NULL; /* to make sure it stops now */

    } while (sample_buffer != NULL);


    faacDecClose(hDecoder);

    fclose(b.infile);

    if (!first_time)
        close_audio_file(aufile);

    if (b.buffer)
        free(b.buffer);

    return frameInfo.error;
}

int GetAACTrack(MP4FileHandle infile)
{
    /* find AAC track */
    unsigned short i;
    int rc;
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
            MP4GetTrackESConfiguration(infile, trackId, &buff, &buff_size);

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

unsigned long srates[] =
{
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000,
    12000, 11025, 8000
};

int decodeMP4file(char *mp4file, char *sndfile, int to_stdout,
                  int outputFormat, int fileType, int downMatrix, int noGapless, int infoOnly)
{
    int track;
    unsigned long samplerate;
    unsigned char channels;
    void *sample_buffer;

    MP4FileHandle infile;
    MP4SampleId sampleId, numSamples;

    audio_file *aufile;

    faacDecHandle hDecoder;
    faacDecConfigurationPtr config;
    faacDecFrameInfo frameInfo;

    unsigned char *buffer;
    int buffer_size;

    char percents[200];
    int percent, old_percent = -1;

    int first_time = 1;

    /* for gapless decoding */
    unsigned int useAacLength = 1;
    unsigned int framesize;
    unsigned int initial = 1;
    unsigned long timescale;

    hDecoder = faacDecOpen();

    /* Set configuration */
    config = faacDecGetCurrentConfiguration(hDecoder);
    config->outputFormat = outputFormat;
    config->downMatrix = downMatrix;
    faacDecSetConfiguration(hDecoder, config);

    infile = MP4Read(mp4file, 0);
    if (!infile)
    {
        /* unable to open file */
        fprintf(stderr, "Error opening file: %s\n", mp4file);
        return 1;
    }

    /* print some mp4 file info */
    fprintf(stderr, "%s file info:\n", mp4file);
    {
        char *file_info = MP4Info(infile, MP4_INVALID_TRACK_ID);
        fprintf(stderr, "%s\n", file_info);
        free(file_info);
    }

    if (infoOnly)
    {
        faacDecClose(hDecoder);
        MP4Close(infile);
        return 0;
    }

    if ((track = GetAACTrack(infile)) < 0)
    {
        fprintf(stderr, "Unable to find correct AAC sound track in the MP4 file.\n");
        faacDecClose(hDecoder);
        MP4Close(infile);
        return 1;
    }

    buffer = NULL;
    buffer_size = 0;
    MP4GetTrackESConfiguration(infile, track, &buffer, &buffer_size);

    if(faacDecInit2(hDecoder, buffer, buffer_size,
                    &samplerate, &channels) < 0)
    {
        /* If some error initializing occured, skip the file */
        fprintf(stderr, "Error initializing decoder library.\n");
        faacDecClose(hDecoder);
        MP4Close(infile);
        return 1;
    }

    if (!noGapless)
    {
        mp4AudioSpecificConfig mp4ASC;

        timescale = MP4GetTrackTimeScale(infile, track);
        framesize = 1024;
        useAacLength = 0;

        if (buffer)
        {
            if (AudioSpecificConfig(buffer, buffer_size, &mp4ASC) >= 0)
            {
                if (mp4ASC.frameLengthFlag == 1) framesize = 960;
                if (mp4ASC.sbr_present_flag == 1) framesize *= 2;
            }
        }
    }

    if (buffer) free(buffer);

    numSamples = MP4GetTrackNumberOfSamples(infile, track);

    for (sampleId = 1; sampleId <= numSamples; sampleId++)
    {
        int rc;
        MP4Duration dur;
        unsigned int sample_count;
        unsigned int delay = 0;

        /* get acces unit from MP4 file */
        buffer = NULL;
        buffer_size = 0;

        rc = MP4ReadSample(infile, track, sampleId, &buffer, &buffer_size,
            NULL, &dur, NULL, NULL);
        if (rc == 0)
        {
            fprintf(stderr, "Reading from MP4 file failed.\n");
            faacDecClose(hDecoder);
            MP4Close(infile);
            return 1;
        }

        sample_buffer = faacDecDecode(hDecoder, &frameInfo, buffer, buffer_size);

        if (buffer) free(buffer);

        if (!noGapless)
        {
            if (useAacLength || (timescale != samplerate)) {
                sample_count = frameInfo.samples;
            } else {
                sample_count = (unsigned int)(dur * frameInfo.channels);

                if (!useAacLength && !initial && (sampleId < numSamples/2) && (sample_count != frameInfo.samples))
                {
                    fprintf(stderr, "MP4 seems to have incorrect frame duration, using values from AAC data.\n");
                    useAacLength = 1;
                }
            }

            if (initial && (sample_count < framesize*frameInfo.channels))
                delay = frameInfo.samples - sample_count;
        }
        else
        {
            sample_count = frameInfo.samples;
        }

        /* open the sound file now that the number of channels are known */
        if (first_time && !frameInfo.error)
        {
            /* print some channel info */
            print_channel_info(&frameInfo);

            /* open output file */
            if(!to_stdout)
            {
                aufile = open_audio_file(sndfile, frameInfo.samplerate, frameInfo.channels,
                    outputFormat, fileType, aacChannelConfig2wavexChannelMask(&frameInfo));
            } else {
#ifdef _WIN32
                setmode(fileno(stdout), O_BINARY);
#endif
                aufile = open_audio_file("-", frameInfo.samplerate, frameInfo.channels,
                    outputFormat, fileType, aacChannelConfig2wavexChannelMask(&frameInfo));
            }
            if (aufile == NULL)
            {
                faacDecClose(hDecoder);
                MP4Close(infile);
                return 0;
            }
            first_time = 0;
        }

        if (sample_count > 0) initial = 0;

        percent = min((int)(sampleId*100)/numSamples, 100);
        if (percent > old_percent)
        {
            old_percent = percent;
            sprintf(percents, "%d%% decoding %s.", percent, mp4file);
            fprintf(stderr, "%s\r", percents);
#ifdef _WIN32
            SetConsoleTitle(percents);
#endif
        }

        if ((frameInfo.error == 0) && (sample_count > 0))
        {
            write_audio_file(aufile, sample_buffer, sample_count, delay);
        }

        if (frameInfo.error > 0)
        {
            fprintf(stderr, "Warning: %s\n",
                faacDecGetErrorMessage(frameInfo.error));
        }
    }


    faacDecClose(hDecoder);

    MP4Close(infile);

    if (!first_time)
        close_audio_file(aufile);

    return frameInfo.error;
}

int main(int argc, char *argv[])
{
    int result;
    int infoOnly = 0;
    int writeToStdio = 0;
    int object_type = LC;
    int def_srate = 0;
    int downMatrix = 0;
    int format = 1;
    int outputFormat = FAAD_FMT_16BIT;
    int outfile_set = 0;
    int showHelp = 0;
    int mp4file = 0;
    int noGapless = 0;
    char *fnp;
    char aacFileName[255];
    char audioFileName[255];
    MP4FileHandle infile;

/* System dependant types */
#ifdef _WIN32
    long begin, end;
#else
    clock_t begin;
#endif

    unsigned long cap = faacDecGetCapabilities();

    fprintf(stderr, " ****** FAAD2 (Freeware AAC Decoder) V%s ******\n\n", FAAD2_VERSION);
    fprintf(stderr, "        Build: %s\n", __DATE__);
    fprintf(stderr, "        Copyright: M. Bakker\n");
    fprintf(stderr, "                   Ahead Software AG\n");
    fprintf(stderr, "        http://www.audiocoding.com\n");
    if (cap & FIXED_POINT_CAP)
        fprintf(stderr, "        Fixed point version\n");
    else
        fprintf(stderr, "        Floating point version\n");
    fprintf(stderr, "\n");
    fprintf(stderr, " ****************************************************\n\n");

    /* begin process command line */
    progName = argv[0];
    while (1) {
        int c = -1;
        int option_index = 0;
        static struct option long_options[] = {
            { "outfile",    0, 0, 'o' },
            { "format",     0, 0, 'f' },
            { "bits",       0, 0, 'b' },
            { "samplerate", 0, 0, 's' },
            { "objecttype", 0, 0, 'l' },
            { "downmix",    0, 0, 'd' },
            { "info",       0, 0, 'i' },
            { "stdio",      0, 0, 'w' },
            { "stdio",      0, 0, 'g' },
            { "help",       0, 0, 'h' }
        };

        c = getopt_long(argc, argv, "o:s:f:b:l:wgdhi",
            long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 'o':
            if (optarg) {
                outfile_set = 1;
                strcpy(audioFileName, optarg);
            }
            break;
        case 's':
            if (optarg) {
                char dr[10];
                if (sscanf(optarg, "%s", dr) < 1) {
                    def_srate = 0;
                } else {
                    def_srate = atoi(dr);
                }
            }
            break;
        case 'f':
            if (optarg) {
                char dr[10];
                if (sscanf(optarg, "%s", dr) < 1) {
                    format = 1;
                } else {
                    format = atoi(dr);
                    if ((format < 1) || (format > 2))
                        showHelp = 1;
                }
            }
            break;
        case 'b':
            if (optarg) {
                char dr[10];
                if (sscanf(optarg, "%s", dr) < 1) {
                    outputFormat = FAAD_FMT_16BIT; /* just use default */
                } else {
                    outputFormat = atoi(dr);
                    if ((outputFormat < 1) || (outputFormat > 9))
                        showHelp = 1;
                }
            }
            break;
        case 'l':
            if (optarg) {
                char dr[10];
                if (sscanf(optarg, "%s", dr) < 1) {
                    object_type = LC; /* default */
                } else {
                    object_type = atoi(dr);
                    if ((object_type != LC) &&
                        (object_type != MAIN) &&
                        (object_type != LTP) &&
                        (object_type != LD))
                    {
                        showHelp = 1;
                    }
                }
            }
            break;
        case 'd':
            downMatrix = 1;
            break;
        case 'w':
            writeToStdio = 1;
            break;
        case 'g':
            noGapless = 1;
            break;
        case 'i':
            infoOnly = 1;
            break;
        case 'h':
            showHelp = 1;
            break;
        default:
            break;
        }
    }

    /* check that we have at least two non-option arguments */
    /* Print help if requested */
    if (((argc - optind) < 1) || showHelp)
    {
        usage();
        return 1;
    }

    /* point to the specified file name */
    strcpy(aacFileName, argv[optind]);

#ifdef _WIN32
    begin = GetTickCount();
#else
    begin = clock();
#endif

    /* Only calculate the path and open the file for writing if
       we are not writing to stdout.
     */
    if(!writeToStdio && !outfile_set)
    {
        strcpy(audioFileName, aacFileName);

        fnp = (char *)strrchr(audioFileName,'.');

        if (fnp)
            fnp[0] = '\0';

        strcat(audioFileName, file_ext[format]);
    }

    mp4file = 1;
    infile = MP4Read(aacFileName, 0);
    if (!infile)
        mp4file = 0;
    if (infile) MP4Close(infile);

    if (mp4file)
    {
        result = decodeMP4file(aacFileName, audioFileName, writeToStdio,
            outputFormat, format, downMatrix, noGapless, infoOnly);
    } else {
        result = decodeAACfile(aacFileName, audioFileName, writeToStdio,
            def_srate, object_type, outputFormat, format, downMatrix, infoOnly);
    }


    if (!result && !infoOnly)
    {
#ifdef _WIN32
        end = GetTickCount();
        fprintf(stderr, "Decoding %s took: %d sec.\n", aacFileName,
            (end-begin)/1000);
        SetConsoleTitle("FAAD");
#else
        /* clock() grabs time since the start of the app but when we decode
           multiple files, each file has its own starttime (begin).
         */
        fprintf(stderr, "Decoding %s took: %5.2f sec.\n", aacFileName,
            (float)(clock() - begin)/(float)CLOCKS_PER_SEC);
#endif
    }

    return 0;
}
