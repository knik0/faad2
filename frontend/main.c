/*
** FAAD - Freeware Advanced Audio Decoder
** Copyright (C) 2002 M. Bakker
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
** $Id: main.c,v 1.34 2003/06/24 20:41:41 menno Exp $
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
    __int64 file_offset;
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

    frames_per_sec = (float)samplerate/1024.0;
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
    fprintf(stderr, "\nUsage:\n");
    fprintf(stderr, "%s [options] infile.aac\n", progName);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, " -h    Shows this help screen.\n");
    fprintf(stderr, " -o X  Set output filename.\n");
    fprintf(stderr, " -f X  Set output format. Valid values for X are:\n");
    fprintf(stderr, "        1:  Microsoft WAV format (default).\n");
    fprintf(stderr, "        2:  RAW PCM data.\n");
    fprintf(stderr, " -b X  Set output sample format. Valid values for X are:\n");
    fprintf(stderr, "        1:  16 bit PCM data (default).\n");
    fprintf(stderr, "        2:  24 bit PCM data.\n");
    fprintf(stderr, "        3:  32 bit PCM data.\n");
    fprintf(stderr, "        4:  32 bit floating point data.\n");
    fprintf(stderr, "        5:  64 bit floating point data.\n");
    fprintf(stderr, "        6:  16 bit PCM data (dithered).\n");
    fprintf(stderr, "        7:  16 bit PCM data (dithered with LIGHT noise shaping).\n");
    fprintf(stderr, "        8:  16 bit PCM data (dithered with MEDIUM noise shaping).\n");
    fprintf(stderr, "        9:  16 bit PCM data (dithered with HEAVY noise shaping).\n");
    fprintf(stderr, " -s X  Force the samplerate to X (for RAW files).\n");
    fprintf(stderr, " -l X  Set object type. Supported object types:\n");
    fprintf(stderr, "        0:  Main object type.\n");
    fprintf(stderr, "        1:  LC (Low Complexity) object type.\n");
    fprintf(stderr, "        3:  LTP (Long Term Prediction) object type.\n");
    fprintf(stderr, "        23: LD (Low Delay) object type.\n");
    fprintf(stderr, " -w    Write output to stdio instead of a file.\n");
    fprintf(stderr, "Example:\n");
    fprintf(stderr, "       faad infile.aac\n");
    fprintf(stderr, "       faad infile.mp4\n");
    fprintf(stderr, "       faad -o outfile.wav infile.aac\n");
    fprintf(stderr, "       faad -w infile.aac > outfile.wav\n");
    return;
}

int decodeAACfile(char *aacfile, char *sndfile, int to_stdout,
                  int def_srate, int object_type, int outputFormat, int fileType)
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

    if (bread != 768*6)
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
            length = ((float)length*8.)/((float)bitrate) + 0.5;
        }

        bitrate = (int)((float)bitrate/1000.0 + 0.5);

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
    fprintf(stderr, "AAC file info:\n");
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

        /* open the sound file now that the number of channels are known */
        if (first_time && !frameInfo.error)
        {
            if (!to_stdout)
            {
                aufile = open_audio_file(sndfile, frameInfo.samplerate, frameInfo.channels,
                    outputFormat, fileType);
            } else {
                aufile = open_audio_file("-", frameInfo.samplerate, frameInfo.channels,
                    outputFormat, fileType);
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

        if ((frameInfo.error == 0) && (frameInfo.samples > 0))
        {
            write_audio_file(aufile, sample_buffer, frameInfo.samples);
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
                  int outputFormat, int fileType)
{
    int track;
    unsigned int tracks;
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

    hDecoder = faacDecOpen();

    /* Set configuration */
    config = faacDecGetCurrentConfiguration(hDecoder);
    config->outputFormat = outputFormat;
    faacDecSetConfiguration(hDecoder, config);

    infile = MP4Read(mp4file, 0);
    if (!infile)
    {
        /* unable to open file */
        fprintf(stderr, "Error opening file: %s\n", mp4file);
        return 1;
    }

    /* print some mp4 file info */
    tracks = MP4GetNumberOfTracks(infile, NULL, 0);
    if (tracks > 0)
    {
        char *file_info;
        unsigned int i;

        fprintf(stderr, "MP4 file info:\n");
        for (i = 0; i < tracks; i++)
        {
            file_info = MP4Info(infile, i+1);
            if (file_info)
            {
                fprintf(stderr, "Track: %s", file_info);
                free(file_info);
            }
        }
        fprintf(stderr, "\n");
    }

    if ((track = GetAACTrack(infile)) < 0)
    {
        fprintf(stderr, "Unable to find correct AAC sound track in the MP4 file.\n");
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
    if (buffer) free(buffer);

    numSamples = MP4GetTrackNumberOfSamples(infile, track);

    for (sampleId = 1; sampleId <= numSamples; sampleId++)
    {
        int rc;

        /* get acces unit from MP4 file */
        buffer = NULL;
        buffer_size = 0;

        rc = MP4ReadSample(infile, track, sampleId, &buffer, &buffer_size,
            NULL, NULL, NULL, NULL);
        if (rc == 0)
        {
            fprintf(stderr, "Reading from MP4 file failed.\n");
            faacDecClose(hDecoder);
            MP4Close(infile);
            return 1;
        }

        sample_buffer = faacDecDecode(hDecoder, &frameInfo, buffer, buffer_size);

        if (buffer) free(buffer);

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

        /* open the sound file now that the number of channels are known */
        if (first_time && !frameInfo.error)
        {
            if(!to_stdout)
            {
                aufile = open_audio_file(sndfile, frameInfo.samplerate, frameInfo.channels,
                    outputFormat, fileType);
            } else {
#ifdef _WIN32
                setmode(fileno(stdout), O_BINARY);
#endif
                aufile = open_audio_file("-", frameInfo.samplerate, frameInfo.channels,
                    outputFormat, fileType);
            }
            if (aufile == NULL)
            {
                faacDecClose(hDecoder);
                MP4Close(infile);
                return 0;
            }
            first_time = 0;
        }

        if ((frameInfo.error == 0) && (frameInfo.samples > 0))
        {
            write_audio_file(aufile, sample_buffer, frameInfo.samples);
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
    int writeToStdio = 0;
    int object_type = LC;
    int def_srate = 0;
    int format = 1;
    int outputFormat = FAAD_FMT_16BIT;
    int outfile_set = 0;
    int showHelp = 0;
    int mp4file = 0;
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
            { "stdio",      0, 0, 'w' },
            { "help",       0, 0, 'h' }
        };

        c = getopt_long(argc, argv, "o:s:f:b:l:wh",
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
        case 'w':
            writeToStdio = 1;
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
            outputFormat, format);
    } else {
        result = decodeAACfile(aacFileName, audioFileName, writeToStdio,
            def_srate, object_type, outputFormat, format);
    }


    if (!result)
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
