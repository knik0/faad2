/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

/* 
 * Notes:
 *  - file formatted with tabstops == 4 spaces 
 */

#include <mpeg4ip.h>
#include <mp4.h>
#include "mp4av.h"

#include "aac2mp4.h"

#define ADTS_HEADER_MAX_SIZE 10 /* bytes */

static u_int8_t firstHeader[ADTS_HEADER_MAX_SIZE];

/* 
 * hdr must point to at least ADTS_HEADER_MAX_SIZE bytes of memory 
 */
static bool LoadNextAdtsHeader(FILE* inFile, u_int8_t* hdr)
{
	u_int state = 0;
	u_int dropped = 0;
	u_int hdrByteSize = ADTS_HEADER_MAX_SIZE;

	while (1) {
		/* read a byte */
		u_int8_t b;

		if (fread(&b, 1, 1, inFile) == 0) {
			return false;
		}

		/* header is complete, return it */
		if (state == hdrByteSize - 1) {
			hdr[state] = b;
			if (dropped > 0) {
				fprintf(stderr, "Warning: dropped %u input bytes\n", dropped);
			}
			return true;
		}

		/* collect requisite number of bytes, no constraints on data */
		if (state >= 2) {
			hdr[state++] = b;
		} else {
			/* have first byte, check if we have 1111X00X */
			if (state == 1) {
				if ((b & 0xF6) == 0xF0) {
					hdr[state] = b;
					state = 2;
					/* compute desired header size */
					hdrByteSize = MP4AV_AdtsGetHeaderByteSize(hdr);
				} else {
					state = 0;
				}
			}
			/* initial state, looking for 11111111 */
			if (state == 0) {
				if (b == 0xFF) {
					hdr[state] = b;
					state = 1;
				} else {
					 /* else drop it */ 
					dropped++;
				}
			}
		}
	}
}

/*
 * Load the next frame from the file
 * into the supplied buffer, which better be large enough!
 *
 * Note: Frames are padded to byte boundaries
 */
static bool LoadNextAacFrame(FILE* inFile, u_int8_t* pBuf, u_int32_t* pBufSize, bool stripAdts)
{
	u_int16_t frameSize;
	u_int16_t hdrBitSize, hdrByteSize;
	u_int8_t hdrBuf[ADTS_HEADER_MAX_SIZE];

	/* get the next AAC frame header, more or less */
	if (!LoadNextAdtsHeader(inFile, hdrBuf)) {
		return false;
	}
	
	/* get frame size from header */
	frameSize = MP4AV_AdtsGetFrameSize(hdrBuf);

	/* get header size in bits and bytes from header */
	hdrBitSize = MP4AV_AdtsGetHeaderBitSize(hdrBuf);
	hdrByteSize = MP4AV_AdtsGetHeaderByteSize(hdrBuf);
	
	/* adjust the frame size to what remains to be read */
	frameSize -= hdrByteSize;

	if (stripAdts) {
		if ((hdrBitSize % 8) == 0) {
			/* header is byte aligned, i.e. MPEG-2 ADTS */
			/* read the frame data into the buffer */
			if (fread(pBuf, 1, frameSize, inFile) != frameSize) {
				return false;
			}
			(*pBufSize) = frameSize;
		} else {
			/* header is not byte aligned, i.e. MPEG-4 ADTS */
			int i;
			u_int8_t newByte;
			int upShift = hdrBitSize % 8;
			int downShift = 8 - upShift;

			pBuf[0] = hdrBuf[hdrBitSize / 8] << upShift;

			for (i = 0; i < frameSize; i++) {
				if (fread(&newByte, 1, 1, inFile) != 1) {
					return false;
				}
				pBuf[i] |= (newByte >> downShift);
				pBuf[i+1] = (newByte << upShift);
			}
			(*pBufSize) = frameSize + 1;
		}
	} else { /* don't strip ADTS headers */
		memcpy(pBuf, hdrBuf, hdrByteSize);
		if (fread(&pBuf[hdrByteSize], 1, frameSize, inFile) != frameSize) {
			return false;
		}
	}

	return true;
}

static bool GetFirstHeader(FILE* inFile)
{
	/* read file until we find an audio frame */
	fpos_t curPos;

	/* already read first header */
	if (firstHeader[0] == 0xff) {
		return true;
	}

	/* remember where we are */
	fgetpos(inFile, &curPos);
	
	/* go back to start of file */
	rewind(inFile);

	if (!LoadNextAdtsHeader(inFile, firstHeader)) {
		return false;
	}

	/* reposition the file to where we were */
	fsetpos(inFile, &curPos);

	return true;
}

MP4TrackId AacCreator(MP4FileHandle mp4File, FILE* inFile)
{
	// collect all the necessary meta information
	u_int32_t samplesPerSecond;
	u_int8_t mpegVersion;
	u_int8_t profile;
	u_int8_t channelConfig;

	if (!GetFirstHeader(inFile)) {
		return MP4_INVALID_TRACK_ID;
	}

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
	MP4TrackId trackId = 
		MP4AddAudioTrack(mp4File, 
			samplesPerSecond, 1024, audioType);

	if (trackId == MP4_INVALID_TRACK_ID) {
		return MP4_INVALID_TRACK_ID;
	}

	if (MP4GetNumberOfTracks(mp4File, MP4_AUDIO_TRACK_TYPE) == 1) {
		MP4SetAudioProfileLevel(mp4File, 0x0F);
	}

	u_int8_t* pConfig = NULL;
	u_int32_t configLength = 0;

	MP4AV_AacGetConfiguration(
		&pConfig,
		&configLength,
		profile,
		samplesPerSecond,
		channelConfig);

	if (!MP4SetTrackESConfiguration(mp4File, trackId, 
	  pConfig, configLength)) {
		MP4DeleteTrack(mp4File, trackId);
		return MP4_INVALID_TRACK_ID;
	}

	// parse the ADTS frames, and write the MP4 samples
	u_int8_t sampleBuffer[8 * 1024];
	u_int32_t sampleSize = sizeof(sampleBuffer);
	MP4SampleId sampleId = 1;

	while (LoadNextAacFrame(inFile, sampleBuffer, &sampleSize, true)) {
		if (!MP4WriteSample(mp4File, trackId, sampleBuffer, sampleSize)) {
			MP4DeleteTrack(mp4File, trackId);
			return MP4_INVALID_TRACK_ID;
		}
		sampleId++;
		sampleSize = sizeof(sampleBuffer);
	}

	return trackId;
}

