//-----------------------------------------------------------------------------
//
// File:	QCDModEncode.h
//
// About:	Encode plugin module interface.  This file is published with the 
//			Encode plugin SDK.
//
// Authors:	Written by Paul Quinn
//
// Copyright:
//
//	QCD multimedia player application Software Development Kit Release 1.0.
//
//	Copyright (C) 1997-2002 Quinnware
//
//	This code is free.  If you redistribute it in any form, leave this notice 
//	here.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//-----------------------------------------------------------------------------

#ifndef QCDMODENCODE_H
#define QCDMODENCODE_H

#include "QCDModDefs.h"

// name of the DLL export for encode plugins
#define ENCODEDLL_ENTRY_POINT	QEncodeModule

// Stop will receive one of these flags
#define STOPFLAG_FORCESTOP		0
#define STOPFLAG_PLAYDONE		1

// pause flags
#define PAUSE_DISABLED			0	// Pause() call is to unpause playback
#define PAUSE_ENABLED			1	// Pause() call is to pause playback

//-----------------------------------------------------------------------------

typedef struct 
{
	UINT				size;			// size of init structure
	UINT				version;		// plugin structure version (set to PLUGIN_API_VERSION)
	PluginServiceFunc	Service;		// player supplied services callback

	struct
	{
		void *dummy;
		void (*PositionUpdate)(UINT marker);
		void *Reserved[2];
	} toPlayer;

	struct 
	{
		BOOL (*Open)(LPCSTR, WAVEFORMATEX *wf);
		BOOL (*Write)(WriteDataStruct*);
		BOOL (*Flush)(UINT marker);
		BOOL (*Stop)(int flags);
		BOOL (*Pause)(int flags);
		BOOL (*Drain)(int flags);
		void (*ShutDown)(int flags);

		void (*Configure)(int flags);
		void (*About)(int flags);

		BOOL (*SetVolume)(int levelleft, int levelright, int flags);
		BOOL (*GetCurrentPosition)(UINT *position, int flags);
		BOOL (*DrainCancel)(int flags);
		void *Reserved[8];
	} toModule;
} QCDModInitEnc;

#endif //QCDMODENCODE_H