/*
** FAAD - Freeware Advanced Audio Decoder
** Copyright (C) 2002 M. Bakker
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
** $Id: QCDMp4.c,v 1.1 2003/04/28 19:07:57 menno Exp $
**/

//#define DEBUG_OUTPUT
#include "QCDInputDLL.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <stdlib.h>
#include <math.h>
#include <faad.h>
#include <mp4.h>

#include "resource.h"
#include "utils.h"
#include "config.h"
#include "aacinfo.h"

static long priority_table[] = {
    0,
    THREAD_PRIORITY_HIGHEST,
    THREAD_PRIORITY_ABOVE_NORMAL,
    THREAD_PRIORITY_NORMAL,
    THREAD_PRIORITY_BELOW_NORMAL,
    THREAD_PRIORITY_LOWEST
};
static int res_id_table[] = {
    IDC_16BITS,
    IDC_24BITS,
    IDC_32BITS,
    0,
    IDC_16BITS_DITHERED
};
static int res_table[] = {
    16,
    24,
    32,
    0,
    16
};

typedef struct state
{
    /* general stuff */
    faacDecHandle hDecoder;
    int samplerate;
    unsigned char channels;
    int decode_pos_ms; // current decoding position, in milliseconds
    int paused; // are we paused?
    int seek_needed; // if != -1, it is the point that the decode thread should seek to, in ms.
    char filename[_MAX_PATH];
    int filetype; /* 0: MP4; 1: AAC */
    int last_frame;

    /* MP4 stuff */
    MP4FileHandle mp4file;
    int mp4track;
    MP4SampleId numSamples;
    MP4SampleId sampleId;

    /* AAC stuff */
    FILE *aacfile;
    long filesize;
    long bytes_read;
    long bytes_into_buffer;
    long bytes_consumed;
    unsigned char *buffer;
    long seconds;
//    faadAACInfo aacInfo;
} state;

static state mp4state;

HINSTANCE		hInstance;
HWND			hwndPlayer, hwndConfig, hwndAbout;
QCDModInitIn	sQCDCallbacks, *QCDCallbacks;
BOOL			oldAPIs = 0;

static int killPlayThread;
HANDLE play_thread_handle = INVALID_HANDLE_VALUE; // the handle to the decode thread

/* Function definitions */
DWORD WINAPI MP4PlayThread(void *b); // the decode thread procedure
DWORD WINAPI AACPlayThread(void *b); // the decode thread procedure

#ifdef DEBUG_OUTPUT
void in_mp4_DebugOutput(char *message)
{
    char s[1024];

    sprintf(s, "%s: %s", mp4state.filename, message);
    MessageBox(NULL, s, "Debug Message", MB_OK);
}
#endif

static void show_error(HWND hwnd, char *message, ...)
{
    if (m_show_errors)
        MessageBox(hwnd, message, "Error", MB_OK);
}

void config_read()
{
    char priority[10];
    char resolution[10];
    char show_errors[10];
    char use_for_aac[10];

    strcpy(show_errors, "1");
    strcpy(priority, "3");
    strcpy(resolution, "0");
    strcpy(use_for_aac, "1");

    RS(priority);
	RS(resolution);
	RS(show_errors);
    RS(use_for_aac);

    m_priority = atoi(priority);
    m_resolution = atoi(resolution);
    m_show_errors = atoi(show_errors);
    m_use_for_aac = atoi(use_for_aac);
}

void config_write()
{
    char priority[10];
    char resolution[10];
    char show_errors[10];
    char use_for_aac[10];

    itoa(m_priority, priority, 10);
    itoa(m_resolution, resolution, 10);
    itoa(m_show_errors, show_errors, 10);
    itoa(m_use_for_aac, use_for_aac, 10);

    WS(priority);
	WS(resolution);
	WS(show_errors);
	WS(use_for_aac);
}

//-----------------------------------------------------------------------------

int WINAPI DllMain(HINSTANCE hInst, DWORD fdwReason, LPVOID pRes)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
		hInstance = hInst;
	return TRUE;
}

//-----------------------------------------------------------------------------
//old entrypoint api
PLUGIN_API BOOL QInputModule(QCDModInitIn *ModInit, QCDModInfo *ModInfo)
{
	ModInit->version					= PLUGIN_API_VERSION;
	ModInit->toModule.ShutDown			= ShutDown;
	ModInit->toModule.GetTrackExtents	= GetTrackExtents;
	ModInit->toModule.GetMediaSupported = GetMediaSupported;
	ModInit->toModule.GetCurrentPosition= GetCurrentPosition;
	ModInit->toModule.Play				= Play;
	ModInit->toModule.Pause				= Pause;
	ModInit->toModule.Stop				= Stop;
	ModInit->toModule.SetVolume			= SetVolume;
	ModInit->toModule.About				= About;
	ModInit->toModule.Configure			= Configure;
	QCDCallbacks = ModInit;

	/* read config */
	QCDCallbacks->Service(opGetPluginSettingsFile, INI_FILE, MAX_PATH, 0);
	config_read();

	ModInfo->moduleString = "MPEG-4 General Audio Plugin v1.0";
	ModInfo->moduleExtensions = m_use_for_aac ? "MP4:AAC" : "MP4";

	hwndPlayer = (HWND)ModInit->Service(opGetParentWnd, 0, 0, 0);
	mp4state.filename[0] = 0;
	mp4state.seek_needed = -1;
	mp4state.paused = 0;
	play_thread_handle = INVALID_HANDLE_VALUE;

	oldAPIs = 1;

	return TRUE;
}

//-----------------------------------------------------------------------------

PLUGIN_API QCDModInitIn* INPUTDLL_ENTRY_POINT(QCDModInitIn *ModInit, QCDModInfo *ModInfo)
{
	sQCDCallbacks.version						= PLUGIN_API_VERSION;
	sQCDCallbacks.toModule.Initialize			= Initialize;
	sQCDCallbacks.toModule.ShutDown				= ShutDown;
	sQCDCallbacks.toModule.GetTrackExtents		= GetTrackExtents;
	sQCDCallbacks.toModule.GetMediaSupported	= GetMediaSupported;
	sQCDCallbacks.toModule.GetCurrentPosition	= GetCurrentPosition;
	sQCDCallbacks.toModule.Play					= Play;
	sQCDCallbacks.toModule.Pause				= Pause;
	sQCDCallbacks.toModule.Stop					= Stop;
	sQCDCallbacks.toModule.SetVolume			= SetVolume;
	sQCDCallbacks.toModule.About				= About;
	sQCDCallbacks.toModule.Configure			= Configure;

	QCDCallbacks = &sQCDCallbacks;
	return &sQCDCallbacks;
}

//----------------------------------------------------------------------------

int Initialize(QCDModInfo *ModInfo, int flags)
{
	hwndPlayer = (HWND)QCDCallbacks->Service(opGetParentWnd, 0, 0, 0);

	mp4state.filename[0] = 0;
	mp4state.seek_needed = -1;
	mp4state.paused = 0;
	play_thread_handle = INVALID_HANDLE_VALUE;

	/* read config */
	QCDCallbacks->Service(opGetPluginSettingsFile, INI_FILE, MAX_PATH, 0);
    config_read();

	ModInfo->moduleString = "MPEG-4 General Audio Plugin v1.0";
	ModInfo->moduleExtensions = m_use_for_aac ? "MP4:AAC" : "MP4";

	// insert menu item into plugin menu
//	QCDCallbacks->Service(opSetPluginMenuItem, hInstance, IDD_CONFIG, (long)"Mp4 Plug-in");

	return TRUE;
}

//----------------------------------------------------------------------------

void ShutDown(int flags) 
{
	Stop(mp4state.filename, STOPFLAG_FORCESTOP);

	// delete the inserted plugin menu
//	QCDCallbacks->Service(opSetPluginMenuItem, hInstance, 0, 0);
}

//-----------------------------------------------------------------------------

int GetMediaSupported(const char* medianame, MediaInfo *mediaInfo) 
{
	char *ch = strrchr(medianame, '.');

	if (!medianame || !*medianame)
		return FALSE;

	if(!ch)
		return (lstrlen(medianame) > 2); // no extension defaults to me (if not drive letter)

   /* Finally fixed */
    if(StringComp(ch, ".mp4", 4) == 0)
    {
		mediaInfo->mediaType = DIGITAL_FILE_MEDIA;
		mediaInfo->op_canSeek = TRUE;
		mp4state.filetype = 0;
		return TRUE;
	}
	else if(StringComp(ch, ".aac", 4) ==0)
	{
		mediaInfo->mediaType = DIGITAL_FILE_MEDIA;
		mediaInfo->op_canSeek = FALSE;
		mp4state.filetype = 1;
		return TRUE;
	}
	else
		return FALSE;
}

//-----------------------------------------------------------------------------

int getsonglength(char *fn)
{
    long msDuration = 0;

    if(StringComp(fn + strlen(fn) - 3, "MP4", 3) == 0)
    {
        int track;
        MP4Duration length;
        MP4FileHandle file;

        file = MP4Read(fn, 0);
        if (!file)
            return 0;

        if ((track = GetAACTrack(file)) < 0)
        {
            MP4Close(file);
            return -1;
        }

        length = MP4GetTrackDuration(file, track);

        msDuration = MP4ConvertFromTrackDuration(file, track,
            length, MP4_MSECS_TIME_SCALE);

        MP4Close(file);

        return msDuration;
    } 
	else 
	{
//        faadAACInfo aacInfo;
//        get_AAC_format(fn, &aacInfo);

//        return aacInfo.length;
        return 0;
    }
}

int GetTrackExtents(const char* medianame, TrackExtents *ext, int flags)
{
	ext->track = 1;
	ext->start = 0;
	if( (ext->end = getsonglength(medianame)) < 0 )
		return FALSE;
	ext->bytesize = 0;
	ext->unitpersec = 1000;

	return TRUE;
}

//-----------------------------------------------------------------------------

BOOL CALLBACK config_dialog_proc(HWND hwndDlg, UINT message,
                                 WPARAM wParam, LPARAM lParam)
{
    int i;

    switch (message) 
	{
    case WM_INITDIALOG:
		SendMessage(GetDlgItem(hwndDlg, IDC_PRIORITY), TBM_SETRANGE, TRUE, MAKELONG(1,5)); 
		SendMessage(GetDlgItem(hwndDlg, IDC_PRIORITY), TBM_SETPOS, TRUE, m_priority);
        SendMessage(GetDlgItem(hwndDlg, res_id_table[m_resolution]), BM_SETCHECK, BST_CHECKED, 0);
        if (m_show_errors)
            SendMessage(GetDlgItem(hwndDlg, IDC_ERROR), BM_SETCHECK, BST_CHECKED, 0);
        if (m_use_for_aac)
            SendMessage(GetDlgItem(hwndDlg, IDC_USEFORAAC), BM_SETCHECK, BST_CHECKED, 0);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) 
		{
        case IDOK:
            m_show_errors = SendMessage(GetDlgItem(hwndDlg, IDC_ERROR), BM_GETCHECK, 0, 0);
            m_use_for_aac = SendMessage(GetDlgItem(hwndDlg, IDC_USEFORAAC), BM_GETCHECK, 0, 0);
            m_priority = SendMessage(GetDlgItem(hwndDlg, IDC_PRIORITY), TBM_GETPOS, 0, 0);
            for (i = 0; i < 5; i++)
            {
                if (SendMessage(GetDlgItem(hwndDlg, res_id_table[i]), BM_GETCHECK, 0, 0))
                {
                    m_resolution = i;
                    break;
                }
            }

            /* save config */
            config_write();
        case IDCANCEL:
            DestroyWindow(hwndDlg);
            return TRUE;
        }
    }
    return FALSE;
}

void Configure(int flags)
{
	if(!IsWindow(hwndConfig))
		hwndConfig = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_CONFIG_INPUT), hwndPlayer, config_dialog_proc);
	ShowWindow(hwndConfig, SW_SHOWNORMAL);
}

//-----------------------------------------------------------------------------
// proc of "About Dialog"
INT_PTR CALLBACK about_dialog_proc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static RECT rcLOGO, rcMail1, rcMail2/*, rcMail3*/;
	POINT ptMouse;
	static char szPluginVer[] = "QCD MP4 Input Plug-in v1.0\nCompiled on " __TIME__ ", " __DATE__;
	static char szFLACVer[] = "Using: FAAD2 v "FAAD2_VERSION" by";

	switch (uMsg)
	{
	case WM_INITDIALOG:
	case WM_MOVE:
		GetWindowRect(GetDlgItem(hwndDlg, IDC_LOGO), &rcLOGO);
		GetWindowRect(GetDlgItem(hwndDlg, IDC_MAIL1), &rcMail1);
		GetWindowRect(GetDlgItem(hwndDlg, IDC_MAIL2), &rcMail2);
//		GetWindowRect(GetDlgItem(hwndDlg, IDC_MAIL2), &rcMail3);

		SetDlgItemText(hwndDlg, IDC_PLUGINVER, szPluginVer);
		SetDlgItemText(hwndDlg, IDC_FAADVER, szFLACVer);
		
		return TRUE;
	case WM_MOUSEMOVE:
		ptMouse.x = LOWORD(lParam);
		ptMouse.y = HIWORD(lParam);
		ClientToScreen(hwndDlg, &ptMouse);
		if( (ptMouse.x >= rcLOGO.left && ptMouse.x <= rcLOGO.right && 
			ptMouse.y >= rcLOGO.top && ptMouse.y<= rcLOGO.bottom) 
			||
			(ptMouse.x >= rcMail1.left && ptMouse.x <= rcMail1.right && 
			ptMouse.y >= rcMail1.top && ptMouse.y<= rcMail1.bottom) 
			||
			(ptMouse.x >= rcMail2.left && ptMouse.x <= rcMail2.right && 
			ptMouse.y >= rcMail2.top && ptMouse.y<= rcMail2.bottom) 
/*			||
			(ptMouse.x >= rcMail3.left && ptMouse.x <= rcMail3.right && 
			ptMouse.y >= rcMail3.top && ptMouse.y<= rcMail3.bottom)*/ )
			SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(32649)));
		else
			SetCursor(LoadCursor(NULL, IDC_ARROW));

		return TRUE;
	case WM_LBUTTONDOWN:
		ptMouse.x = LOWORD(lParam);
		ptMouse.y = HIWORD(lParam);
		ClientToScreen(hwndDlg, &ptMouse);
		if(ptMouse.x >= rcLOGO.left && ptMouse.x <= rcLOGO.right && 
			ptMouse.y >= rcLOGO.top && ptMouse.y<= rcLOGO.bottom)
			ShellExecute(0, NULL, "http://www.audiocoding.com", NULL,NULL, SW_NORMAL);
		else if(ptMouse.x >= rcMail1.left && ptMouse.x <= rcMail1.right && 
			ptMouse.y >= rcMail1.top && ptMouse.y<= rcMail1.bottom)
			ShellExecute(0, NULL, "mailto:shaohao@elong.com", NULL,NULL, SW_NORMAL);
		else if(ptMouse.x >= rcMail2.left && ptMouse.x <= rcMail2.right && 
			ptMouse.y >= rcMail2.top && ptMouse.y<= rcMail2.bottom)
			ShellExecute(0, NULL, "mailto:menno@audiocoding.com", NULL,NULL, SW_NORMAL);
/*		else if(ptMouse.x >= rcMail3.left && ptMouse.x <= rcMail3.right && 
			ptMouse.y >= rcMail3.top && ptMouse.y<= rcMail3.bottom)
			ShellExecute(0, NULL, "I don't know", NULL,NULL, SW_NORMAL);
*/
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
		default:
			DestroyWindow(hwndDlg);
			return TRUE;
		}
	}
	return FALSE;
}

void About(int flags)
{
	if(!IsWindow(hwndAbout))
		hwndAbout = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_ABOUT), hwndPlayer, about_dialog_proc); 
	ShowWindow(hwndAbout, SW_SHOWNORMAL);
}

//-----------------------------------------------------------------------------

int Play(const char* medianame, int playfrom, int playto, int flags)
{
	static WAVEFORMATEX wf;

	if(flags == PLAYFLAG_ENCODING)
	{
		if(QCDCallbacks->toPlayer.OutputOpen(medianame, &wf))
			Stop(medianame, STOPFLAG_FORCESTOP);
		return FALSE;
	}

	if(stricmp(mp4state.filename, medianame) != 0)
	{
		sQCDCallbacks.toPlayer.OutputStop(STOPFLAG_PLAYDONE);
		Stop(mp4state.filename, STOPFLAG_PLAYDONE);
	}

	if(mp4state.paused)
	{
		// Update the player controls to reflect the new unpaused state
		sQCDCallbacks.toPlayer.OutputPause(0);
		
		Pause(medianame, PAUSE_DISABLED);
		
		if (playfrom >= 0)
			mp4state.seek_needed = playfrom;
	}
	else if(play_thread_handle != INVALID_HANDLE_VALUE)
	{
		mp4state.seek_needed = playfrom;
		return TRUE;
	}
	else
	{
#ifdef DEBUG_OUTPUT
		in_mp4_DebugOutput("play");
#endif
		int thread_id;
		int avg_bitrate, br, sr;
		unsigned char *buffer;
		int buffer_size;
		faacDecConfigurationPtr config;	
		
		mp4state.channels = 0;
		mp4state.samplerate = 0;
		mp4state.filetype = 0;

		strcpy(mp4state.filename, medianame);

		if(StringComp(medianame + strlen(medianame) - 3, "AAC", 3) == 0)
			mp4state.filetype = 1;
		
		mp4state.hDecoder = faacDecOpen();
		if (!mp4state.hDecoder)
		{
			show_error(hwndPlayer, "Unable to open decoder library.");
			return -1;
		}
		
		config = faacDecGetCurrentConfiguration(mp4state.hDecoder);
		config->outputFormat = m_resolution + 1;
		faacDecSetConfiguration(mp4state.hDecoder, config);
		
		if (mp4state.filetype)
		{
			long pos, tmp, read, tagsize;
			
			//        get_AAC_format(mp4state.filename, &mp4state.aacInfo);
			
			mp4state.aacfile = fopen(mp4state.filename, "rb");
			if (!mp4state.aacfile)
			{
				show_error(hwndPlayer, "Unable to open file.");
				faacDecClose(mp4state.hDecoder);
				return -1;
			}
			
			pos = ftell(mp4state.aacfile);
			fseek(mp4state.aacfile, 0, SEEK_END);
			mp4state.filesize = ftell(mp4state.aacfile);
			fseek(mp4state.aacfile, pos, SEEK_SET);
			
			if (!(mp4state.buffer=(unsigned char*)malloc(768*48)))
			{
				show_error(hwndPlayer, "Memory allocation error.");
				faacDecClose(mp4state.hDecoder);
				fclose(mp4state.aacfile);
				return -1;
			}
			memset(mp4state.buffer, 0, 768*48);
			
			if (mp4state.filesize < 768*48)
				tmp = mp4state.filesize;
			else
				tmp = 768*48;
			read = fread(mp4state.buffer, 1, tmp, mp4state.aacfile);
			if (read == tmp)
			{
				mp4state.bytes_read = read;
				mp4state.bytes_into_buffer = read;
			} 
			else 
			{
				show_error(hwndPlayer, "Error reading from file.");
				faacDecClose(mp4state.hDecoder);
				fclose(mp4state.aacfile);
				return -1;
			}
			
			if (StringComp(mp4state.buffer, "ID3", 3) == 0)
			{
				/* high bit is not used */
				tagsize = (mp4state.buffer[6] << 21) | (mp4state.buffer[7] << 14) |
					(mp4state.buffer[8] <<  7) | (mp4state.buffer[9] <<  0);
				
				tagsize += 10;
			} 
			else 
			{
				tagsize = 0;
			}
			
			if ((mp4state.bytes_consumed = faacDecInit(mp4state.hDecoder,
				mp4state.buffer+tagsize, mp4state.bytes_into_buffer,
				&mp4state.samplerate, &mp4state.channels)) < 0)
			{
				show_error(hwndPlayer, "Can't initialize library.");
				faacDecClose(mp4state.hDecoder);
				fclose(mp4state.aacfile);
				return -1;
			}
			mp4state.bytes_consumed += tagsize;
			mp4state.bytes_into_buffer -= mp4state.bytes_consumed;
			
			//        avg_bitrate = mp4state.aacInfo.bitrate;
			avg_bitrate = 0;
			
//			module.is_seekable = 0;
		} 
		else 
		{
			mp4state.mp4file = MP4Read(mp4state.filename, 0);
			if (!mp4state.mp4file)
			{
				show_error(hwndPlayer, "Unable to open file.");
				faacDecClose(mp4state.hDecoder);
				return -1;
			}
			
			if ((mp4state.mp4track = GetAACTrack(mp4state.mp4file)) < 0)
			{
				show_error(hwndPlayer, "Unsupported Audio track type.");
				faacDecClose(mp4state.hDecoder);
				MP4Close(mp4state.mp4file);
				return -1;
			}
			
			buffer = NULL;
			buffer_size = 0;
			MP4GetTrackESConfiguration(mp4state.mp4file, mp4state.mp4track,
				&buffer, &buffer_size);
			if (!buffer)
			{
				faacDecClose(mp4state.hDecoder);
				MP4Close(mp4state.mp4file);
				return -1;
			}
			
			if(faacDecInit2(mp4state.hDecoder, buffer, buffer_size,
				&mp4state.samplerate, &mp4state.channels) < 0)
			{
				/* If some error initializing occured, skip the file */
				faacDecClose(mp4state.hDecoder);
				MP4Close(mp4state.mp4file);
				return -1;
			}
			free(buffer);
			
			avg_bitrate = MP4GetTrackIntegerProperty(mp4state.mp4file, mp4state.mp4track,
				"mdia.minf.stbl.stsd.mp4a.esds.decConfigDescr.avgBitrate");
			
			mp4state.numSamples = MP4GetTrackNumberOfSamples(mp4state.mp4file, mp4state.mp4track);
			mp4state.sampleId = 1;
			
//			module.is_seekable = 1;
		}
		
		if (mp4state.channels == 0)
		{
			show_error(hwndPlayer, "Number of channels not supported for playback.");
			faacDecClose(mp4state.hDecoder);
			if (mp4state.filetype)
				fclose(mp4state.aacfile);
			else
				MP4Close(mp4state.mp4file);
			return -1;
		}
		
		// open outputdevice
		wf.wFormatTag = WAVE_FORMAT_PCM;
		wf.cbSize = 0;
		wf.nChannels = mp4state.channels;
		wf.wBitsPerSample = res_table[m_resolution];
		wf.nSamplesPerSec = mp4state.samplerate;
		wf.nBlockAlign = wf.nChannels * wf.wBitsPerSample / 8;
		wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
		if (!QCDCallbacks->toPlayer.OutputOpen(mp4state.filename, &wf))
		{
			show_error(hwndPlayer, "Error: Failed openning output plugin!");
			faacDecClose(mp4state.hDecoder);
			if (mp4state.filetype)
				fclose(mp4state.aacfile);
			else
				MP4Close(mp4state.mp4file);
			return -1; // cannot open sound device
		}
		
		mp4state.paused			= 0;
		mp4state.decode_pos_ms	= 0;
		mp4state.seek_needed	= playfrom > 0 ? playfrom : -1;
		
		br = (int)floor(((float)avg_bitrate + 500.0)/1000.0);
		sr = (int)floor((float)mp4state.samplerate/1000.0);
		// show constant bitrate at first
		{
			AudioInfo cai;
			cai.struct_size = sizeof(AudioInfo);
			cai.frequency = sr * 1000;
			cai.bitrate = br * 1000;
			cai.mode = (mp4state.channels == 2) ? 0 : 3;
			cai.layer = 0;
			cai.level = 0;
			QCDCallbacks->Service(opSetAudioInfo, &cai, sizeof(AudioInfo), 0);
		}
		
		killPlayThread = 0;
		
		if (mp4state.filetype)
		{
			if ((play_thread_handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AACPlayThread,
				(void *)&killPlayThread, 0, &thread_id)) == NULL)
			{
				show_error(hwndPlayer, "Cannot create playback thread");
				faacDecClose(mp4state.hDecoder);
				fclose(mp4state.aacfile);
				return -1;
			}
		} 
		else 
		{
			if ((play_thread_handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MP4PlayThread,
				(void *)&killPlayThread, 0, &thread_id)) == NULL)
			{
				show_error(hwndPlayer, "Cannot create playback thread");
				faacDecClose(mp4state.hDecoder);
				MP4Close(mp4state.mp4file);
				return -1;
			}
		}
		
		SetThreadAffinityMask(play_thread_handle, 1);
		
		if (m_priority != 3)
			SetThreadPriority(play_thread_handle, priority_table[m_priority]);
	}
	return TRUE;
}

//-----------------------------------------------------------------------------

int Pause(const char* medianame, int flags)
{
#ifdef DEBUG_OUTPUT
    in_mp4_DebugOutput("pause");
#endif
	if(QCDCallbacks->toPlayer.OutputPause(flags))
	{
		// send back pause/unpause notification
		QCDCallbacks->toPlayer.PlayPaused(medianame, flags);
		mp4state.paused = flags;
		return TRUE;
	}
	return FALSE;
}

//-----------------------------------------------------------------------------

void SetVolume(int levelleft, int levelright, int flags)
{
	QCDCallbacks->toPlayer.OutputSetVol(levelleft, levelright, flags);
}

//-----------------------------------------------------------------------------

int GetCurrentPosition(const char* medianame, long *track, long *offset)
{
	return QCDCallbacks->toPlayer.OutputGetCurrentPosition((UINT*)offset, 0);
}

//-----------------------------------------------------------------------------

int Stop(const char* medianame, int flags)
{
#ifdef DEBUG_OUTPUT
    in_mp4_DebugOutput("stop");
#endif
	if(medianame && *medianame && stricmp(mp4state.filename, medianame) == 0)
	{
		sQCDCallbacks.toPlayer.OutputStop(flags);

		killPlayThread = 1;
		if(play_thread_handle != INVALID_HANDLE_VALUE)
		{
			if (WaitForSingleObject(play_thread_handle, INFINITE) == WAIT_TIMEOUT)
			{
//				MessageBox(hwndPlayer, "MP4 thread kill timeout", "debug", 0);
				TerminateThread(play_thread_handle,0);
			}
			CloseHandle(play_thread_handle);
			play_thread_handle = INVALID_HANDLE_VALUE;
		}
		
		if (oldAPIs)
			QCDCallbacks->toPlayer.PlayStopped(mp4state.filename);

		mp4state.filename[0] = 0;
		if(mp4state.hDecoder)
			faacDecClose(mp4state.hDecoder);
		if (mp4state.filetype)
			fclose(mp4state.aacfile);
		else
			MP4Close(mp4state.mp4file);
	}
	
	return TRUE;
}

DWORD WINAPI MP4PlayThread(void *b)
{
	BOOL done = FALSE, updatePos = FALSE;
    int l;

    void *sample_buffer;
    unsigned char *buffer;
    int buffer_size, ms;
    faacDecFrameInfo frameInfo;

#ifdef DEBUG_OUTPUT
    in_mp4_DebugOutput("MP4PlayThread");
#endif

    mp4state.last_frame = 0;

    while (!*((int *)b))
    {
        /* seeking */
        if (!done && mp4state.seek_needed != -1)
        {
            MP4Duration duration;

			QCDCallbacks->toPlayer.OutputFlush(mp4state.decode_pos_ms);
            duration = MP4ConvertToTrackDuration(mp4state.mp4file,
                mp4state.mp4track, mp4state.seek_needed, MP4_MSECS_TIME_SCALE);
            mp4state.sampleId = MP4GetSampleIdFromTime(mp4state.mp4file,
                mp4state.mp4track, duration, 0);

            mp4state.decode_pos_ms = mp4state.seek_needed;
			mp4state.seek_needed = -1;
			updatePos = TRUE;
        }
		/* quit */
		if (done)
		{
			if (QCDCallbacks->toPlayer.OutputDrain(0) && !(mp4state.seek_needed >= 0))
			{
				play_thread_handle = INVALID_HANDLE_VALUE;
				QCDCallbacks->toPlayer.OutputStop(STOPFLAG_PLAYDONE);
				QCDCallbacks->toPlayer.PlayDone(mp4state.filename);
			}
			else if (mp4state.seek_needed >= 0)
			{
				done = FALSE;
				continue;
			}
			break;
		}
		/* decoding */
		else
		{

            if (mp4state.last_frame)
            {
                done = TRUE;
            } 
			else 
			{
                int rc;

                /* get acces unit from MP4 file */
                buffer = NULL;
                buffer_size = 0;

                rc = MP4ReadSample(mp4state.mp4file, mp4state.mp4track,
                    mp4state.sampleId++, &buffer, &buffer_size,
                    NULL, NULL, NULL, NULL);
                if (rc == 0 || buffer == NULL)
                {
                    mp4state.last_frame = 1;
                    sample_buffer = NULL;
                    frameInfo.samples = 0;
                } 
				else 
				{
                    sample_buffer = faacDecDecode(mp4state.hDecoder, &frameInfo,
                        buffer, buffer_size);
                }
                if (frameInfo.error > 0)
                {
                    show_error(hwndPlayer, faacDecGetErrorMessage(frameInfo.error));
                    mp4state.last_frame = 1;
                }
                if (mp4state.sampleId >= mp4state.numSamples)
                    mp4state.last_frame = 1;

                if (buffer) free(buffer);

                if (!killPlayThread && (frameInfo.samples > 0))
                {
                    if (res_table[m_resolution] == 24)
                    {
                        /* convert libfaad output (3 bytes packed in 4) */
                        char *temp_buffer = convert3in4to3in3(sample_buffer, frameInfo.samples);
                        memcpy((void*)sample_buffer, (void*)temp_buffer, frameInfo.samples*3);
                        free(temp_buffer);
                    }

                    ms = (int)floor(((float)frameInfo.samples*1000.0) /
                        ((float)mp4state.samplerate*(float)frameInfo.channels));
                    mp4state.decode_pos_ms += ms;

                    l = frameInfo.samples * res_table[m_resolution] / 8;

					if (updatePos)
					{
						QCDCallbacks->toPlayer.PositionUpdate(mp4state.decode_pos_ms);
						updatePos = FALSE;
					}
                    {
						WriteDataStruct wd;

						wd.bytelen = l;
						wd.data = sample_buffer;
						wd.markerend = 0;
						wd.markerstart = mp4state.decode_pos_ms;
						wd.bps = res_table[m_resolution];
						wd.nch = frameInfo.channels;
						wd.numsamples = frameInfo.samples/frameInfo.channels;
						wd.srate = mp4state.samplerate;

						if (!QCDCallbacks->toPlayer.OutputWrite(&wd))
							done = TRUE;
                    }
                }
            }
        }
		Sleep(10);
    }

	// close up
	play_thread_handle = INVALID_HANDLE_VALUE;

	return 0;
}

DWORD WINAPI AACPlayThread(void *b)
{
	BOOL done = FALSE, updatePos = FALSE;
    int l, ms;

    void *sample_buffer;
    faacDecFrameInfo frameInfo;

#ifdef DEBUG_OUTPUT
    in_mp4_DebugOutput("AACPlayThread");
#endif

    mp4state.last_frame = 0;

    while (!*((int *)b))
    {
#if 0
        /* seeking */
        if (!done && mp4state.seek_needed != -1)
        {
            int ms;

            /* Round off to a second */
            ms = mp4state.seek_needed - (mp4state.seek_needed%1000);
			QCDCallbacks->toPlayer.OutputFlush(mp4state.decode_pos_ms);
            aac_seek(ms);
            mp4state.decode_pos_ms = ms;
            mp4state.seek_needed = -1;
			updatePos = TRUE;
        }
#endif
		/* quit */
		if (done)
		{
			if (QCDCallbacks->toPlayer.OutputDrain(0) && !(mp4state.seek_needed >= 0))
			{
				QCDCallbacks->toPlayer.OutputStop(STOPFLAG_PLAYDONE);
				QCDCallbacks->toPlayer.PlayDone(mp4state.filename);
			}
			else if (mp4state.seek_needed >= 0)
			{
				done = FALSE;
				continue;
			}
			break;
		}

		/* decoding */
		else
		{
            if (mp4state.last_frame)
            {
                done = TRUE;
            } 
			else 
			{
                long tmp, read;
                unsigned char *buffer = mp4state.buffer;

                do
                {
                    if (mp4state.bytes_consumed > 0)
                    {
                        if (mp4state.bytes_into_buffer)
                        {
                            memcpy(buffer, buffer+mp4state.bytes_consumed,
                                mp4state.bytes_into_buffer);
                        }

                        if (mp4state.bytes_read < mp4state.filesize)
                        {
                            if (mp4state.bytes_read + mp4state.bytes_consumed < mp4state.filesize)
                                tmp = mp4state.bytes_consumed;
                            else
                                tmp = mp4state.filesize - mp4state.bytes_read;
                            read = fread(buffer + mp4state.bytes_into_buffer, 1, tmp, mp4state.aacfile);
                            if (read == tmp)
                            {
                                mp4state.bytes_read += read;
                                mp4state.bytes_into_buffer += read;
                            }
                        } 
						else 
						{
                            if (mp4state.bytes_into_buffer)
                            {
                                memset(buffer + mp4state.bytes_into_buffer, 0,
                                    mp4state.bytes_consumed);
                            }
                        }

                        mp4state.bytes_consumed = 0;
                    }

                    if (mp4state.bytes_into_buffer < 1)
                    {
                        if (mp4state.bytes_read < mp4state.filesize)
                        {
                            show_error(hwndPlayer, faacDecGetErrorMessage(frameInfo.error));
                            mp4state.last_frame = 1;
                        } 
						else
						{
                            mp4state.last_frame = 1;
                        }
                    }

                    sample_buffer = faacDecDecode(mp4state.hDecoder, &frameInfo,
                        buffer, mp4state.bytes_into_buffer);

                    mp4state.bytes_consumed += frameInfo.bytesconsumed;
                    mp4state.bytes_into_buffer -= mp4state.bytes_consumed;
                } while (!frameInfo.samples && !frameInfo.error);

                if (!killPlayThread && (frameInfo.samples > 0))
                {
                    if (res_table[m_resolution] == 24)
                    {
                        /* convert libfaad output (3 bytes packed in 4 bytes) */
                        char *temp_buffer = convert3in4to3in3(sample_buffer, frameInfo.samples);
                        memcpy((void*)sample_buffer, (void*)temp_buffer, frameInfo.samples*3);
                        free(temp_buffer);
                    }

                    ms = (int)floor(((float)frameInfo.samples*1000.0) /
                        ((float)mp4state.samplerate*(float)frameInfo.channels));
                    mp4state.decode_pos_ms += ms;

                    l = frameInfo.samples * res_table[m_resolution] / 8;

					if (updatePos)
					{
						QCDCallbacks->toPlayer.PositionUpdate(mp4state.decode_pos_ms);
						updatePos = FALSE;
					}
                    {
						WriteDataStruct wd;

						wd.bytelen = l;
						wd.data = sample_buffer;
						wd.markerend = 0;
						wd.markerstart = mp4state.decode_pos_ms;
						wd.bps = res_table[m_resolution];
						wd.nch = frameInfo.channels;
						wd.numsamples = frameInfo.samples/frameInfo.channels;
						wd.srate = mp4state.samplerate;

						if (!QCDCallbacks->toPlayer.OutputWrite(&wd))
							done = TRUE;
                    } 
                }
            }
        } 
		Sleep(10);
    }

	// close up
	play_thread_handle = INVALID_HANDLE_VALUE;

    return 0;
}
