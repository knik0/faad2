//-----------------------------------------------------------------------------
// 
// File:	QCDEncodeDLL.h
//
// About:	QCD Player Output module DLL interface.  For more documentation, see
//			QCDModOutput.h.
//
// Authors:	Written by Paul Quinn
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

#include "QCDModEncode.h"

#ifndef QCDENCODEDLL_H
#define QCDENCODEDLL_H

extern QCDModInitEnc	*QCDCallbacks_Enc;

// Calls from the Player
void ShutDown_Enc(int flags);
BOOL Open_Enc(LPCSTR, WAVEFORMATEX *wf);
BOOL Write_Enc(WriteDataStruct*);
BOOL Stop_Enc(int flags);
BOOL GetCurrentPosition_Enc(UINT *position, int flags);

void Configure_Enc(int flags);
void About_Enc(int flags);

#endif //QCDOUTPUTDLL_H