//-----------------------------------------------------------------------------
//
// File:	QCDEncodeDLL.cpp
//
// About:	See QCDOutputDLL.h
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

#include <windows.h>
#include <shlobj.h>
#include <mp4.h>

#include "QCDInputDLL.h"
#include "QCDConvertDLL.h"
#include "resource.h"
#include "utils.h"
#include "config.h"
#include "aac2mp4.h"

HWND			hwndConfigEnc;
QCDModInitEnc	*QCDCallbacks_Enc;

BOOL CALLBACK config_convert_proc(HWND hwndDlg, UINT message,
                                 WPARAM wParam, LPARAM lParam);

static char m_output_folder[MAX_PATH];// our output folder

void config_enc_read()
{
	char output_folder[MAX_PATH];
	output_folder[0] = 0;

    RS(output_folder);

	lstrcpy(m_output_folder, output_folder);
}

void config_enc_write()
{
	char output_folder[MAX_PATH];
	lstrcpy(output_folder, m_output_folder);

	WS(output_folder);
}

BOOL isFolder(char *folder)
{
	if(lstrlen(folder) > 0 && folder[1] == ':')
		return TRUE;
	else
		return FALSE;
}

//-----------------------------------------------------------------------------

PLUGIN_API BOOL ENCODEDLL_ENTRY_POINT(QCDModInitEnc *ModInit, QCDModInfo *ModInfo)
{
	ModInit->version = PLUGIN_API_VERSION;
	ModInfo->moduleString = "AAC to Mp4 Converter v1.0";

	ModInit->toModule.ShutDown				= ShutDown_Enc;
	ModInit->toModule.Open					= Open_Enc;
	ModInit->toModule.Write					= Write_Enc;
	ModInit->toModule.Stop					= Stop_Enc;
	ModInit->toModule.GetCurrentPosition	= GetCurrentPosition_Enc;
	ModInit->toModule.Configure				= Configure_Enc;
	ModInit->toModule.About					= About_Enc;

/* encoders can handle these calls if they wish,
 * but they are generally not needed

	ModInit->toModule.Flush				= Flush;
	ModInit->toModule.Pause				= Pause;
	ModInit->toModule.Drain				= Drain;
	ModInit->toModule.SetVolume			= SetVolume;
	ModInit->toModule.DrainCancel		= DrainCancel;

*/

	QCDCallbacks_Enc = ModInit;

	hwndPlayer = (HWND)QCDCallbacks_Enc->Service(opGetParentWnd, 0, 0, 0);

	QCDCallbacks_Enc->Service(opGetPluginSettingsFile, INI_FILE, MAX_PATH, 0);

	//
	// TODO: all your plugin initialization here
	//
	config_enc_read();

	
	// return TRUE for successful initialization
	return TRUE;
}

//-----------------------------------------------------------------------------

void ShutDown_Enc(int flags)
{
	Stop_Enc(STOPFLAG_FORCESTOP);
}

//-----------------------------------------------------------------------------

BOOL Open_Enc(LPCSTR medianame, WAVEFORMATEX *wf)
{
	char mp4FileName[256];
	
	if(StringComp(strrchr(medianame, '.'), ".aac", 4))
	{
		MessageBox(hwndPlayer, "Only AAC files can be converted into Mp4 Files!", "File Type Error", MB_OK);
		return FALSE;
	}
	else if(!isFolder(m_output_folder) && MessageBox(hwndPlayer, "You should select a output folder!", "Error", MB_OK) == IDOK )
	{
		if(!IsWindow(hwndConfigEnc))
			hwndConfigEnc = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_CONFIG_CONVERT), 
				hwndPlayer, config_convert_proc);
		ShowWindow(hwndConfigEnc, SW_SHOW);
		return FALSE;
	}
	else
	{
		lstrcpy(mp4FileName, m_output_folder); // copy the folder path
		lstrcat(mp4FileName, strrchr(medianame, '\\')); //copy the file name
		lstrcpy(strrchr(mp4FileName, '.'), ".mp4"); // rename the file name to .mp4

		if(covert_aac_to_mp4(medianame, mp4FileName))
		{
			MessageBox(hwndPlayer, "An error occured while converting AAC to MP4!", "An error occured!", MB_OK);
			return FALSE;
		}
		else
		{
			QCDCallbacks_Enc->Service(opSetStatusMessage, "Converting OK", TEXT_TOOLTIP, 0);

			return TRUE;
		}
	}
}

//-----------------------------------------------------------------------------

BOOL Write_Enc(WriteDataStruct* writeData)
{
	//
	// TODO : Use the raw wave audio data passed from the player.
	//
	// Return value - one of the follwing:
	//
	// TRUE		- write succeeded
	// FALSE	- write failed
	//
	// Note: this call can block (eg: to wait for available space to write)

	return TRUE;
}

//-----------------------------------------------------------------------------

BOOL Stop_Enc(int flags)
{
	//
	// TODO : Stop and close the output.
	//
	// Return TRUE for success, FALSE for failure

	return TRUE;
}

//-----------------------------------------------------------------------------

BOOL GetCurrentPosition_Enc(UINT *position, int flags)
{
	// 
	// TODO : set position to exact current playing position
	// returned position needs to be latest marker sent to Write
	// return TRUE for success

	return TRUE;
}

//-----------------------------------------------------------------------------

BOOL CALLBACK config_convert_proc(HWND hwndDlg, UINT message,
                                 WPARAM wParam, LPARAM lParam)
{
	char str_buffer[MAX_PATH];

	switch (message) 
	{
    case WM_INITDIALOG:
		if(isFolder(m_output_folder))// is a driver folder
			SetDlgItemText(hwndDlg, IDC_OUTPUTFOLDER, m_output_folder);
		else
			SetDlgItemText(hwndDlg, IDC_OUTPUTFOLDER, "Select Output Folder");
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
		{
		case IDC_OUTPUTFOLDER:
			{
				char name[MAX_PATH];
				BROWSEINFO bi;
				ITEMIDLIST *idlist;
				bi.hwndOwner = hwndDlg;
				bi.pidlRoot = 0;
				bi.pszDisplayName = name;
				bi.lpszTitle = "Select a directory for saving mp4 files converted from aac files: ";
				bi.ulFlags = BIF_RETURNONLYFSDIRS /*| BIF_USENEWUI*/;
				bi.lpfn = NULL;
				bi.lParam = 0;
				
				idlist = SHBrowseForFolder( &bi );
				if(idlist)
				{
					SHGetPathFromIDList( idlist, m_output_folder);
					SetDlgItemText(hwndDlg, IDC_OUTPUTFOLDER, m_output_folder);
				}
				return TRUE;
			}
        case IDOK:
			GetDlgItemText(hwndDlg, IDC_OUTPUTFOLDER, str_buffer, MAX_PATH);
			if(isFolder(str_buffer)) // is a driver foder
				lstrcpy(m_output_folder, str_buffer);
			else
				m_output_folder[0] = 0;

            /* save config */
            config_enc_write();
		default:
			/* close the dialogbox */
            DestroyWindow(hwndDlg);
            return TRUE;
        }
    }
    return FALSE;
}

void Configure_Enc(int flags)
{
	if(!IsWindow(hwndConfigEnc))
		hwndConfigEnc = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_CONFIG_CONVERT), 
			hwndPlayer, config_convert_proc);
	ShowWindow(hwndConfigEnc, SW_SHOWNORMAL);
}

//-----------------------------------------------------------------------------

void About_Enc(int flags)
{
	if(!IsWindow(hwndAbout))
		hwndAbout = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_ABOUT), 
			hwndPlayer, about_dialog_proc);
	ShowWindow(hwndAbout, SW_SHOWNORMAL);
}
