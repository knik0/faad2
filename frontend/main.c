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
** $Id: main.c,v 1.5 2002/01/15 13:51:42 menno Exp $
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
#include <sndfile.h>
#include <mp4.h>

#include "audio.h"

#ifndef min
#define min(a,b) ( (a) < (b) ? (a) : (b) )
#endif

#define MAX_CHANNELS 6 /* make this higher to support files with
                          more channels */

/* FAAD file buffering routines */
/* declare buffering variables */
#define DEC_BUFF_VARS \
    int fileread, bytesconsumed, k; \
    int buffercount = 0, buffer_index = 0; \
    unsigned char *buffer;

/* initialise buffering */
#define INIT_BUFF(file) \
    fseek(file, 0, SEEK_END); \
    fileread = ftell(file); \
    fseek(file, 0, SEEK_SET); \
    buffer = (unsigned char*)malloc(FAAD_MIN_STREAMSIZE*MAX_CHANNELS); \
    memset(buffer, 0, FAAD_MIN_STREAMSIZE*MAX_CHANNELS); \
    fread(buffer, 1, FAAD_MIN_STREAMSIZE*MAX_CHANNELS, file);

/* skip bytes in buffer */
#define UPDATE_BUFF_SKIP(bytes) \
    fseek(infile, bytes, SEEK_SET); \
    buffer_index += bytes; \
    buffercount = 0; \
    fread(buffer, 1, FAAD_MIN_STREAMSIZE*MAX_CHANNELS, infile);

/* update buffer */
#define UPDATE_BUFF_READ \
    if (bytesconsumed > 0) { \
        for (k = 0; k < (FAAD_MIN_STREAMSIZE*MAX_CHANNELS - bytesconsumed); k++) \
            buffer[k] = buffer[k + bytesconsumed]; \
        fread(buffer + (FAAD_MIN_STREAMSIZE*MAX_CHANNELS) - bytesconsumed, 1, bytesconsumed, infile); \
        bytesconsumed = 0; \
    }

/* update buffer indices after faacDecDecode */
#define UPDATE_BUFF_IDX(frame) \
    bytesconsumed += frame.bytesconsumed; \
    buffer_index += frame.bytesconsumed;

/* true if decoding has to stop because of EOF */
#define IS_FILE_END buffer_index >= fileread

/* end buffering */
#define END_BUFF if (buffer) free(buffer);



/* globals */
char *progName;

int id3v2_tag(unsigned char *buffer)
{
    if (strncmp(buffer, "ID3", 3) == 0) {
        unsigned long tagsize;

        /* high bit is not used */
        tagsize = (buffer[6] << 21) | (buffer[7] << 14) |
            (buffer[8] <<  7) | (buffer[9] <<  0);

        tagsize += 10;

        return tagsize;
    } else {
        return 0;
    }
}

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
    fprintf(stderr, "        2:  Apple/SGI AIFF format.\n");
    fprintf(stderr, "        3:  Sun/NeXT AU format.\n");
    fprintf(stderr, "        4:  DEC AU format.\n");
    fprintf(stderr, "        5:  RAW PCM data.\n");
    fprintf(stderr, " -b X  Set output sample format. Valid values for X are:\n");
    fprintf(stderr, "        1:  16 bit PCM data (default).\n");
    fprintf(stderr, "        2:  24 bit PCM data.\n");
    fprintf(stderr, "        3:  32 bit PCM data.\n");
    fprintf(stderr, "        4:  32 bit floats.\n");
    fprintf(stderr, " -s X  Force the samplerate to X (for RAW files).\n");
    fprintf(stderr, " -l    Use Long Term Prediction (for RAW files).\n");
    fprintf(stderr, " -w    Write output to stdio instead of a file.\n");
    fprintf(stderr, "Example:\n");
    fprintf(stderr, "       faad infile.aac\n");
    fprintf(stderr, "       faad infile.mp4\n");
    fprintf(stderr, "       faad -o outfile.wav infile.aac\n");
    fprintf(stderr, "       faad -w infile.aac > outfile.wav\n");
    return;
}


int decodeAACfile(char *aacfile, char *sndfile, int to_stdout,
                  int def_srate, int use_ltp, int outputFormat, int fileType)
{
    int tagsize;
    unsigned long samplerate, channels;
    void *sample_buffer;

    FILE *infile;

    audio_file *aufile;

    faacDecHandle hDecoder;
    faacDecFrameInfo frameInfo;
    faacDecConfigurationPtr config;

    char percents[200];
    int percent, old_percent = -1;

    int first_time = 1;

    /* declare variables for buffering */
    DEC_BUFF_VARS


    infile = fopen(aacfile, "rb");
    if (infile == NULL)
    {
        /* unable to open file */
        fprintf(stderr, "Error opening file: %s\n", aacfile);
        return 1;
    }
    INIT_BUFF(infile)

    tagsize = id3v2_tag(buffer);
    if (tagsize) {
        UPDATE_BUFF_SKIP(tagsize)
    }

    hDecoder = faacDecOpen();

    /* Set the default object type and samplerate */
    /* This is useful for RAW AAC files */
    config = faacDecGetCurrentConfiguration(hDecoder);
    if (def_srate)
        config->defSampleRate = def_srate;
    if (use_ltp)
        config->defObjectType = LTP;
    config->outputFormat = outputFormat;

    faacDecSetConfiguration(hDecoder, config);

    if((bytesconsumed = faacDecInit(hDecoder, buffer, &samplerate,
        &channels)) < 0)
    {
        /* If some error initializing occured, skip the file */
        fprintf(stderr, "Error initializing decoder library.\n");
        END_BUFF
        faacDecClose(hDecoder);
        fclose(infile);
        return 1;
    }

    /* update buffer */
    UPDATE_BUFF_READ

    do
    {
        /* update buffer */
        UPDATE_BUFF_READ

        sample_buffer = faacDecDecode(hDecoder, &frameInfo, buffer);

        /* update buffer indices */
        UPDATE_BUFF_IDX(frameInfo)

        if (frameInfo.error > 0)
        {
            fprintf(stderr, "Error: %s\n",
                faacDecGetErrorMessage(frameInfo.error));
        }

        percent = min((int)(buffer_index*100)/fileread, 100);
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
            if(!to_stdout)
            {
                aufile = open_audio_file(sndfile, samplerate, frameInfo.channels,
                    outputFormat, fileType);
            } else {
                setmode(fileno(stdout), O_BINARY);
                aufile = open_audio_file("-", samplerate, frameInfo.channels,
                    outputFormat, fileType);
            }
            if (aufile == NULL)
            {
                END_BUFF
                faacDecClose(hDecoder);
                fclose(infile);
                return 0;
            }
            first_time = 0;
        }

        if ((frameInfo.error == 0) && (frameInfo.samples > 0))
        {
            write_audio_file(aufile, sample_buffer, frameInfo.samples);
        }

        if (IS_FILE_END)
            sample_buffer = NULL; /* to make sure it stops now */

    } while (sample_buffer != NULL);


    faacDecClose(hDecoder);

    fclose(infile);

    close_audio_file(aufile);

    END_BUFF

    return frameInfo.error;
}

int use_ltp;

int GetAACTrack(MP4FileHandle *infile)
{
    /* find AAC track */
    int i;
	int numTracks = MP4GetNumberOfTracks(infile, NULL);

	for (i = 0; i < numTracks; i++)
    {
        MP4TrackId trackId = MP4FindTrackId(infile, i, NULL);
        const char* trackType = MP4GetTrackType(infile, trackId);

        if (!strcmp(trackType, MP4_AUDIO_TRACK_TYPE))
        {
            int type = MP4GetTrackAudioType(infile, trackId);

            switch (type)
            {
            case MP4_MPEG4_AUDIO_TYPE:
                /* could be LTP */
                use_ltp = 1;

                return trackId;
            case MP4_MPEG2_AAC_MAIN_AUDIO_TYPE:
            case MP4_MPEG2_AAC_LC_AUDIO_TYPE:
            case MP4_MPEG2_AAC_SSR_AUDIO_TYPE:
                /* MPEG2: no LTP */
                use_ltp = 0;

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
    int track, srate;
    unsigned long samplerate, channels;
    void *sample_buffer;

    MP4FileHandle *infile;
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

	infile = MP4Read(mp4file, 0);
	if (!infile)
    {
        /* unable to open file */
        fprintf(stderr, "Error opening file: %s\n", mp4file);
        return 1;
	}

    use_ltp = 0;

    if ((track = GetAACTrack(infile)) < 0)
    {
        fprintf(stderr, "Unable to find correct AAC sound track in the MP4 file.\n");
        MP4Close(infile);
        return 1;
    }
    srate = MP4GetTrackTimeScale(infile, track);

    hDecoder = faacDecOpen();

    /* Set the default object type and samplerate */
    /* This is useful for RAW AAC files */
    config = faacDecGetCurrentConfiguration(hDecoder);
    config->defSampleRate = srates[srate];
    if (use_ltp)
        config->defObjectType = LTP;
    config->outputFormat = outputFormat;

    faacDecSetConfiguration(hDecoder, config);


    if(faacDecInit(hDecoder, NULL, &samplerate,
        &channels) < 0)
    {
        /* If some error initializing occured, skip the file */
        fprintf(stderr, "Error initializing decoder library.\n");
        faacDecClose(hDecoder);
        MP4Close(infile);
        return 1;
    }

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

        sample_buffer = faacDecDecode(hDecoder, &frameInfo, buffer);

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
                aufile = open_audio_file(sndfile, srates[srate], frameInfo.channels,
                    outputFormat, fileType);
            } else {
                setmode(fileno(stdout), O_BINARY);
                aufile = open_audio_file("-", srates[srate], frameInfo.channels,
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
            fprintf(stderr, "Error: %s\n",
                faacDecGetErrorMessage(frameInfo.error));
            break;
        }
    }


    faacDecClose(hDecoder);

    MP4Close(infile);

    close_audio_file(aufile);

    return frameInfo.error;
}

int main(int argc, char *argv[])
{
    int result;
    int writeToStdio = 0;
    int use_ltp = 0;
    int def_srate = 0;
    int format = 1;
    int outputFormat = FAAD_FMT_16BIT;
    int outfile_set = 0;
    int showHelp = 0;
    int mp4file = 0;
    char *fnp;
    char aacFileName[255];
    char audioFileName[255];

    FILE *infile;

/* System dependant types */
#ifdef _WIN32
    long begin, end;
#else
    clock_t begin;
#endif

    fprintf(stderr, "FAAD (Freeware AAC Decoder) Compiled on: " __DATE__ "\n");
    fprintf(stderr, "Copyright:   M. Bakker\n");
    fprintf(stderr, "             http://www.audiocoding.com\n\n");

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
            { "ltp",        0, 0, 'l' },
            { "stdio",      0, 0, 'w' },
            { "help",       0, 0, 'h' }
        };

        c = getopt_long(argc, argv, "o:s:f:b:lwh",
            long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 'o':
            if (optarg) {
                char dr[255];
                if (sscanf(optarg, "%s", dr) < 1) {
                    outfile_set = 0;
                } else {
                    outfile_set = 1;
                    strcpy(audioFileName, dr);
                }
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
                    if ((format < 1) || (format > 5))
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
                    if ((outputFormat < 1) || (outputFormat > 4))
                        showHelp = 1;
                }
            }
            break;
        case 'l':
            use_ltp = 1;
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

    fnp = (char *)strrchr(aacFileName, '.');
    if (!stricmp(fnp, ".MP4"))
        mp4file = 1;

    if (mp4file)
    {
        result = decodeMP4file(aacFileName, audioFileName, writeToStdio,
            outputFormat, format);
    } else {
        result = decodeAACfile(aacFileName, audioFileName, writeToStdio,
            def_srate, use_ltp, outputFormat, format);
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
