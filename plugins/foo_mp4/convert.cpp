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
**/

#include "convert.h"

static int GetAACTrack(MP4FileHandle infile)
{
    // find AAC track
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

    // can't decode this
    return -1;
}

converter::converter(const char *in_file, const char *out_file, const file_info *infos)
{
    in = 0;
    out = 0;

    if (in_file) in = file::g_open(in_file, reader::MODE_READ);
    if (out_file) out = file::g_open(out_file, reader::MODE_WRITE_NEW);

    mp4File = MP4_INVALID_FILE_HANDLE;
    if (infos)
    {
        info.copy(infos);
        info.info_remove_all();
    }
    else
    {
        info.reset();
    }
}

converter::~converter()
{
    if (mp4File != MP4_INVALID_FILE_HANDLE) MP4Close(mp4File);
    if (in) in->reader_release();
    if (out) out->reader_release();
}

bool converter::aac_to_mp4()
{
    if (!in || !out) return false;

    mp4File = MP4CreateCb(0, 0, 0, open_cb, close_cb, read_cb, write_cb, setpos_cb, getpos_cb, filesize_cb, (void *)out);
    if (mp4File == MP4_INVALID_FILE_HANDLE) return false;

    MP4SetTimeScale(mp4File, 90000);

    MP4TrackId createdTrackId = ConvertAAC();
    if (createdTrackId == MP4_INVALID_TRACK_ID) return false;

    WriteTagMP4();

    /*
    int allMpeg4Streams = 0;
    const char *type = MP4GetTrackType(mp4File, createdTrackId);

    if (!strcmp(type, MP4_AUDIO_TRACK_TYPE))
        allMpeg4Streams &= (MP4GetTrackAudioType(mp4File, createdTrackId) == MP4_MPEG4_AUDIO_TYPE);

    MP4Close(mp4File);
    mp4File = MP4_INVALID_FILE_HANDLE;
    MP4MakeIsmaCompliant(mp4FileName, 0, allMpeg4Streams);
    */

    return true;
}

bool converter::mp4_to_aac()
{
    if (!in || !out) return false;

    mp4File = MP4ReadCb(0, open_cb, close_cb, read_cb, write_cb, setpos_cb, getpos_cb, filesize_cb, (void*)in);
    if (mp4File == MP4_INVALID_FILE_HANDLE) return false;

    MP4TrackId track = GetAACTrack(mp4File);
    if (track < 0) return false;

    if (!ExtractTrack(track)) return false;

    WriteTagAAC();

    return true;
}

// private:

// hdr must point to at least ADTS_HEADER_MAX_SIZE bytes of memory
bool converter::LoadNextAdtsHeader(u_int8_t *hdr)
{
    u_int state = 0;
    u_int dropped = 0;
    u_int hdrByteSize = ADTS_HEADER_MAX_SIZE;

    while (1) {
        u_int8_t b;
        if (in->read(&b, 1) != 1) return false;

        // header is complete, return it
        if (state == hdrByteSize - 1) {
            hdr[state] = b;
            if (dropped > 0)
                console::warning(string_printf("Dropped %u input bytes", dropped));
            return true;
        }

        // collect requisite number of bytes, no constraints on data
        if (state >= 2) {
            hdr[state++] = b;
        } else {
            // have first byte, check if we have 1111X00X
            if (state == 1) {
                if ((b & 0xF6) == 0xF0) {
                    hdr[state] = b;
                    state = 2;
                    // compute desired header size
                    hdrByteSize = MP4AV_AdtsGetHeaderByteSize(hdr);
                } else {
                    state = 0;
                }
            }
            // initial state, looking for 11111111
            if (state == 0) {
                if (b == 0xFF) {
                    hdr[state] = b;
                    state = 1;
                } else {
                    // else drop it
                    dropped++;
                }
            }
        }
    }
}

// Load the next frame from the file
// into the supplied buffer, which better be large enough!
//
// Note: Frames are padded to byte boundaries
bool converter::LoadNextAacFrame(u_int8_t *pBuf, u_int32_t *pBufSize, bool stripAdts)
{
    u_int16_t frameSize;
    u_int16_t hdrBitSize, hdrByteSize;
    u_int8_t hdrBuf[ADTS_HEADER_MAX_SIZE];

    // get the next AAC frame header, more or less
    if (!LoadNextAdtsHeader(hdrBuf)) return false;

    // get frame size from header
    frameSize = MP4AV_AdtsGetFrameSize(hdrBuf);

    // get header size in bits and bytes from header
    hdrBitSize = MP4AV_AdtsGetHeaderBitSize(hdrBuf);
    hdrByteSize = MP4AV_AdtsGetHeaderByteSize(hdrBuf);

    // adjust the frame size to what remains to be read
    frameSize -= hdrByteSize;

    if (stripAdts) {
        if ((hdrBitSize % 8) == 0) {
            // header is byte aligned, i.e. MPEG-2 ADTS
            // read the frame data into the buffer
            if (in->read(pBuf, frameSize) != frameSize) return false;
            (*pBufSize) = frameSize;
        } else {
            // header is not byte aligned, i.e. MPEG-4 ADTS
            u_int8_t newByte;
            int upShift = hdrBitSize % 8;
            int downShift = 8 - upShift;

            pBuf[0] = hdrBuf[hdrBitSize / 8] << upShift;

            for (int i = 0; i < frameSize; i++) {
                if (in->read(&newByte, 1) != 1) return false;
                pBuf[i] |= (newByte >> downShift);
                pBuf[i+1] = (newByte << upShift);
            }
            (*pBufSize) = frameSize + 1;
        }
    } else { // don't strip ADTS headers
        memcpy(pBuf, hdrBuf, hdrByteSize);
        if (in->read(&pBuf[hdrByteSize], frameSize) != frameSize) return false;
    }

    return true;
}

bool converter::GetFirstHeader()
{
    // already read first header
    if (firstHeader[0] == 0xff) return true;

    __int64 pos = in->get_position();
    in->seek(0);

    if (!LoadNextAdtsHeader(firstHeader)) return false;

    in->seek(pos);

    return true;
}

MP4TrackId converter::ConvertAAC()
{
    // collect all the necessary meta information
    u_int32_t samplesPerSecond;
    u_int8_t mpegVersion;
    u_int8_t profile;
    u_int8_t channelConfig;

    if (!GetFirstHeader()) return MP4_INVALID_TRACK_ID;

    samplesPerSecond = MP4AV_AdtsGetSamplingRate(firstHeader);
    mpegVersion = MP4AV_AdtsGetVersion(firstHeader);
    profile = MP4AV_AdtsGetProfile(firstHeader);
    channelConfig = MP4AV_AdtsGetChannels(firstHeader);

    u_int8_t audioType = MP4_INVALID_AUDIO_TYPE;
    switch (mpegVersion) {
    case 0:
        audioType = MP4_MPEG4_AUDIO_TYPE;
        break;
    case 1:
        switch (profile) {
        case 0:
            audioType = MP4_MPEG2_AAC_MAIN_AUDIO_TYPE;
            break;
        case 1:
            audioType = MP4_MPEG2_AAC_LC_AUDIO_TYPE;
            break;
        case 2:
            audioType = MP4_MPEG2_AAC_SSR_AUDIO_TYPE;
            break;
        case 3:
            return MP4_INVALID_TRACK_ID;
        }
        break;
    }

    // add the new audio track
    MP4TrackId trackId = MP4AddAudioTrack(mp4File, samplesPerSecond, 1024, audioType);

    if (trackId == MP4_INVALID_TRACK_ID) return MP4_INVALID_TRACK_ID;

    if (MP4GetNumberOfTracks(mp4File, MP4_AUDIO_TRACK_TYPE) == 1)
        MP4SetAudioProfileLevel(mp4File, 0x0F);

    u_int8_t* pConfig = NULL;
    u_int32_t configLength = 0;

    MP4AV_AacGetConfiguration(
        &pConfig,
        &configLength,
        profile,
        samplesPerSecond,
        channelConfig);

    if (!MP4SetTrackESConfiguration(mp4File, trackId, pConfig, configLength)) {
        MP4DeleteTrack(mp4File, trackId);
        return MP4_INVALID_TRACK_ID;
    }

    // parse the ADTS frames, and write the MP4 samples
    u_int8_t sampleBuffer[8 * 1024];
    u_int32_t sampleSize = sizeof(sampleBuffer);
    MP4SampleId sampleId = 1;

    while (LoadNextAacFrame(sampleBuffer, &sampleSize, true)) {
        if (!MP4WriteSample(mp4File, trackId, sampleBuffer, sampleSize)) {
            MP4DeleteTrack(mp4File, trackId);
            return MP4_INVALID_TRACK_ID;
        }
        sampleId++;
        sampleSize = sizeof(sampleBuffer);
    }

    return trackId;
}

bool converter::WriteTagMP4()
{
    int count = info.meta_get_count();

    if (count <= 0) return true;

    for ( int i = 0; i < count; i++ ) {
        char *pName = (char *)info.meta_enum_name ( i );
        const char *val = info.meta_enum_value ( i );
        if ( !val || (val && !(*val)) ) continue;

        if ( !stricmp (pName, "TITLE") ) {
            MP4SetMetadataName ( mp4File, val );
        }
        else if ( !stricmp (pName, "ARTIST") ) {
            MP4SetMetadataArtist ( mp4File, val );
        }
        else if ( !stricmp (pName, "WRITER") ) {
            MP4SetMetadataWriter ( mp4File, val );
        }
        else if ( !stricmp (pName, "ALBUM") ) {
            MP4SetMetadataAlbum ( mp4File, val );
        }
        else if ( !stricmp (pName, "YEAR") || !stricmp (pName, "DATE") ) {
            MP4SetMetadataYear ( mp4File, val );
        }
        else if ( !stricmp (pName, "COMMENT") ) {
            MP4SetMetadataComment ( mp4File, val );
        }
        else if ( !stricmp (pName, "GENRE") ) {
            MP4SetMetadataGenre ( mp4File, val );
        }
        else if ( !stricmp (pName, "TRACKNUMBER") ) {
            unsigned __int16 trkn = atoi(val), tot = 0;
            MP4SetMetadataTrack ( mp4File, trkn, tot );
        }
        else if ( !stricmp (pName, "DISKNUMBER") || !stricmp (pName, "DISC") ) {
            unsigned __int16 disk = atoi(val), tot = 0;
            MP4SetMetadataDisk ( mp4File, disk, tot );
        }
        else if ( !stricmp (pName, "COMPILATION") ) {
            unsigned __int8 cpil = atoi(val);
            MP4SetMetadataCompilation ( mp4File, cpil );
        }
        else if ( !stricmp (pName, "TEMPO") ) {
            unsigned __int16 tempo = atoi(val);
            MP4SetMetadataTempo ( mp4File, tempo );
        } else {
            MP4SetMetadataFreeForm ( mp4File, pName, (unsigned __int8*)val, strlen(val) );
        }
    }

    return true;
}

bool converter::WriteTagAAC()
{
    if (info.meta_get_count() <= 0) return true;

    return !!tag_writer::g_run(out, &info, "ape");
}

bool converter::ExtractTrack(MP4TrackId trackId)
{
    // some track types have special needs
    // to properly recreate their raw ES file

    bool prependES = false;
    bool prependADTS = false;

    const char *trackType = MP4GetTrackType(mp4File, trackId);

    if (!strcmp(trackType, MP4_VIDEO_TRACK_TYPE))
    {
        if (MP4_IS_MPEG4_VIDEO_TYPE(MP4GetTrackEsdsObjectTypeId(mp4File, trackId)))
            prependES = true;
    }
    else if (!strcmp(trackType, MP4_AUDIO_TRACK_TYPE))
    {
        if (MP4_IS_AAC_AUDIO_TYPE(MP4GetTrackEsdsObjectTypeId(mp4File, trackId)))
            prependADTS = true;
    }

    /*
    u_int8_t *pConfig = NULL;
    u_int32_t configSize = 0;
    mp4AudioSpecificConfig mp4ASC;

    MP4GetTrackESConfiguration(mp4File, trackId, &pConfig, &configSize);
    if (!pConfig)
    {
        console::error("Unable to read track specific configuration.");
        return false;
    }

    AudioSpecificConfig((unsigned char*)pConfig, configSize, &mp4ASC);
    */

    MP4SampleId numSamples = MP4GetTrackNumberOfSamples(mp4File, trackId);
    u_int8_t *pSample;
    u_int32_t sampleSize;

    // extraction loop
    for (MP4SampleId sampleId = 1; sampleId <= numSamples; sampleId++)
    {
        // signal to ReadSample() 
        // that it should malloc a buffer for us
        pSample = NULL;
        sampleSize = 0;

        if (prependADTS)
        {
            // need some very specialized work for these
            MP4AV_AdtsMakeFrameFromMp4Sample(
                mp4File,
                trackId,
                sampleId,
                0/*aacProfileLevel*/,
                &pSample,
                &sampleSize);
        }
        else
        {
            // read the sample
            int rc = MP4ReadSample(
                mp4File, 
                trackId, 
                sampleId, 
                &pSample, 
                &sampleSize);

            if (rc == 0 || !pSample)
            {
                if (pSample) free(pSample);
                console::error(string_printf("Failed to read sample %u", (unsigned int)sampleId));
                return false;
            }

            if (prependES && sampleId == 1)
            {
                u_int8_t *pConfig = NULL;
                u_int32_t configSize = 0;

                MP4GetTrackESConfiguration(mp4File, trackId, &pConfig, &configSize);
                if (!pConfig)
                {
                    console::error("Unable to read track specific configuration.");
                    return false;
                }

                if (out->write(pConfig, configSize) != configSize)
                {
                    free(pConfig);
                    console::error("Failed to write to output");
                    return false;
                }

                free(pConfig);
            }
        }

        if (out->write(pSample, sampleSize) != sampleSize)
        {
            free(pSample);
            console::error("Failed to write to output");
            return false;
        }

        free(pSample);
    }

    return true;
}
