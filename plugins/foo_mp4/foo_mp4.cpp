
#include <mp4.h>
#include <faad.h>
#include "pfc/pfc.h"
#include "foobar2000/SDK/input.h"

class input_mp4 : public input
{
public:

    virtual int test_filename(const WCHAR * fn,const WCHAR * ext) 
    {
        return !wcsicmp(ext,L"MP4");
    }

    virtual int open(reader * r,file_info * info,int full_open)
    {
        unsigned __int8 *buffer;
        unsigned __int32 buffer_size;
        unsigned __int8 channels;
        faacDecConfigurationPtr config;

        hDecoder = faacDecOpen();
        if (!hDecoder) return 0;

        config = faacDecGetCurrentConfiguration(hDecoder);
        config->outputFormat = FAAD_FMT_FLOAT;
        faacDecSetConfiguration(hDecoder, config);

        char filename[_MAX_PATH];
        int len = (wcslen(info->get_file_path())+1)*2;
        WideCharToMultiByte(CP_ACP,0,info->get_file_path(),-1,filename,len,0,0);

        hFile = MP4Read(filename, 0);
        if (hFile == MP4_INVALID_FILE_HANDLE) return 0;

        track = GetAACTrack(hFile);
        if (track < 1) return 0;

        buffer = NULL;
        buffer_size = 0;
        MP4GetTrackESConfiguration(hFile, track, &buffer, &buffer_size);
        if (!buffer) return 0;

        int rc = faacDecInit2(hDecoder, (unsigned char*)buffer, buffer_size,
            (unsigned long*)&m_samplerate, (unsigned char*)&channels);
        if (buffer) free(buffer);
        if (rc < 0) return 0;

        numSamples = MP4GetTrackNumberOfSamples(hFile, track);
        sampleId = 1;

        unsigned __int64 length = MP4GetTrackDuration(hFile, track);
        __int64 sDuration = MP4ConvertFromTrackDuration(hFile, track,
            length, MP4_SECS_TIME_SCALE);
        info->set_length((double)sDuration);

        info->info_set_int(L"bitrate",(__int64)MP4GetTrackIntegerProperty(hFile,
            track, "mdia.minf.stbl.stsd.mp4a.esds.decConfigDescr.avgBitrate"));
        info->info_set_int(L"channels", (__int64)channels);
        info->info_set_int(L"samplerate", (__int64)m_samplerate);

        return 1;
    }

    input_mp4()
    {
        hFile = MP4_INVALID_FILE_HANDLE;
        hDecoder = NULL;
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
        void *sample_buffer;

        do {
            buffer = NULL;
            buffer_size = 0;

            MP4ReadSample(hFile, track, sampleId,
                (unsigned __int8**)&buffer, &buffer_size,
                NULL, NULL, NULL, NULL);
            sampleId++;

            sample_buffer = faacDecDecode(hDecoder, &frameInfo, buffer, buffer_size);

            if (buffer) free(buffer);

        } while ((frameInfo.error == 0) && (frameInfo.samples == 0));

        if (frameInfo.error || (sampleId > numSamples))
            return 0;

        chunk->data = (float*)sample_buffer;
        chunk->samples = frameInfo.samples/frameInfo.channels;
        chunk->nch = frameInfo.channels;
        chunk->srate = m_samplerate;

        return 1;
    }

    virtual int set_info(reader *r,const file_info * info)
    {
        return 1;
    }

    virtual int seek(double seconds)
    {
        MP4Duration duration;

        duration = MP4ConvertToTrackDuration(hFile,
            track, seconds, MP4_SECS_TIME_SCALE);
        sampleId = MP4GetSampleIdFromTime(hFile,
            track, duration, 0);

        if (sampleId == MP4_INVALID_SAMPLE_ID)
            sampleId = numSamples;

        return 1;
    }

private:

    MP4FileHandle hFile;
    MP4SampleId sampleId, numSamples;
    int track;

    unsigned __int32 m_samplerate;

    faacDecHandle hDecoder;


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
                unsigned long dummy1_32;
                unsigned char dummy2_8, dummy3_8, dummy4_8, dummy5_8, dummy6_8,
                    dummy7_8, dummy8_8;
                MP4GetTrackESConfiguration(infile, trackId,
                    (unsigned __int8**)&buff, (unsigned __int32*)&buff_size);

                if (buff)
                {
                    rc = AudioSpecificConfig(buff, buff_size, &dummy1_32, &dummy2_8,
                        &dummy3_8, &dummy4_8, &dummy5_8, &dummy6_8, &dummy7_8, &dummy8_8);
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
};

static service_factory_t<input,input_mp4> foo;
