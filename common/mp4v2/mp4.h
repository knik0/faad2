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
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#ifndef __MP4_INCLUDED__
#define __MP4_INCLUDED__

/* include system and project specific headers */
#include "mpeg4ip.h"

#include <math.h>	/* to define float HUGE_VAL and/or NAN */
#ifndef NAN
#define NAN HUGE_VAL
#endif

#ifdef __cplusplus
/* exploit C++ ability of default values for function parameters */
#define DEFAULT(x)	=x
#else
#define DEFAULT(x)
#endif

/* MP4 API types */
typedef void*		MP4FileHandle;
typedef u_int32_t	MP4TrackId;
typedef u_int32_t	MP4SampleId;
typedef u_int64_t	MP4Timestamp;
typedef u_int64_t	MP4Duration;

/* Invalid values for API types */
#define MP4_INVALID_FILE_HANDLE	((MP4FileHandle)NULL)
#define MP4_INVALID_TRACK_ID	((MP4TrackId)0)
#define MP4_INVALID_SAMPLE_ID	((MP4SampleId)0)
#define MP4_INVALID_TIMESTAMP	((MP4Timestamp)-1)
#define MP4_INVALID_DURATION	((MP4Duration)-1)

/* Macros to test for API type validity */
#define MP4_IS_VALID_FILE_HANDLE(x)	((x) != MP4_INVALID_FILE_HANDLE) 
#define MP4_IS_VALID_TRACK_ID(x)	((x) != MP4_INVALID_TRACK_ID) 
#define MP4_IS_VALID_SAMPLE_ID(x)	((x) != MP4_INVALID_SAMPLE_ID) 
#define MP4_IS_VALID_TIMESTAMP(x)	((x) != MP4_INVALID_TIMESTAMP) 
#define MP4_IS_VALID_DURATION(x)	((x) != MP4_INVALID_DURATION) 

/* MP4 verbosity levels - e.g. MP4SetVerbosity() */
#define MP4_DETAILS_ALL				0xFFFFFFFF
#define MP4_DETAILS_ERROR			0x00000001
#define MP4_DETAILS_WARNING			0x00000002
#define MP4_DETAILS_READ			0x00000004
#define MP4_DETAILS_WRITE			0x00000008
#define MP4_DETAILS_FIND			0x00000010
#define MP4_DETAILS_TABLE			0x00000020
#define MP4_DETAILS_SAMPLE			0x00000040
#define MP4_DETAILS_HINT			0x00000080
#define MP4_DETAILS_ISMA			0x00000100

#define MP4_DETAILS_READ_ALL		\
	(MP4_DETAILS_READ | MP4_DETAILS_TABLE | MP4_DETAILS_SAMPLE)
#define MP4_DETAILS_WRITE_ALL		\
	(MP4_DETAILS_WRITE | MP4_DETAILS_TABLE | MP4_DETAILS_SAMPLE)

/*
 * MP4 Known track type names - e.g. MP4GetNumberOfTracks(type) 
 *
 * Note this first group of track types should be created 
 * via the MP4Add<Type>Track() functions, and not MP4AddTrack(type)
 */
#define MP4_OD_TRACK_TYPE		"odsm"
#define MP4_SCENE_TRACK_TYPE	"sdsm"
#define MP4_AUDIO_TRACK_TYPE	"soun"
#define MP4_VIDEO_TRACK_TYPE	"vide"
#define MP4_HINT_TRACK_TYPE		"hint"
/*
 * This second set of track types should be created 
 * via MP4AddSystemsTrack(type)
 */
#define MP4_CLOCK_TRACK_TYPE	"crsm"
#define MP4_MPEG7_TRACK_TYPE	"m7sm"
#define MP4_OCI_TRACK_TYPE		"ocsm"
#define MP4_IPMP_TRACK_TYPE		"ipsm"
#define MP4_MPEGJ_TRACK_TYPE	"mjsm"

/* MP4 Audio track types - see MP4AddAudioTrack()*/
#define MP4_INVALID_AUDIO_TYPE			0x00
#define MP4_MPEG1_AUDIO_TYPE			0x6B
#define MP4_MPEG2_AUDIO_TYPE			0x69
#define MP4_MP3_AUDIO_TYPE				MP4_MPEG2_AUDIO_TYPE
#define MP4_MPEG2_AAC_MAIN_AUDIO_TYPE	0x66
#define MP4_MPEG2_AAC_LC_AUDIO_TYPE		0x67
#define MP4_MPEG2_AAC_SSR_AUDIO_TYPE	0x68
#define MP4_MPEG2_AAC_AUDIO_TYPE		MP4_MPEG2_AAC_MAIN_AUDIO_TYPE
#define MP4_MPEG4_AUDIO_TYPE			0x40
#define MP4_PRIVATE_AUDIO_TYPE			0xC0
#define MP4_PCM16_AUDIO_TYPE			0xD0	/* a private definition */

/* MP4 Video track types - see MP4AddVideoTrack() */
#define MP4_INVALID_VIDEO_TYPE			0x00
#define MP4_MPEG1_VIDEO_TYPE			0x6A
#define MP4_MPEG2_SIMPLE_VIDEO_TYPE		0x60
#define MP4_MPEG2_MAIN_VIDEO_TYPE		0x61
#define MP4_MPEG2_SNR_VIDEO_TYPE		0x62
#define MP4_MPEG2_SPATIAL_VIDEO_TYPE	0x63
#define MP4_MPEG2_HIGH_VIDEO_TYPE		0x64
#define MP4_MPEG2_442_VIDEO_TYPE		0x65
#define MP4_MPEG2_VIDEO_TYPE			MP4_MPEG2_MAIN_VIDEO_TYPE
#define MP4_MPEG4_VIDEO_TYPE			0x20
#define MP4_JPEG_VIDEO_TYPE				0x6C
#define MP4_PRIVATE_VIDEO_TYPE			0xC1
#define MP4_YUV12_VIDEO_TYPE			0xD1	/* a private definition */


/* MP4 API declarations */

#ifdef __cplusplus
extern "C" {
#endif

/* file operations */

MP4FileHandle MP4Create(const char* fileName, 
	u_int32_t verbosity DEFAULT(0),
	bool use64bits DEFAULT(0),
	bool useExtensibleFormat DEFAULT(0));

MP4FileHandle MP4Modify(const char* fileName, 
	u_int32_t verbosity DEFAULT(0),
	bool useExtensibleFormat DEFAULT(0));

MP4FileHandle MP4Read(const char* fileName, 
	u_int32_t verbosity DEFAULT(0));

bool MP4Close(MP4FileHandle hFile);

bool MP4Optimize(const char* existingFileName, 
	const char* newFileName DEFAULT(NULL), 
	u_int32_t verbosity DEFAULT(0));

bool MP4Dump(MP4FileHandle hFile, 
	FILE* pDumpFile DEFAULT(NULL), 
	bool dumpImplicits DEFAULT(0));

/* file properties */

/* specific file properties */

u_int32_t MP4GetVerbosity(MP4FileHandle hFile);

bool MP4SetVerbosity(MP4FileHandle hFile, u_int32_t verbosity);

MP4Duration MP4GetDuration(MP4FileHandle hFile);

u_int32_t MP4GetTimeScale(MP4FileHandle hFile);

bool MP4SetTimeScale(MP4FileHandle hFile, u_int32_t value);

u_int8_t MP4GetODProfileLevel(MP4FileHandle hFile);

bool MP4SetODProfileLevel(MP4FileHandle hFile, u_int8_t value);

u_int8_t MP4GetSceneProfileLevel(MP4FileHandle hFile);

bool MP4SetSceneProfileLevel(MP4FileHandle hFile, u_int8_t value);

u_int8_t MP4GetVideoProfileLevel(MP4FileHandle hFile);

bool MP4SetVideoProfileLevel(MP4FileHandle hFile, u_int8_t value);

u_int8_t MP4GetAudioProfileLevel(MP4FileHandle hFile);

bool MP4SetAudioProfileLevel(MP4FileHandle hFile, u_int8_t value);

u_int8_t MP4GetGraphicsProfileLevel(MP4FileHandle hFile);

bool MP4SetGraphicsProfileLevel(MP4FileHandle hFile, u_int8_t value);

/* generic file properties */

u_int64_t MP4GetIntegerProperty(
	MP4FileHandle hFile, const char* propName);

float MP4GetFloatProperty(
	MP4FileHandle hFile, const char* propName);

const char* MP4GetStringProperty(
	MP4FileHandle hFile, const char* propName);

void MP4GetBytesProperty(
	MP4FileHandle hFile, const char* propName,
	u_int8_t** ppValue, u_int32_t* pValueSize);

bool MP4SetIntegerProperty(
	MP4FileHandle hFile, const char* propName, int64_t value);

bool MP4SetFloatProperty(
	MP4FileHandle hFile, const char* propName, float value);

bool MP4SetStringProperty(
	MP4FileHandle hFile, const char* propName, const char* value);

bool MP4SetBytesProperty(
	MP4FileHandle hFile, const char* propName, 
	const u_int8_t* pValue, u_int32_t valueSize);

/* track operations */

MP4TrackId MP4AddTrack(
	MP4FileHandle hFile, const char* type);

MP4TrackId MP4AddSystemsTrack(
	MP4FileHandle hFile, const char* type);

MP4TrackId MP4AddODTrack(
	MP4FileHandle hFile);

MP4TrackId MP4AddSceneTrack(
	MP4FileHandle hFile);

MP4TrackId MP4AddAudioTrack(
	MP4FileHandle hFile, u_int32_t timeScale, u_int32_t sampleDuration,
	u_int8_t audioType DEFAULT(MP4_MPEG4_AUDIO_TYPE));

MP4TrackId MP4AddVideoTrack(
	MP4FileHandle hFile, u_int32_t timeScale, u_int32_t sampleDuration,
	u_int16_t width, u_int16_t height,
	u_int8_t videoType DEFAULT(MP4_MPEG4_VIDEO_TYPE));

MP4TrackId MP4AddHintTrack(
	MP4FileHandle hFile, MP4TrackId refTrackId);

bool MP4DeleteTrack(
	MP4FileHandle hFile, MP4TrackId trackId);

u_int32_t MP4GetNumberOfTracks(
	MP4FileHandle hFile, 
	const char* type DEFAULT(NULL),
	u_int8_t subType DEFAULT(0));

MP4TrackId MP4FindTrackId(
	MP4FileHandle hFile, 
	u_int16_t index, 
	const char* type DEFAULT(NULL),
	u_int8_t subType DEFAULT(0));

u_int16_t MP4FindTrackIndex(
	MP4FileHandle hFile, 
	MP4TrackId trackId);

/* track properties */

/* specific track properties */

const char* MP4GetTrackType(
	MP4FileHandle hFile, MP4TrackId trackId);

MP4Duration MP4GetTrackDuration(
	MP4FileHandle hFile, MP4TrackId trackId);

u_int32_t MP4GetTrackTimeScale(
	MP4FileHandle hFile, MP4TrackId trackId);

bool MP4SetTrackTimeScale(
	MP4FileHandle hFile, MP4TrackId trackId, u_int32_t value);

u_int8_t MP4GetTrackAudioType(
	MP4FileHandle hFile, MP4TrackId trackId);

u_int8_t MP4GetTrackVideoType(
	MP4FileHandle hFile, MP4TrackId trackId);

/* returns MP4_INVALID_DURATION if track samples do not have a fixed duration */
MP4Duration MP4GetTrackFixedSampleDuration(
	MP4FileHandle hFile, MP4TrackId trackId);

void MP4GetTrackESConfiguration(
	MP4FileHandle hFile, MP4TrackId trackId, 
	u_int8_t** ppConfig, u_int32_t* pConfigSize);

bool MP4SetTrackESConfiguration(
	MP4FileHandle hFile, MP4TrackId trackId, 
	const u_int8_t* pConfig, u_int32_t configSize);

MP4SampleId MP4GetTrackNumberOfSamples(
	MP4FileHandle hFile, MP4TrackId trackId);

/* generic track properties */

u_int64_t MP4GetTrackIntegerProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	const char* propName);

float MP4GetTrackFloatProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	const char* propName);

const char* MP4GetTrackStringProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	const char* propName);

void MP4GetTrackBytesProperty(
	MP4FileHandle hFile, MP4TrackId trackId, const char* propName,
	u_int8_t** ppValue, u_int32_t* pValueSize);

bool MP4SetTrackIntegerProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	const char* propName, int64_t value);

bool MP4SetTrackFloatProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	const char* propName, float value);

bool MP4SetTrackStringProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	const char* propName, const char* value);

bool MP4SetTrackBytesProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	const char* propName, const u_int8_t* pValue, u_int32_t valueSize);

/* sample operations */

bool MP4ReadSample(
	/* input parameters */
	MP4FileHandle hFile,
	MP4TrackId trackId, 
	MP4SampleId sampleId,
	/* input/output parameters */
	u_int8_t** ppBytes, 
	u_int32_t* pNumBytes, 
	/* output parameters */
	MP4Timestamp* pStartTime DEFAULT(NULL), 
	MP4Duration* pDuration DEFAULT(NULL),
	MP4Duration* pRenderingOffset DEFAULT(NULL), 
	bool* pIsSyncSample DEFAULT(NULL));

bool MP4WriteSample(
	MP4FileHandle hFile,
	MP4TrackId trackId,
	u_int8_t* pBytes, 
	u_int32_t numBytes,
	MP4Duration duration DEFAULT(MP4_INVALID_DURATION),
	MP4Duration renderingOffset DEFAULT(0), 
	bool isSyncSample DEFAULT(true));

u_int32_t MP4GetSampleSize(
	MP4FileHandle hFile,
	MP4TrackId trackId, 
	MP4SampleId sampleId);

u_int32_t MP4GetTrackMaxSampleSize(
	MP4FileHandle hFile,
	MP4TrackId trackId); 

MP4SampleId MP4GetSampleIdFromTime(
	MP4FileHandle hFile,
	MP4TrackId trackId, 
	MP4Timestamp when, 
	bool wantSyncSample DEFAULT(false));

MP4Timestamp MP4GetSampleTime(
	MP4FileHandle hFile,
	MP4TrackId trackId, 
	MP4SampleId sampleId);

MP4Duration MP4GetSampleDuration(
	MP4FileHandle hFile,
	MP4TrackId trackId, 
	MP4SampleId sampleId);

MP4Duration MP4GetSampleRenderingOffset(
	MP4FileHandle hFile,
	MP4TrackId trackId, 
	MP4SampleId sampleId);

bool MP4SetSampleRenderingOffset(
	MP4FileHandle hFile,
	MP4TrackId trackId, 
	MP4SampleId sampleId,
	MP4Duration renderingOffset);

int8_t MP4GetSampleSync(
	MP4FileHandle hFile,
	MP4TrackId trackId, 
	MP4SampleId sampleId);

/* rtp hint track operations */

bool MP4GetHintTrackRtpPayload(
	MP4FileHandle hFile,
	MP4TrackId hintTrackId,
	char** ppPayloadName DEFAULT(NULL),
	u_int8_t* pPayloadNumber DEFAULT(NULL),
	u_int16_t* pMaxPayloadSize DEFAULT(NULL));

bool MP4SetHintTrackRtpPayload(
	MP4FileHandle hFile,
	MP4TrackId hintTrackId,
	const char* pPayloadName,
	u_int8_t* pPayloadNumber,
	u_int16_t maxPayloadSize DEFAULT(0));

const char* MP4GetSessionSdp(
	MP4FileHandle hFile);

bool MP4SetSessionSdp(
	MP4FileHandle hFile,
	const char* sdpString);

bool MP4AppendSessionSdp(
	MP4FileHandle hFile,
	const char* sdpString);

const char* MP4GetHintTrackSdp(
	MP4FileHandle hFile,
	MP4TrackId hintTrackId);

bool MP4SetHintTrackSdp(
	MP4FileHandle hFile,
	MP4TrackId hintTrackId,
	const char* sdpString);

bool MP4AppendHintTrackSdp(
	MP4FileHandle hFile,
	MP4TrackId hintTrackId,
	const char* sdpString);

MP4TrackId MP4GetHintTrackReferenceTrackId(
	MP4FileHandle hFile,
	MP4TrackId hintTrackId);

bool MP4ReadRtpHint(
	MP4FileHandle hFile,
	MP4TrackId hintTrackId,
	MP4SampleId hintSampleId,
	u_int16_t* pNumPackets DEFAULT(NULL));

u_int16_t MP4GetRtpHintNumberOfPackets(
	MP4FileHandle hFile,
	MP4TrackId hintTrackId);

int8_t MP4GetRtpPacketBFrame(
	MP4FileHandle hFile,
	MP4TrackId hintTrackId,
	u_int16_t packetIndex);

int32_t MP4GetRtpPacketTransmitOffset(
	MP4FileHandle hFile,
	MP4TrackId hintTrackId,
	u_int16_t packetIndex);

bool MP4ReadRtpPacket(
	MP4FileHandle hFile,
	MP4TrackId hintTrackId,
	u_int16_t packetIndex,
	u_int8_t** ppBytes, 
	u_int32_t* pNumBytes,
	u_int32_t ssrc DEFAULT(0),
	bool includeHeader DEFAULT(true),
	bool includePayload DEFAULT(true));

MP4Timestamp MP4GetRtpTimestampStart(
	MP4FileHandle hFile,
	MP4TrackId hintTrackId);

bool MP4SetRtpTimestampStart(
	MP4FileHandle hFile,
	MP4TrackId hintTrackId,
	MP4Timestamp rtpStart);

bool MP4AddRtpHint(
	MP4FileHandle hFile,
	MP4TrackId hintTrackId);

bool MP4AddRtpVideoHint(
	MP4FileHandle hFile,
	MP4TrackId hintTrackId,
	bool isBframe DEFAULT(false), 
	u_int32_t timestampOffset DEFAULT(0));

bool MP4AddRtpPacket(
	MP4FileHandle hFile,
	MP4TrackId hintTrackId,
	bool setMbit DEFAULT(false),
	int32_t transmitOffset DEFAULT(0));

bool MP4AddRtpImmediateData(
	MP4FileHandle hFile,
	MP4TrackId hintTrackId,
	const u_int8_t* pBytes,
	u_int32_t numBytes);

bool MP4AddRtpSampleData(
	MP4FileHandle hFile,
	MP4TrackId hintTrackId,
	MP4SampleId sampleId,
	u_int32_t dataOffset,
	u_int32_t dataLength);

bool MP4AddRtpESConfigurationPacket(
	MP4FileHandle hFile,
	MP4TrackId hintTrackId);

bool MP4WriteRtpHint(
	MP4FileHandle hFile,
	MP4TrackId hintTrackId,
	MP4Duration duration,
	bool isSyncSample DEFAULT(true));

/* ISMA specific operations */

bool MP4MakeIsmaCompliant(const char* fileName, 
	u_int32_t verbosity DEFAULT(0),
	bool addIsmaComplianceSdp DEFAULT(true));

/* time conversion utilties */

/* predefined values for timeScale parameter below */
#define MP4_SECONDS_TIME_SCALE		1
#define MP4_MILLISECONDS_TIME_SCALE 1000
#define MP4_MICROSECONDS_TIME_SCALE 1000000
#define MP4_NANOSECONDS_TIME_SCALE 	1000000000

#define MP4_SECS_TIME_SCALE 	MP4_SECONDS_TIME_SCALE
#define MP4_MSECS_TIME_SCALE	MP4_MILLISECONDS_TIME_SCALE
#define MP4_USECS_TIME_SCALE	MP4_MICROSECONDS_TIME_SCALE
#define MP4_NSECS_TIME_SCALE	MP4_NANOSECONDS_TIME_SCALE

u_int64_t MP4ConvertFromMovieDuration(
	MP4FileHandle hFile,
	MP4Duration duration,
	u_int32_t timeScale);

u_int64_t MP4ConvertFromTrackTimestamp(
	MP4FileHandle hFile,
	MP4TrackId trackId, 
	MP4Timestamp timeStamp,
	u_int32_t timeScale);

MP4Timestamp MP4ConvertToTrackTimestamp(
	MP4FileHandle hFile,
	MP4TrackId trackId, 
	u_int64_t timeStamp,
	u_int32_t timeScale);

u_int64_t MP4ConvertFromTrackDuration(
	MP4FileHandle hFile,
	MP4TrackId trackId, 
	MP4Duration duration,
	u_int32_t timeScale);

MP4Duration MP4ConvertToTrackDuration(
	MP4FileHandle hFile,
	MP4TrackId trackId, 
	u_int64_t duration,
	u_int32_t timeScale);

char* MP4BinaryToBase16(
	const u_int8_t* pData, 
	u_int32_t dataSize);

char* MP4BinaryToBase64(
	const u_int8_t* pData, 
	u_int32_t dataSize);

#ifdef __cplusplus
}
#endif

/* undefined our utlity macro to avoid conflicts */
#undef DEFAULT

#endif /* __MP4_INCLUDED__ */
