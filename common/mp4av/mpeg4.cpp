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
 * Copyright (C) Cisco Systems Inc. 2000-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

/* 
 * Notes:
 *  - file formatted with tabstops == 4 spaces 
 */

#include <mp4av_common.h>

extern "C" uint8_t *MP4AV_Mpeg4FindVosh (uint8_t *pBuf, uint32_t buflen)
{
  while (buflen > 4) {
    if (pBuf[0] == 0x0 &&
	pBuf[1] == 0x0 &&
	pBuf[2] == 0x1 &&
	pBuf[3] == MP4AV_MPEG4_VOSH_START) {
      return pBuf;
    }
    pBuf++;
    buflen--;
  }
  return NULL;
}

extern "C" bool MP4AV_Mpeg4ParseVosh(
				     u_int8_t* pVoshBuf, 
				     u_int32_t voshSize,
				     u_int8_t* pProfileLevel)
{
	CMemoryBitstream vosh;

	vosh.SetBytes(pVoshBuf, voshSize);

	try {
		vosh.GetBits(32);				// start code
		*pProfileLevel = vosh.GetBits(8);
	}
	catch (int e) {
		return false;
	}

	return true;
}

extern "C" bool MP4AV_Mpeg4CreateVosh(
	u_int8_t** ppBytes,
	u_int32_t* pNumBytes,
	u_int8_t profileLevel)
{
	CMemoryBitstream vosh;

	try {
		if (*ppBytes) {
			// caller must guarantee buffer against overrun
			memset((*ppBytes) + (*pNumBytes), 0, 5);
			vosh.SetBytes(*ppBytes, (*pNumBytes) + 5);
			vosh.SetBitPosition((*pNumBytes) << 3);
		} else {
			vosh.AllocBytes(5);
		}

		vosh.PutBits(MP4AV_MPEG4_SYNC, 24);
		vosh.PutBits(MP4AV_MPEG4_VOSH_START, 8);
		vosh.PutBits(profileLevel, 8);

		*ppBytes = vosh.GetBuffer();
		*pNumBytes = vosh.GetNumberOfBytes();
	}
	catch (int e) {
		return false;
	}

	return true;
}

extern "C" bool MP4AV_Mpeg4CreateVo(
	u_int8_t** ppBytes,
	u_int32_t* pNumBytes,
	u_int8_t objectId)
{
	CMemoryBitstream vo;

	try {
		if (*ppBytes) {
			// caller must guarantee buffer against overrun
			memset((*ppBytes) + (*pNumBytes), 0, 9);
			vo.SetBytes(*ppBytes, *pNumBytes + 9);
			vo.SetBitPosition((*pNumBytes) << 3);
		} else {
			vo.AllocBytes(9);
		}

		vo.PutBits(MP4AV_MPEG4_SYNC, 24);
		vo.PutBits(MP4AV_MPEG4_VO_START, 8);
		vo.PutBits(0x08, 8);	// no verid, priority, or signal type
		vo.PutBits(MP4AV_MPEG4_SYNC, 24);
		vo.PutBits(objectId - 1, 8);

		*ppBytes = vo.GetBuffer();
		*pNumBytes = vo.GetNumberOfBytes();
	}
	catch (int e) {
		return false;
	}

	return true;
}

extern "C" uint8_t *MP4AV_Mpeg4FindVol (uint8_t *pBuf, uint32_t buflen)
{
  while (buflen > 4) {
    if (pBuf[0] == 0x0 &&
	pBuf[1] == 0x0 &&
	pBuf[2] == 0x1 &&
	(pBuf[3] & 0xf0) == MP4AV_MPEG4_VOL_START) {
      return pBuf;
    }
    pBuf++;
    buflen--;
  }
  return NULL;
}
extern "C" uint8_t *MP4AV_Mpeg4FindVop (uint8_t *pBuf, uint32_t buflen)
{
  while (buflen > 4) {
    if (pBuf[0] == 0x0 &&
	pBuf[1] == 0x0 &&
	pBuf[2] == 0x1 &&
	pBuf[3] == MP4AV_MPEG4_VOP_START) {
      return pBuf;
    }
    pBuf++;
    buflen--;
  }
  return NULL;
}

extern "C" bool MP4AV_Mpeg4ParseVol(
	u_int8_t* pVolBuf, 
	u_int32_t volSize,
	u_int8_t* pTimeBits, 
	u_int16_t* pTimeTicks, 
	u_int16_t* pFrameDuration, 
	u_int16_t* pFrameWidth, 
	u_int16_t* pFrameHeight,
	u_int8_t * aspectRatioDefine,
	u_int8_t * aspectRatioWidth,
	u_int8_t * aspectRatioHeight)

{
	CMemoryBitstream vol;
	uint8_t aspect;

	vol.SetBytes(pVolBuf, volSize);

	try {
		vol.SkipBits(32);				// start code

		vol.SkipBits(1);				// random accessible vol
		vol.SkipBits(8);				// object type id
		u_int8_t verid = 1;
		if (vol.GetBits(1)) {			// is object layer id
			verid = vol.GetBits(4);			// object layer verid
			vol.SkipBits(3);				// object layer priority
		}
		aspect = vol.GetBits(4);
		if (aspectRatioDefine != NULL)
		  *aspectRatioDefine = aspect;
		if (aspect == 0xF) { 	// aspect ratio info
		  if (aspectRatioWidth != NULL) 
		    *aspectRatioWidth = vol.GetBits(8);
		  else
		    vol.SkipBits(8);				// par width
		  if (aspectRatioHeight != NULL) 
		    *aspectRatioHeight = vol.GetBits(8);
		  else
		    vol.SkipBits(8);				// par height
		}
		if (vol.GetBits(1)) {			// vol control parameters
			vol.SkipBits(2);				// chroma format
			vol.SkipBits(1);				// low delay
			if (vol.GetBits(1)) {			// vbv parameters
				vol.SkipBits(15);				// first half bit rate
				vol.SkipBits(1);				// marker bit
				vol.SkipBits(15);				// latter half bit rate
				vol.SkipBits(1);				// marker bit
				vol.SkipBits(15);				// first half vbv buffer size
				vol.SkipBits(1);				// marker bit
				vol.SkipBits(3);				// latter half vbv buffer size
				vol.SkipBits(11);				// first half vbv occupancy
				vol.SkipBits(1);				// marker bit
				vol.SkipBits(15);				// latter half vbv occupancy
				vol.SkipBits(1);				// marker bit
			}
		}
		u_int8_t shape = vol.GetBits(2); // object layer shape
		if (shape == 3 /* GRAYSCALE */ && verid != 1) {
			vol.SkipBits(4);				// object layer shape extension
		}
		vol.SkipBits(1);				// marker bit
		*pTimeTicks = vol.GetBits(16);		// vop time increment resolution 

		u_int8_t i;
		u_int32_t powerOf2 = 1;
		for (i = 0; i < 16; i++) {
			if (*pTimeTicks < powerOf2) {
				break;
			}
			powerOf2 <<= 1;
		}
		*pTimeBits = i;

		vol.SkipBits(1);				// marker bit
		if (vol.GetBits(1)) {			// fixed vop rate
			// fixed vop time increment
			*pFrameDuration = vol.GetBits(*pTimeBits); 
		} else {
			*pFrameDuration = 0;
		}
		if (shape == 0 /* RECTANGULAR */) {
			vol.SkipBits(1);				// marker bit
			*pFrameWidth = vol.GetBits(13);	// object layer width
			vol.SkipBits(1);				// marker bit
			*pFrameHeight = vol.GetBits(13);// object layer height
			vol.SkipBits(1);				// marker bit
		} else {
			*pFrameWidth = 0;
			*pFrameHeight = 0;
		}
		// there's more, but we don't need it
	}
	catch (int e) {
		return false;
	}

	return true;
}

extern "C" bool MP4AV_Mpeg4CreateVol(
	u_int8_t** ppBytes,
	u_int32_t* pNumBytes,
	u_int8_t profile,
	float frameRate,
	bool shortTime,
	bool variableRate,
	u_int16_t width,
	u_int16_t height,
	u_int8_t quantType,
	u_int8_t* pTimeBits)
{
	CMemoryBitstream vol;

	try {
		if (*ppBytes) {
			// caller must guarantee buffer against overrun
			memset((*ppBytes) + (*pNumBytes), 0, 20);
			vol.SetBytes(*ppBytes, *pNumBytes + 20);
			vol.SetBitPosition((*pNumBytes) << 3);
		} else {
			vol.AllocBytes(20);
		}

		/* VOL - Video Object Layer */
		vol.PutBits(MP4AV_MPEG4_SYNC, 24);
		vol.PutBits(MP4AV_MPEG4_VOL_START, 8);

		/* 1 bit - random access = 0 (1 only if every VOP is an I frame) */
		vol.PutBits(0, 1);
		/*
		 * 8 bits - type indication 
		 * 		= 1 (simple profile)
		 * 		= 4 (main profile)
		 */
		vol.PutBits(profile, 8);
		/* 1 bit - is object layer id = 1 */
		vol.PutBits(1, 1);
		/* 4 bits - visual object layer ver id = 1 */
		vol.PutBits(1, 4); 
		/* 3 bits - visual object layer priority = 1 */
		vol.PutBits(1, 3); 

		/* 4 bits - aspect ratio info = 1 (square pixels) */
		vol.PutBits(1, 4);
		/* 1 bit - VOL control params = 0 */
		vol.PutBits(0, 1);
		/* 2 bits - VOL shape = 0 (rectangular) */
		vol.PutBits(0, 2);
		/* 1 bit - marker = 1 */
		vol.PutBits(1, 1);

		u_int16_t ticks;
		if (shortTime /* && frameRate == (float)((int)frameRate) */) {
			ticks = (u_int16_t)(frameRate + 0.5);
		} else {
			ticks = 30000;
		}
		/* 16 bits - VOP time increment resolution */
		vol.PutBits(ticks, 16);
		/* 1 bit - marker = 1 */
		vol.PutBits(1, 1);

		u_int8_t rangeBits = 1;
		while (ticks > (1 << rangeBits)) {
			rangeBits++;
		}
		if (pTimeBits) {
			*pTimeBits = rangeBits;
		}

		/* 1 bit - fixed vop rate = 0 or 1 */
		if (variableRate) {
			vol.PutBits(0, 1);
		} else {
			vol.PutBits(1, 1);

			u_int16_t frameDuration = 
				(u_int16_t)((float)ticks / frameRate);

			/* 1-16 bits - fixed vop time increment in ticks */
			vol.PutBits(frameDuration, rangeBits);
		}
		/* 1 bit - marker = 1 */
		vol.PutBits(1, 1);
		/* 13 bits - VOL width */
		vol.PutBits(width, 13);
		/* 1 bit - marker = 1 */
		vol.PutBits(1, 1);
		/* 13 bits - VOL height */
		vol.PutBits(height, 13);
		/* 1 bit - marker = 1 */
		vol.PutBits(1, 1);
		/* 1 bit - interlaced = 0 */
		vol.PutBits(0, 1);

		/* 1 bit - overlapped block motion compensation disable = 1 */
		vol.PutBits(1, 1);
#if 0
		/* 2 bits - sprite usage = 0 */
		vol.PutBits(0, 2);
#else
		vol.PutBits(0, 1);
#endif
		/* 1 bit - not 8 bit pixels = 0 */
		vol.PutBits(0, 1);
		/* 1 bit - quant type = 0 */
		vol.PutBits(quantType, 1);
		if (quantType) {
			/* 1 bit - load intra quant mat = 0 */
			vol.PutBits(0, 1);
			/* 1 bit - load inter quant mat = 0 */
			vol.PutBits(0, 1);
		}
#if 0
		/* 1 bit - quarter pixel = 0 */
		vol.PutBits(0, 1);
#endif
		/* 1 bit - complexity estimation disable = 1 */
		vol.PutBits(1, 1);
		/* 1 bit - resync marker disable = 1 */
		vol.PutBits(1, 1);
		/* 1 bit - data partitioned = 0 */
		vol.PutBits(0, 1);
#if 0
		/* 1 bit - newpred = 0 */
		vol.PutBits(0, 1);
		/* 1 bit - reduced resolution vop = 0 */
		vol.PutBits(0, 1);
#endif
		/* 1 bit - scalability = 0 */
		vol.PutBits(0, 1);

		/* pad to byte boundary with 0 then as many 1's as needed */
		vol.PutBits(0, 1);
		if ((vol.GetBitPosition() & 7) != 0) {
			vol.PutBits(0xFF, 8 - (vol.GetBitPosition() & 7));
		}

		*ppBytes = vol.GetBuffer();
		*pNumBytes = vol.GetBitPosition() >> 3;
	}
	catch (int e) {
		return false;
	}

	return true;
}

extern "C" bool MP4AV_Mpeg4ParseGov(
	u_int8_t* pGovBuf, 
	u_int32_t govSize,
	u_int8_t* pHours, 
	u_int8_t* pMinutes, 
	u_int8_t* pSeconds)
{
	CMemoryBitstream gov;

	gov.SetBytes(pGovBuf, govSize);

	try {
		gov.SkipBits(32);	// start code
		*pHours = gov.GetBits(5);
		*pMinutes = gov.GetBits(6);
		gov.SkipBits(1);		// marker bit
		*pSeconds = gov.GetBits(6);
	}
	catch (int e) {
		return false;
	}

	return true;
}

static bool Mpeg4ParseShortHeaderVop(
	u_int8_t* pVopBuf, 
	u_int32_t vopSize,
	u_char* pVopType)
{
	CMemoryBitstream vop;

	vop.SetBytes(pVopBuf, vopSize);

	try {
		// skip start code, temporal ref, and into type
		vop.SkipBits(22 + 8 + 5 + 3);	
		if (vop.GetBits(1) == 0) {
			*pVopType = 'I';
		} else {
			*pVopType = 'P';
		}
	}
	catch (int e) {
		return false;
	}

	return true;
}

extern "C" bool MP4AV_Mpeg4ParseVop(
	u_int8_t* pVopBuf, 
	u_int32_t vopSize,
	u_char* pVopType, 
	u_int8_t timeBits, 
	u_int16_t timeTicks, 
	u_int32_t* pVopTimeIncrement)
{
	CMemoryBitstream vop;

	vop.SetBytes(pVopBuf, vopSize);

	try {
		vop.SkipBits(32);	// skip start code

		switch (vop.GetBits(2)) {
		case 0:
			/* Intra */
			*pVopType = 'I';
			break;
		case 1:
			/* Predictive */
			*pVopType = 'P';
			break;
		case 2:
			/* Bidirectional Predictive */
			*pVopType = 'B';
			break;
		case 3:
			/* Sprite */
			*pVopType = 'S';
			break;
		}

		if (!pVopTimeIncrement) {
			return true;
		}

		u_int8_t numSecs = 0;
		while (vop.GetBits(1) != 0) {
			numSecs++;
		}
		vop.SkipBits(1);		// skip marker
		u_int16_t numTicks = vop.GetBits(timeBits);
		*pVopTimeIncrement = (numSecs * timeTicks) + numTicks; 
	}
	catch (int e) {
		return false;
	}

	return true;
}


extern "C" u_char MP4AV_Mpeg4GetVopType(u_int8_t* pVopBuf, u_int32_t vopSize)
{
	u_char vopType = 0;

	if (vopSize <= 4) {
		return vopType;
	}

	if (pVopBuf[0] == 0 && pVopBuf[1] == 0 
	  && (pVopBuf[2] & 0xFC) == 0x08 && (pVopBuf[3] & 0x03) == 0x02) {
		// H.263, (MPEG-4 short header mode)
		Mpeg4ParseShortHeaderVop(pVopBuf, vopSize, &vopType);
		
	} else {
		// MPEG-4 (normal mode)
		MP4AV_Mpeg4ParseVop(pVopBuf, vopSize, &vopType, 0, 0, NULL);
	}

	return vopType;
}

