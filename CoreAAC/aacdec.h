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

#include "..\\include\\faad.h"
#include "resource.h"

#define FILTER_VERSION "1.0b"

// ===========================================================================================

// {6AC7C19E-8CA0-4e3d-9A9F-2881DE29E0AC}
DEFINE_GUID(CLSID_DECODER, 0x6ac7c19e, 0x8ca0, 0x4e3d, 0x9a, 0x9f, 0x28, 0x81, 0xde, 0x29, 0xe0, 0xac);

class CDecoder : public CTransformFilter, public ISpecifyPropertyPages
{
public :
	DECLARE_IUNKNOWN
	static CUnknown *WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);    

	CDecoder(LPUNKNOWN lpunk, HRESULT *phr) :
		CTransformFilter(NAME("AAC Audio Decoder"), lpunk, CLSID_DECODER),
		m_decHandle(NULL),
		m_decoderSpecificLen(0),
		m_decoderSpecific(NULL)
		{ }

	virtual ~CDecoder();

	// property pages
	STDMETHODIMP GetPages(CAUUID *pPages);
	STDMETHODIMP FreePages(CAUUID *pPages);
 
	// ITransformFilter interface methods
	HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
	HRESULT CheckInputType(const CMediaType *mtIn);
	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	HRESULT DecideBufferSize(IMemAllocator *pAllocator, ALLOCATOR_PROPERTIES *pprop);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

	HRESULT CompleteConnect(PIN_DIRECTION direction, IPin *pReceivePin);

	bool Decode(BYTE *pSrc, DWORD SrcLength, BYTE *pDst, DWORD DstLength, DWORD *ActualDstLength);

private:
	unsigned char* m_decoderSpecific;
	int m_decoderSpecificLen;
	faacDecHandle m_decHandle;
	int m_Channels;
	int m_SamplesPerSec;
	int m_BitesPerSample;
};

// ===========================================================================================
// simple about page

// {4665E44B-8B9A-4515-A086-E94ECE374608}
DEFINE_GUID(CLSID_ABOUT, 0x4665e44b, 0x8b9a, 0x4515, 0xa0, 0x86, 0xe9, 0x4e, 0xce, 0x37, 0x46, 0x8);

class CAbout : public CBasePropertyPage
{
public:
	static CUnknown *WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr)
	{
	    CAbout *pNewObject = new CAbout(punk, phr);
	    if (!pNewObject)
	        *phr = E_OUTOFMEMORY;
	    return pNewObject;
	}

	CAbout(LPUNKNOWN pUnk, HRESULT *phr) :
		CBasePropertyPage(NAME("About"), pUnk, IDD_ABOUT, IDS_ABOUT)
		{ }

	~CAbout()
		{ }

	HRESULT OnActivate()
	{		
		SetDlgItemText(m_hwnd,IDC_STATIC_VERSION,"Version " FILTER_VERSION " - ("__DATE__", "__TIME__")");
		return S_OK;
	}

	BOOL OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);		
	}
};

// ===========================================================================================
