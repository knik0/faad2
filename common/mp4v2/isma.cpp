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

#include "mp4common.h"

void MP4File::MakeIsmaCompliant(bool addIsmaComplianceSdp)
{
	ProtectWriteOperation("MP4MakeIsmaCompliant");

	if (m_useIsma) {
		// already done
		return;
	}
	m_useIsma = true;

	// find first audio and/or video tracks

	MP4TrackId audioTrackId = MP4_INVALID_TRACK_ID;
	try {
		audioTrackId = FindTrackId(0, MP4_AUDIO_TRACK_TYPE);
	}
	catch (MP4Error* e) {
		delete e;
	}

	MP4TrackId videoTrackId = MP4_INVALID_TRACK_ID;
	try {
		videoTrackId = FindTrackId(0, MP4_VIDEO_TRACK_TYPE);
	}
	catch (MP4Error* e) {
		delete e;
	}

	// delete any existing OD track
	if (m_odTrackId != MP4_INVALID_TRACK_ID) {
		DeleteTrack(m_odTrackId);
	}

	AddODTrack();
	SetODProfileLevel(0xFF);

	if (audioTrackId != MP4_INVALID_TRACK_ID) {
		AddTrackToOd(audioTrackId);
	}

	if (videoTrackId != MP4_INVALID_TRACK_ID) {
		AddTrackToOd(videoTrackId);
	}

	// delete any existing scene track
	MP4TrackId sceneTrackId = MP4_INVALID_TRACK_ID;
	try {
		sceneTrackId = FindTrackId(0, MP4_SCENE_TRACK_TYPE);
	}
	catch (MP4Error *e) {
		delete e;
	}
	if (sceneTrackId != MP4_INVALID_TRACK_ID) {
		DeleteTrack(sceneTrackId);
	}

	// add scene track
	sceneTrackId = AddSceneTrack();
	SetSceneProfileLevel(0xFF);
	SetGraphicsProfileLevel(0xFF);
	SetTrackIntegerProperty(sceneTrackId, 
		"mdia.minf.stbl.stsd.mp4s.esds.decConfigDescr.objectTypeId", 
		MP4SystemsV2ObjectType);
	static u_int8_t bifsv2Config[3] = {
		0x00, 0x00, 0x40 // IsCommandStream
	};
	SetTrackESConfiguration(sceneTrackId, 
		bifsv2Config, sizeof(bifsv2Config));

	u_int8_t* pBytes = NULL;
	u_int64_t numBytes = 0;

	// write OD Update Command
	CreateIsmaODUpdateCommand(
		m_odTrackId, audioTrackId, videoTrackId, true,
		&pBytes, &numBytes);

	WriteSample(m_odTrackId, pBytes, numBytes, 1);

	MP4Free(pBytes);
	pBytes = NULL;

	// write BIFS Scene Replace Command
	CreateIsmaSceneCommand(
		sceneTrackId, audioTrackId, videoTrackId,
		&pBytes, &numBytes);

	WriteSample(sceneTrackId, pBytes, numBytes, 1);

	MP4Free(pBytes);
	pBytes = NULL;

	// add session level sdp 
	CreateIsmaIod(
		m_odTrackId, sceneTrackId, audioTrackId, videoTrackId,
		&pBytes, &numBytes);

	char* sdpBuf = (char*)MP4Calloc(numBytes + 256);

	if (addIsmaComplianceSdp) {
		strcpy(sdpBuf, "a=isma-compliance:1,1.0,1\015\012");
	}

	sprintf(&sdpBuf[strlen(sdpBuf)], 
		"a=mpeg4-iod: \042data:application/mpeg4-iod;base64,%s\042\015\012",
		MP4ToBase64(pBytes, numBytes));

	SetSessionSdp(sdpBuf);

	VERBOSE_ISMA(GetVerbosity(),
		printf("IOD SDP = %s\n", sdpBuf));

	MP4Free(pBytes);
	pBytes = NULL;
	MP4Free(sdpBuf);
	sdpBuf = NULL;
}

static void CloneIntegerProperty(
	MP4Descriptor* pDest, 
	MP4DescriptorProperty* pSrc,
	const char* name)
{
	MP4IntegerProperty* pGetProperty;
	MP4IntegerProperty* pSetProperty;

	pSrc->FindProperty(name, (MP4Property**)&pGetProperty);
	pDest->FindProperty(name, (MP4Property**)&pSetProperty);

	pSetProperty->SetValue(pGetProperty->GetValue());
} 

void MP4File::CreateIsmaIod(
	MP4TrackId odTrackId,
	MP4TrackId sceneTrackId,
	MP4TrackId audioTrackId, 
	MP4TrackId videoTrackId,
	u_int8_t** ppBytes,
	u_int64_t* pNumBytes)
{
	MP4Descriptor* pIod = new MP4IODescriptor();
	pIod->SetTag(MP4IODescrTag);
	pIod->Generate();

	MP4Atom* pIodsAtom = FindAtom("moov.iods");
	ASSERT(pIodsAtom);
	MP4DescriptorProperty* pSrcIod = 
		(MP4DescriptorProperty*)pIodsAtom->GetProperty(2);

	CloneIntegerProperty(pIod, pSrcIod, "objectDescriptorId");
	CloneIntegerProperty(pIod, pSrcIod, "ODProfileLevelId");
	CloneIntegerProperty(pIod, pSrcIod, "sceneProfileLevelId");
	CloneIntegerProperty(pIod, pSrcIod, "audioProfileLevelId");
	CloneIntegerProperty(pIod, pSrcIod, "visualProfileLevelId");
	CloneIntegerProperty(pIod, pSrcIod, "graphicsProfileLevelId");

	// mutate esIds from MP4ESIDIncDescrTag to MP4ESDescrTag
	MP4DescriptorProperty* pEsProperty;
	pIod->FindProperty("esIds", (MP4Property**)&pEsProperty);
	pEsProperty->SetTags(MP4ESDescrTag);

	MP4IntegerProperty* pSetProperty;

	// OD
	MP4Descriptor* pOdEsd =
		pEsProperty->AddDescriptor(MP4ESDescrTag);
	pOdEsd->Generate();

	pOdEsd->FindProperty("ESID", 
		(MP4Property**)&pSetProperty);
	pSetProperty->SetValue(m_odTrackId);

	pOdEsd->FindProperty("URLFlag", 
		(MP4Property**)&pSetProperty);
	pSetProperty->SetValue(1);

	u_int8_t* pBytes;
	u_int64_t numBytes;

	CreateIsmaODUpdateCommand(
		m_odTrackId, audioTrackId, videoTrackId, false,
		&pBytes, &numBytes);

	VERBOSE_ISMA(GetVerbosity(),
		printf("OD data =\n"); MP4HexDump(pBytes, numBytes));

	MP4StringProperty* pUrlProperty;
	char* urlBuf = NULL;

	urlBuf = (char*)MP4Malloc((numBytes * 4 / 3) + 64);

	sprintf(urlBuf, 
		"data:application/mpeg4-od-au;base64,%s",
		 MP4ToBase64(pBytes, numBytes));

	pOdEsd->FindProperty("URL", 
		(MP4Property**)&pUrlProperty);
	pUrlProperty->SetValue(urlBuf);

	VERBOSE_ISMA(GetVerbosity(),
		printf("OD data URL = \042%s\042\n", urlBuf));

	MP4Free(pBytes);
	pBytes = NULL;
	MP4Free(urlBuf);
	urlBuf = NULL;

	MP4DescriptorProperty* pSrcDcd = NULL;

	// HACK temporarily point to scene decoder config
	FindProperty(MakeTrackName(odTrackId, 
		"mdia.minf.stbl.stsd.mp4s.esds.decConfigDescr"),
		(MP4Property**)&pSrcDcd);
	ASSERT(pSrcDcd);
	MP4Property* pOrgOdEsdProperty = 
		pOdEsd->GetProperty(8);
	pOdEsd->SetProperty(8, pSrcDcd);

	// bufferSizeDB needs to be set appropriately
	MP4BitfieldProperty* pBufferSizeProperty = NULL;
	pOdEsd->FindProperty("slConfigDescr.predefined", 
		(MP4Property**)&pBufferSizeProperty);
	ASSERT(pBufferSizeProperty);
	pBufferSizeProperty->SetValue(numBytes);

	// SL config needs to change from 2 (file) to 1 (null)
	pOdEsd->FindProperty("slConfigDescr.predefined", 
		(MP4Property**)&pSetProperty);
	pSetProperty->SetValue(1);


	// Scene
	MP4Descriptor* pSceneEsd =
		pEsProperty->AddDescriptor(MP4ESDescrTag);
	pSceneEsd->Generate();

	pSceneEsd->FindProperty("ESID", 
		(MP4Property**)&pSetProperty);
	pSetProperty->SetValue(sceneTrackId);

	pSceneEsd->FindProperty("URLFlag", 
		(MP4Property**)&pSetProperty);
	pSetProperty->SetValue(1);

	CreateIsmaSceneCommand(
		sceneTrackId, audioTrackId, videoTrackId,
		&pBytes, &numBytes);

	VERBOSE_ISMA(GetVerbosity(),
		printf("Scene data =\n"); MP4HexDump(pBytes, numBytes));

	urlBuf = (char*)MP4Malloc((numBytes * 4 / 3) + 64);
	sprintf(urlBuf, 
		"data:application/mpeg4-bifs-au;base64,%s",
		 MP4ToBase64(pBytes, numBytes));

	pSceneEsd->FindProperty("URL", 
		(MP4Property**)&pUrlProperty);
	pUrlProperty->SetValue(urlBuf);

	VERBOSE_ISMA(GetVerbosity(),
		printf("Scene data URL = \042%s\042\n", urlBuf));

	MP4Free(urlBuf);
	urlBuf = NULL;
	MP4Free(pBytes);
	pBytes = NULL;

	// HACK temporarily point to scene decoder config
	FindProperty(MakeTrackName(sceneTrackId, 
		"mdia.minf.stbl.stsd.mp4s.esds.decConfigDescr"),
		(MP4Property**)&pSrcDcd);
	ASSERT(pSrcDcd);
	MP4Property* pOrgSceneEsdProperty = 
		pSceneEsd->GetProperty(8);
	pSceneEsd->SetProperty(8, pSrcDcd);

	// bufferSizeDB needs to be set
	pBufferSizeProperty = NULL;
	pSceneEsd->FindProperty("slConfigDescr.predefined", 
		(MP4Property**)&pBufferSizeProperty);
	ASSERT(pBufferSizeProperty);
	pBufferSizeProperty->SetValue(numBytes);

	// SL config needs to change from 2 (file) to 1 (null)
	pSceneEsd->FindProperty("slConfigDescr.predefined", 
		(MP4Property**)&pSetProperty);
	pSetProperty->SetValue(1);


	// finally get the whole thing written to a memory 
	pIod->WriteToMemory(this, ppBytes, pNumBytes);


	// now carefully replace esd properties before destroying
	pOdEsd->SetProperty(8, pOrgOdEsdProperty);
	pSceneEsd->SetProperty(8, pOrgSceneEsdProperty);

	delete pIod;

	VERBOSE_ISMA(GetVerbosity(),
		printf("IOD data =\n"); MP4HexDump(*ppBytes, *pNumBytes));
}

void MP4File::CreateIsmaODUpdateCommand(
	MP4TrackId odTrackId,
	MP4TrackId audioTrackId, 
	MP4TrackId videoTrackId,
	bool mp4FileMode,
	u_int8_t** ppBytes,
	u_int64_t* pNumBytes)
{
	MP4Descriptor* pCommand = CreateODCommand(MP4ODUpdateODCommandTag);
	pCommand->Generate();

	MP4Descriptor* pAudioOd = NULL;
	MP4Descriptor* pVideoOd = NULL;
	MP4DescriptorProperty* pOrgAudioEsdProperty = NULL;
	MP4DescriptorProperty* pOrgVideoEsdProperty = NULL;
	MP4DescriptorProperty* pRealAudioEsdProperty = NULL;
	MP4DescriptorProperty* pRealVideoEsdProperty = NULL;

	for (u_int8_t i = 0; i < 2; i++) {
		MP4TrackId trackId;
		u_int16_t odId;

		if (i == 0) {
			trackId = audioTrackId;
			odId = 10;
		} else {
			trackId = videoTrackId;
			odId = 20;
		}

		if (trackId == MP4_INVALID_TRACK_ID) {
			continue;
		}

		u_int32_t mpodIndex = FindTrackReference(
			MakeTrackName(odTrackId, "tref.mpod"), trackId);
		ASSERT(mpodIndex != 0);

		u_int8_t odTag;
		if (mp4FileMode) {
			odTag = MP4FileODescrTag;
		} else {
			odTag = MP4ODescrTag;
		}

		MP4DescriptorProperty* pOdDescrProperty =
				(MP4DescriptorProperty*)(pCommand->GetProperty(0));

		pOdDescrProperty->SetTags(odTag);

		MP4Descriptor* pOd =
			pOdDescrProperty->AddDescriptor(odTag);

		pOd->Generate();

		if (i == 0) {
			pAudioOd = pOd;
		} else {
			pVideoOd = pOd;
		}

		MP4BitfieldProperty* pOdIdProperty = NULL;
		pOd->FindProperty("objectDescriptorId", 
			(MP4Property**)&pOdIdProperty);
		pOdIdProperty->SetValue(odId);

		MP4DescriptorProperty* pEsIdsDescriptorProperty = NULL;
		pOd->FindProperty("esIds", 
			(MP4Property**)&pEsIdsDescriptorProperty);
		ASSERT(pEsIdsDescriptorProperty);

		if (mp4FileMode) {
			pEsIdsDescriptorProperty->SetTags(MP4ESIDRefDescrTag);

			MP4Descriptor *pRefDescriptor =
				pEsIdsDescriptorProperty->AddDescriptor(MP4ESIDRefDescrTag);
			pRefDescriptor->Generate();

			MP4Integer16Property* pRefIndexProperty = NULL;
			pRefDescriptor->FindProperty("refIndex", 
				(MP4Property**)&pRefIndexProperty);
			ASSERT(pRefIndexProperty);

			pRefIndexProperty->SetValue(mpodIndex);

		} else { // stream mode
			pEsIdsDescriptorProperty->SetTags(MP4ESDescrTag);

			MP4Atom* pEsdsAtom = 
				FindAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.*.esds"));
			ASSERT(pEsdsAtom);

			MP4DescriptorProperty* pEsdProperty = 
				(MP4DescriptorProperty*)(pEsdsAtom->GetProperty(2));

			// HACK we temporarily point to the esds 
			if (i == 0) {
				pOrgAudioEsdProperty = pEsIdsDescriptorProperty;
				pRealAudioEsdProperty = pEsdProperty;
			} else {
				pOrgVideoEsdProperty = pEsIdsDescriptorProperty;
				pRealVideoEsdProperty = pEsdProperty;
			}
			pOd->SetProperty(4, pEsdProperty);

			// SL config needs to change from 2 (file) to 1 (null)
			MP4Integer8Property* pSLConfigProperty = NULL;
			pEsdProperty->FindProperty("slConfigDescr.predefined", 
				(MP4Property**)&pSLConfigProperty);
			ASSERT(pSLConfigProperty);
			pSLConfigProperty->SetValue(1);
		}
	}

	pCommand->WriteToMemory(this, ppBytes, pNumBytes);

	// carefully replace esd properties before destroying
	if (pAudioOd) {
		pAudioOd->SetProperty(4, pOrgAudioEsdProperty);

		// SL config needs to go back to 2 (file)
		if (!mp4FileMode) {
			ASSERT(pRealAudioEsdProperty);
			MP4Integer8Property* pSLConfigProperty = NULL;
			pRealAudioEsdProperty->FindProperty("slConfigDescr.predefined", 
				(MP4Property**)&pSLConfigProperty);
			ASSERT(pSLConfigProperty);
			pSLConfigProperty->SetValue(2);
		}
	}

	if (pVideoOd) {
		pVideoOd->SetProperty(4, pOrgVideoEsdProperty);

		// SL config needs to go back to 2 (file)
		if (!mp4FileMode) {
			ASSERT(pRealVideoEsdProperty);
			MP4Integer8Property* pSLConfigProperty = NULL;
			pRealVideoEsdProperty->FindProperty("slConfigDescr.predefined", 
				(MP4Property**)&pSLConfigProperty);
			ASSERT(pSLConfigProperty);
			pSLConfigProperty->SetValue(2);
		}
	}

	delete pCommand;
}

void MP4File::CreateIsmaSceneCommand(
	MP4TrackId sceneTrackId, 
	MP4TrackId audioTrackId, 
	MP4TrackId videoTrackId,
	u_int8_t** ppBytes,
	u_int64_t* pNumBytes)
{
	// from ISMA 1.0 Tech Spec Appendix E
	static u_int8_t bifsAudioOnly[] = {
		0xC0, 0x10, 0x12, 
		0x81, 0x30, 0x2A, 0x05, 0x7C
	};
	static u_int8_t bifsVideoOnly[] = {
		0xC0, 0x10, 0x12, 
		0x61, 0x04, 0x88, 0x50, 0x45, 0x05, 0x3F, 0x00
	};
	static u_int8_t bifsAudioVideo[] = {
		0xC0, 0x10, 0x12, 
		0x81, 0x30, 0x2A, 0x05, 0x72,
		0x61, 0x04, 0x88, 0x50, 0x45, 0x05, 0x3F, 0x00
	};

	if (audioTrackId != MP4_INVALID_TRACK_ID 
	  && videoTrackId != MP4_INVALID_TRACK_ID) {
		*pNumBytes = sizeof(bifsAudioVideo);
		*ppBytes = (u_int8_t*)MP4Malloc(*pNumBytes);
		memcpy(*ppBytes, bifsAudioVideo, sizeof(bifsAudioVideo));

	} else if (audioTrackId != MP4_INVALID_TRACK_ID) {
		*pNumBytes = sizeof(bifsAudioOnly);
		*ppBytes = (u_int8_t*)MP4Malloc(*pNumBytes);
		memcpy(*ppBytes, bifsAudioOnly, sizeof(bifsAudioOnly));

	} else if (videoTrackId != MP4_INVALID_TRACK_ID) {
		*pNumBytes = sizeof(bifsVideoOnly);
		*ppBytes = (u_int8_t*)MP4Malloc(*pNumBytes);
		memcpy(*ppBytes, bifsVideoOnly, sizeof(bifsVideoOnly));
	} else {
		*pNumBytes = 0;
		*ppBytes = NULL;
	}
}	

