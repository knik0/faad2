/* 
 * CoreAAC - AAC DirectShow Decoder Filter
 *
 * Modification to decode AAC without ADTS and multichannel support
 * christophe.paris@free.fr
 */
 
/*
 * AAC DirectShow Decoder Filter
 * Copyright (C) 2003 Robert Cioch
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */
 

#include <windows.h>
#include <streams.h>
#include <initguid.h>
#include <olectl.h>
#include <dvdmedia.h>
#include <transfrm.h>

#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>

#include "aacdec.h"

// ===========================================================================================
#include <stdio.h>

#if defined(_DEBUG)
	void DBGOUT(char *s, ...)
	{
		char b[1024];
		va_list args;

		va_start(args, s);
		vsprintf(b, s, args);
		va_end(args);
		OutputDebugString(b);
	}
#else
	#define DBGOUT
#endif

// ===========================================================================================
/*
// As for today there is no Dolby's AAC waveformat tag registered at Microsoft
// I have choosen 0xAAC0 because it looks nice, but when Dolby registers this format
// then we may get in trouble since on 99.9% it will _NOT_ be 0xAAC0

#define WAVE_FORMAT_AAC 0xAAC0

// {0000aac0-0000-0010-8000-00AA00389B71}
DEFINE_GUID(MEDIASUBTYPE_AAC,
	WAVE_FORMAT_AAC, 0x000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
*/

// Be compatible with 3ivx
#define WAVE_FORMAT_AAC 0x00FF

// {000000FF-0000-0010-8000-00AA00389B71}
DEFINE_GUID(MEDIASUBTYPE_AAC,
	WAVE_FORMAT_AAC, 0x000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
	

// ===========================================================================================
//  Registration setup stuff

AMOVIESETUP_MEDIATYPE sudInputType[] =
{
	{ &MEDIATYPE_Audio, &MEDIASUBTYPE_AAC }
};

AMOVIESETUP_MEDIATYPE sudOutputType[] =
{
	{ &MEDIATYPE_Audio, &MEDIASUBTYPE_PCM }
};

AMOVIESETUP_PIN sudPins[] =
{
	{ L"Input",
		FALSE,							// bRendered
		FALSE,							// bOutput
		FALSE,							// bZero
		FALSE,							// bMany
		&CLSID_NULL,					// clsConnectsToFilter
		NULL,							// ConnectsToPin
		NUMELMS(sudInputType),			// Number of media types
		sudInputType
	},
	{ L"Output",
		FALSE,							// bRendered
		TRUE,							// bOutput
		FALSE,							// bZero
		FALSE,							// bMany
		&CLSID_NULL,					// clsConnectsToFilter
		NULL,							// ConnectsToPin
		NUMELMS(sudOutputType),			// Number of media types
		sudOutputType
	}
};

AMOVIESETUP_FILTER sudDecoder =
{
	&CLSID_DECODER,
	L"CoreAAC Audio Decoder",
	MERIT_PREFERRED,
	NUMELMS(sudPins),
	sudPins
};

// ===========================================================================================
// COM Global table of objects in this dll

CFactoryTemplate g_Templates[] = 
{
  { L"CoreAAC Audio Decoder", &CLSID_DECODER, CDecoder::CreateInstance, NULL, &sudDecoder },
  { L"CoreAAC Audio Decoder About", &CLSID_ABOUT, CAbout::CreateInstance}
};

// Count of objects listed in g_cTemplates
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

// ===========================================================================================

STDAPI DllRegisterServer()
{
	DBGOUT("RegisterServer\n");
	return AMovieDllRegisterServer2(TRUE);
}

// -------------------------------------------------------------------------------------------

STDAPI DllUnregisterServer()
{
	DBGOUT("UnregisterServer\n");
	return AMovieDllRegisterServer2(FALSE);
}

// -------------------------------------------------------------------------------------------

// The streams.h DLL entrypoint.
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

// The entrypoint required by the MSVC runtimes. This is used instead
// of DllEntryPoint directly to ensure global C++ classes get initialised.
BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved) {
	
    return DllEntryPoint(reinterpret_cast<HINSTANCE>(hDllHandle), dwReason, lpreserved);
}

// -------------------------------------------------------------------------------------------

CUnknown *WINAPI CDecoder :: CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	DBGOUT("CreateInstance\n");
	CDecoder *pNewObject = new CDecoder(punk, phr);
	if (!pNewObject)
		*phr = E_OUTOFMEMORY;
	return pNewObject;
}

// -------------------------------------------------------------------------------------------

CDecoder :: ~CDecoder()
{
	if(m_decHandle)
	{
		faacDecClose(m_decHandle);
		m_decHandle = NULL;
	}
	if(m_decoderSpecific)
	{
		delete m_decoderSpecific;
		m_decoderSpecific = NULL;
	}
}

// -------------------------------------------------------------------------------------------

STDMETHODIMP CDecoder :: NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_ISpecifyPropertyPages)
		return GetInterface((ISpecifyPropertyPages *)this, ppv);

	return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
}

// -------------------------------------------------------------------------------------------
// property pages

STDMETHODIMP CDecoder :: GetPages(CAUUID *pPages)
{
	DBGOUT("GetPages\n");

	pPages->cElems = 1;
	pPages->pElems = (GUID *)CoTaskMemAlloc(pPages->cElems * sizeof(GUID));
	if (!pPages->pElems)
		return E_OUTOFMEMORY;

	pPages->pElems[0] = CLSID_ABOUT;

	return S_OK;
}
 
// -------------------------------------------------------------------------------------------

STDMETHODIMP CDecoder :: FreePages(CAUUID *pPages)
{
	DBGOUT("FreePages\n");

	CoTaskMemFree(pPages->pElems);
	return S_OK;
}

// ===========================================================================================
// accept only aac audio wrapped in waveformat

HRESULT CDecoder :: CheckInputType(const CMediaType *mtIn)
{
	DBGOUT("CheckInputType\n");

	if (*mtIn->Type() != MEDIATYPE_Audio || *mtIn->Subtype() != MEDIASUBTYPE_AAC)
		return VFW_E_TYPE_NOT_ACCEPTED;

	if (*mtIn->FormatType() != FORMAT_WaveFormatEx)
		return VFW_E_TYPE_NOT_ACCEPTED;

	if (mtIn->IsTemporalCompressed())
		return VFW_E_TYPE_NOT_ACCEPTED;

	WAVEFORMATEX *wfex = (WAVEFORMATEX *)mtIn->Format();
	if (wfex->wFormatTag != WAVE_FORMAT_AAC)
		return VFW_E_TYPE_NOT_ACCEPTED;

	if(wfex->cbSize < 2)
		return VFW_E_TYPE_NOT_ACCEPTED;

	m_decoderSpecificLen = wfex->cbSize;
	if(m_decoderSpecific)
	{
		delete m_decoderSpecific;
		m_decoderSpecific = NULL;
	}
	m_decoderSpecific = new unsigned char[m_decoderSpecificLen];
	
	// Keep decoderSpecific initialization data (appended to the WAVEFORMATEX struct)
	memcpy(m_decoderSpecific,(char*)wfex+sizeof(WAVEFORMATEX), m_decoderSpecificLen);
	
	return S_OK;
}

// ===========================================================================================
// propose proper waveformat

HRESULT CDecoder :: GetMediaType(int iPosition, CMediaType *mtOut)
{
	DBGOUT("GetMediaType\n");

	if (!m_pInput->IsConnected())
		return E_UNEXPECTED;

	if (iPosition < 0)
		return E_INVALIDARG;

	if (iPosition > 0)
		return VFW_S_NO_MORE_ITEMS;


	WAVEFORMATEXTENSIBLE wfex;
	ZeroMemory(&wfex, sizeof(WAVEFORMATEXTENSIBLE));
	
	wfex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	wfex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE);
	wfex.Format.nChannels = (unsigned short)m_Channels;
	wfex.Format.nSamplesPerSec = (unsigned short)m_SamplesPerSec;
	wfex.Format.wBitsPerSample = (unsigned short)m_BitesPerSample;
	wfex.Format.nBlockAlign = (unsigned short)((wfex.Format.nChannels * wfex.Format.wBitsPerSample) / 8);
	wfex.Format.nAvgBytesPerSec = wfex.Format.nSamplesPerSec * wfex.Format.nBlockAlign;
	switch(m_Channels)
	{
	case 1:
		wfex.dwChannelMask = KSAUDIO_SPEAKER_MONO;		
		break;
	case 2:
		wfex.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
		break;
	case 6:
		wfex.dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
		break;
	default:
		break;
	}
	wfex.Samples.wValidBitsPerSample = wfex.Format.wBitsPerSample;	
	wfex.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
	
	mtOut->SetType(&MEDIATYPE_Audio);
	mtOut->SetSubtype(&MEDIASUBTYPE_PCM);
	mtOut->SetFormatType(&FORMAT_WaveFormatEx);
	mtOut->SetFormat( (BYTE*) &wfex,sizeof(WAVEFORMATEXTENSIBLE));
	mtOut->SetTemporalCompression(FALSE);
	
	return S_OK; 
}

// -------------------------------------------------------------------------------------------

HRESULT CDecoder :: CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	DBGOUT("CheckTransform\n");

	if (*mtOut->FormatType() != FORMAT_WaveFormatEx)
		return VFW_E_INVALIDMEDIATYPE;

	if (mtOut->FormatLength() < sizeof(WAVEFORMATEX))
		return VFW_E_INVALIDMEDIATYPE;

	if (((WAVEFORMATEX *)mtIn->Format())->nSamplesPerSec != ((WAVEFORMATEX *)mtOut->Format())->nSamplesPerSec)
		return VFW_E_TYPE_NOT_ACCEPTED;

	// should we compare whole waveformat here???
	return S_OK; 
}

// -------------------------------------------------------------------------------------------
#define MAXFRAMELEN 1024		// 960 for LD or else 1024 

HRESULT CDecoder :: DecideBufferSize(IMemAllocator *pAllocator, ALLOCATOR_PROPERTIES *pProperties)
{
	DBGOUT("DecideBufferSize\n");
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = m_Channels * MAXFRAMELEN * 2;
	
	DBGOUT("CDecoder::DecideBufferSize %d", pProperties->cbBuffer);

	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pAllocator->SetProperties(pProperties, &Actual);
	if(FAILED(hr))
		return hr;

	if (Actual.cbBuffer < pProperties->cbBuffer || Actual.cBuffers < pProperties->cBuffers)
		return E_INVALIDARG;

	return S_OK;
}

// -------------------------------------------------------------------------------------------

HRESULT CDecoder :: CompleteConnect(PIN_DIRECTION direction, IPin *pReceivePin)
{
	DBGOUT("CompleteConnect\n");	
	HRESULT hr = CTransformFilter::CompleteConnect(direction, pReceivePin);
	
	if(direction == PINDIR_INPUT)
	{
		if(m_decHandle)
		{
			faacDecClose(m_decHandle);
			m_decHandle = NULL;
		}
		m_decHandle = faacDecOpen();

		// Initialize the decoder
		unsigned long SamplesPerSec = 0;
		unsigned char Channels = 0;
		if(faacDecInit2(m_decHandle, m_decoderSpecific, m_decoderSpecificLen,
			&SamplesPerSec, &Channels) < 0)
		{
			return E_FAIL;
		}

		// WAVEFORMATEX used for output mediatype
		m_Channels = Channels;
		m_SamplesPerSec = SamplesPerSec;
		m_BitesPerSample = 16; // XXX : can it be different ???
	}

	return hr;
}

// -------------------------------------------------------------------------------------------

HRESULT CDecoder :: Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	DBGOUT("Transform\n");

	if (m_State == State_Stopped)
	{	
		pOut->SetActualDataLength(0);
		return S_OK;
	}

	// Decode the sample data
	DWORD ActualDstLength;
	BYTE *pSrc, *pDst;
	DWORD SrcLength = pIn->GetActualDataLength();
	DWORD DstLength = pOut->GetSize();
	pIn->GetPointer(&pSrc);
	pOut->GetPointer(&pDst);    	

	// Decode data
	if(!Decode(pSrc, SrcLength, pDst, DstLength, &ActualDstLength))
		return S_FALSE;

	DBGOUT("Transform: %u->%u (%u)\n", SrcLength, ActualDstLength, DstLength);

	// Copy the actual data length
	pOut->SetActualDataLength(ActualDstLength);
	return S_OK;
}

// ===========================================================================================

bool CDecoder :: Decode(BYTE *pSrc, DWORD SrcLength, BYTE *pDst, DWORD DstLength, DWORD *ActualDstLength)
{
	faacDecFrameInfo frameInfo;
	short *outsamples = (short *)faacDecDecode(m_decHandle, &frameInfo, pSrc, DstLength);

	if (frameInfo.error)
	{
		DBGOUT("AAC: Error %d [%s]\n", frameInfo.error, faacDecGetErrorMessage(frameInfo.error));
		return false;
	}

	if (!frameInfo.error && outsamples)
		memcpy(pDst, outsamples, frameInfo.samples * 2);
	else
		return false;

	*ActualDstLength = frameInfo.samples * 2;
	return true;
}

// ===========================================================================================
