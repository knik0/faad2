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
** $Id: in_mp4.c,v 1.12 2002/08/07 08:14:31 menno Exp $
**/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <faad.h>
#include <mp4.h>

#include "resource.h"
#include "in2.h"

static char app_name[] = "AudioCoding.com MPEG-4 General Audio player";
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
    IDC_32BITS
};
static int res_table[] = {
    16,
    24,
    32
};
static char info_fn[_MAX_PATH];

// post this to the main window at end of file (after playback has stopped)
#define WM_WA_AAC_EOF WM_USER+2

typedef struct state
{
    faacDecHandle hDecoder;
    MP4FileHandle mp4file;
    int mp4track;
    MP4SampleId numSamples;
    MP4SampleId sampleId;
    int samplerate;
    unsigned char channels;
    int decode_pos_ms; // current decoding position, in milliseconds
    int paused; // are we paused?
    int seek_needed; // if != -1, it is the point that the decode thread should seek to, in ms.
    char filename[_MAX_PATH];
} state;

static char INI_FILE[MAX_PATH];
static int m_priority = 3;
static int m_resolution = 0;
static int m_show_errors = 1;

static state mp4state;

static In_Module module; // the output module (declared near the bottom of this file)

static int killPlayThread;
static int PlayThreadAlive = 0; // 1=play thread still running
HANDLE play_thread_handle = INVALID_HANDLE_VALUE; // the handle to the decode thread

/* Function definitions */
DWORD WINAPI PlayThread(void *b); // the decode thread procedure

void show_error(HWND hwnd, char *message, ...)
{
    if (m_show_errors)
        MessageBox(hwnd, message, "Error", MB_OK);
}

int StringComp(char const *str1, char const *str2, unsigned long len)
{
    signed int c1 = 0, c2 = 0;

    while (len--)
    {
        c1 = *str1++;
        c2 = *str2++;

        if (c1 == 0 || c1 != c2)
            break;
    }

    return c1 - c2;
}

int GetAACTrack(MP4FileHandle infile)
{
    /* find AAC track */
    int i, rc;
	int numTracks = MP4GetNumberOfTracks(infile, NULL, 0);

	for (i = 0; i < numTracks; i++)
    {
        MP4TrackId trackId = MP4FindTrackId(infile, i, NULL, 0);
        const char* trackType = MP4GetTrackType(infile, trackId);

        if (!strcmp(trackType, MP4_AUDIO_TRACK_TYPE))
        {
            unsigned char *buff = NULL;
            int buff_size = 0;
            unsigned char dummy2_8, dummy3_8, dummy4_8, dummy5_8, dummy6_8,
                dummy7_8, dummy8_8;
            unsigned int dummy1_32;
            MP4GetTrackESConfiguration(infile, trackId, &buff, &buff_size);

            if (buff)
            {
                rc = AudioSpecificConfig(buff, &dummy1_32, &dummy2_8, &dummy3_8, &dummy4_8,
                    &dummy5_8, &dummy6_8, &dummy7_8, &dummy8_8);
                free(buff);

                if (rc < 0)
                    return -1;
                return trackId;
            }
        }
    }

    /* can't decode this */
    return -1;
}

int GetAudioTrack(MP4FileHandle infile)
{
    /* find AAC track */
    int i;
	int numTracks = MP4GetNumberOfTracks(infile, NULL, 0);

	for (i = 0; i < numTracks; i++)
    {
        MP4TrackId trackId = MP4FindTrackId(infile, i, NULL, 0);
        const char* trackType = MP4GetTrackType(infile, trackId);

        if (!strcmp(trackType, MP4_AUDIO_TRACK_TYPE))
        {
            return trackId;
        }
    }

    /* can't decode this */
    return -1;
}

int GetVideoTrack(MP4FileHandle infile)
{
    /* find AAC track */
    int i;
	int numTracks = MP4GetNumberOfTracks(infile, NULL, 0);

	for (i = 0; i < numTracks; i++)
    {
        MP4TrackId trackId = MP4FindTrackId(infile, i, NULL, 0);
        const char* trackType = MP4GetTrackType(infile, trackId);

        if (!strcmp(trackType, MP4_VIDEO_TRACK_TYPE))
        {
            return trackId;
        }
    }

    /* can't decode this */
    return -1;
}

static void _r_s(char *name,char *data, int mlen)
{
	char buf[10];
	strcpy(buf,data);
	GetPrivateProfileString(app_name,name,buf,data,mlen,INI_FILE);
}
#define RS(x) (_r_s(#x,x,sizeof(x)))
#define WS(x) (WritePrivateProfileString(app_name,#x,x,INI_FILE))



static void config_init()
{
	char *p=INI_FILE;
	GetModuleFileName(NULL,INI_FILE,sizeof(INI_FILE));
	while (*p) p++;
	while (p >= INI_FILE && *p != '.') p--;
	strcpy(p+1,"ini");
}

void config_read()
{
    char priority[10];
    char resolution[10];
    char show_errors[10];

	config_init();

    strcpy(show_errors, "1");
    strcpy(priority, "3");
    strcpy(resolution, "0");

    RS(priority);
	RS(resolution);
	RS(show_errors);

    m_priority = atoi(priority);
    m_resolution = atoi(resolution);
    m_show_errors = atoi(show_errors);
}

void config_write()
{
    char priority[10];
    char resolution[10];
    char show_errors[10];

    itoa(m_priority, priority, 10);
    itoa(m_resolution, resolution, 10);
    itoa(m_show_errors, show_errors, 10);

    WS(priority);
	WS(resolution);
	WS(show_errors);
}

void init()
{
    config_read();
}

void quit()
{
}

BOOL CALLBACK info_dialog_proc(HWND hwndDlg, UINT message,
                               WPARAM wParam, LPARAM lParam)
{
    int i, width, height, numFrames;
    unsigned char ch, sf, dummy8;
    float fps;
    MP4FileHandle file;
    int track;
    char info[256];

    static const char* mpeg4AudioNames[] = {
        "MPEG-4 Null (Raw PCM)",
        "MPEG-4 AAC Main",
        "MPEG-4 AAC LC",
        "MPEG-4 AAC SSR",
        "MPEG-4 AAC LTP",
        "Reserved",
        "MPEG-4 AAC Scalable",
        "MPEG-4 TwinVQ",
        "MPEG-4 CELP",
        "MPEG-4 HVXC",
        "Reserved",
        "Reserved",
        "MPEG-4 TTSI",
        "MPEG-4 Main synthetic",
        "MPEG-4 Wavetable synthesis",
        "MPEG-4 General MIDI",
        "MPEG-4 Algorithmic Synthesis and Audio FX",
        /* defined in MPEG-4 version 2 */
        "MPEG-4 ER AAC LC",
        "Reserved",
        "MPEG-4 ER AAC LTP",
        "MPEG-4 ER AAC Scalable",
        "MPEG-4 ER TwinVQ",
        "MPEG-4 ER BSAC",
        "MPEG-4 ER AAC LD",
        "MPEG-4 ER CELP",
        "MPEG-4 ER HVXC",
        "MPEG-4 ER HILN",
        "MPEG-4 ER Parametric",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved"
    };
    static int numMpeg4AudioTypes = 
        sizeof(mpeg4AudioNames)/sizeof(char*);

	static const char* mpeg4VideoNames[] = {
		"MPEG-4 Simple @ L3",
		"MPEG-4 Simple @ L2",
		"MPEG-4 Simple @ L1",
		"MPEG-4 Simple Scalable @ L2",
		"MPEG-4 Simple Scalable @ L1",
		"MPEG-4 Core @ L2",
		"MPEG-4 Core @ L1",
		"MPEG-4 Main @ L4",
		"MPEG-4 Main @ L3",
		"MPEG-4 Main @ L2",
		"MPEG-4 Main @ L1",
		"MPEG-4 N-Bit @ L2",
		"MPEG-4 Hybrid @ L2",
		"MPEG-4 Hybrid @ L1",
		"MPEG-4 Hybrid @ L1"
	};
	static int numMpeg4VideoTypes = 
		sizeof(mpeg4VideoNames) / sizeof(char*);

	static int mpegVideoTypes[] = {
		MP4_MPEG2_SIMPLE_VIDEO_TYPE,	// 0x60
		MP4_MPEG2_MAIN_VIDEO_TYPE,		// 0x61
		MP4_MPEG2_SNR_VIDEO_TYPE,		// 0x62
		MP4_MPEG2_SPATIAL_VIDEO_TYPE,	// 0x63
		MP4_MPEG2_HIGH_VIDEO_TYPE,		// 0x64
		MP4_MPEG2_442_VIDEO_TYPE,		// 0x65
		MP4_MPEG1_VIDEO_TYPE,			// 0x6A
		MP4_JPEG_VIDEO_TYPE 			// 0x6C
	};
	static const char* mpegVideoNames[] = {
		"MPEG-2 Simple",
		"MPEG-2 Main",
		"MPEG-2 SNR",
		"MPEG-2 Spatial",
		"MPEG-2 High",
		"MPEG-2 4:2:2",
		"MPEG-1",
		"JPEG"
	};
	static int numMpegVideoTypes = 
		sizeof(mpegVideoTypes) / sizeof(u_int8_t);

    unsigned long msDuration;
    MP4Duration trackDuration;
    unsigned int timeScale, avgBitRate;
    unsigned char type;
    const char* typeName;

    switch (message) {
    case WM_INITDIALOG:
        file = MP4Read(info_fn, 0);
        if (!file)
            return FALSE;

        if ((track = GetAudioTrack(file)) >= 0)
        {
            unsigned char *buff = NULL;
            int buff_size = 0;
            MP4GetTrackESConfiguration(file, track, &buff, &buff_size);

            if (buff)
            {
                AudioSpecificConfig(buff, &timeScale, &ch, &sf, &type,
                    &dummy8, &dummy8, &dummy8, &dummy8);
                typeName = mpeg4AudioNames[type];
                free(buff);

                sprintf(info, "%d", ch);
                SetDlgItemText(hwndDlg, IDC_CHANNELS, info);

                sprintf(info, "%d Hz", timeScale);
                SetDlgItemText(hwndDlg, IDC_SAMPLERATE, info);
            } else {
                typeName = "Unknown";
                SetDlgItemText(hwndDlg, IDC_CHANNELS, "n/a");
                SetDlgItemText(hwndDlg, IDC_SAMPLERATE, "n/a");
            }
            trackDuration = MP4GetTrackDuration(file, track);

            msDuration = MP4ConvertFromTrackDuration(file, track,
                trackDuration, MP4_MSECS_TIME_SCALE);

            avgBitRate = MP4GetTrackIntegerProperty(file, track,
                "mdia.minf.stbl.stsd.mp4a.esds.decConfigDescr.avgBitrate");

            SetDlgItemText(hwndDlg, IDC_TYPE, typeName);

            sprintf(info, "%.3f secs", (float)msDuration/1000.0);
            SetDlgItemText(hwndDlg, IDC_DURATION, info);

            sprintf(info, "%u Kbps", (avgBitRate+500)/1000);
            SetDlgItemText(hwndDlg, IDC_BITRATE, info);
        }

        if ((track = GetVideoTrack(file)) >= 0)
        {
            type = MP4GetTrackVideoType(file, track);
            typeName = "Unknown";

            if (type == MP4_MPEG4_VIDEO_TYPE) {
                type = MP4GetVideoProfileLevel(file);
                if (type > 0 && type <= numMpeg4VideoTypes) {
                    typeName = mpeg4VideoNames[type - 1];
                } else {
                    typeName = "MPEG-4";
                }
            } else {
                for (i = 0; i < numMpegVideoTypes; i++) {
                    if (type == mpegVideoTypes[i]) {
                        typeName = mpegVideoNames[i];
                        break;
                    }
                }
            }

            trackDuration = MP4GetTrackDuration(file, track);
            msDuration = MP4ConvertFromTrackDuration(file, track,
                trackDuration, MP4_MSECS_TIME_SCALE);

            avgBitRate = MP4GetTrackIntegerProperty(file, track,
                "mdia.minf.stbl.stsd.mp4v.esds.decConfigDescr.avgBitrate");

            // Note not all mp4 implementations set width and height correctly
            // The real answer can be buried inside the ES configuration info
            width = MP4GetTrackIntegerProperty(file, track,
                "mdia.minf.stbl.stsd.mp4v.width");

            height = MP4GetTrackIntegerProperty(file, track,
                "mdia.minf.stbl.stsd.mp4v.height");

            // Note if variable rate video is being used the fps is an average 
            numFrames = MP4GetTrackNumberOfSamples(file, track);

            fps = ((float)numFrames/(float)msDuration) * 1000;

            SetDlgItemText(hwndDlg, IDC_VTYPE, typeName);

            sprintf(info, "%.3f secs", (float)msDuration/1000.0);
            SetDlgItemText(hwndDlg, IDC_VDURATION, info);

            sprintf(info, "%u Kbps", (avgBitRate+500)/1000);
            SetDlgItemText(hwndDlg, IDC_VBITRATE, info);

            sprintf(info, "%dx%d", width, height);
            SetDlgItemText(hwndDlg, IDC_VSIZE, info);

            sprintf(info, "%.2f", fps);
            SetDlgItemText(hwndDlg, IDC_VFPS, info);
        }

        MP4Close(file);

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
        case IDOK:
            EndDialog(hwndDlg, wParam);
            return TRUE;
        }
    }
    return FALSE;
}

int infoDlg(char *fn, HWND hwndParent)
{
    lstrcpy(info_fn, fn);

    DialogBox(module.hDllInstance, MAKEINTRESOURCE(IDD_INFO),
        hwndParent, info_dialog_proc);

    return 0;
}

BOOL CALLBACK config_dialog_proc(HWND hwndDlg, UINT message,
                                 WPARAM wParam, LPARAM lParam)
{
    int i;

    switch (message) {
    case WM_INITDIALOG:
		SendMessage(GetDlgItem(hwndDlg, IDC_PRIORITY), TBM_SETRANGE, TRUE, MAKELONG(1,5)); 
		SendMessage(GetDlgItem(hwndDlg, IDC_PRIORITY), TBM_SETPOS, TRUE, m_priority);
        SendMessage(GetDlgItem(hwndDlg, res_id_table[m_resolution]), BM_SETCHECK, BST_CHECKED, 0);
        if (m_show_errors)
            SendMessage(GetDlgItem(hwndDlg, IDC_ERROR), BM_SETCHECK, BST_CHECKED, 0);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hwndDlg, wParam);
            return TRUE;
        case IDOK:
            m_show_errors = SendMessage(GetDlgItem(hwndDlg, IDC_ERROR), BM_GETCHECK, 0, 0);
            m_priority = SendMessage(GetDlgItem(hwndDlg, IDC_PRIORITY), TBM_GETPOS, 0, 0);
            for (i = 0; i < 3; i++)
            {
                int set = SendMessage(GetDlgItem(hwndDlg, res_id_table[i]), BM_GETCHECK, 0, 0);
                if (set)
                {
                    m_resolution = i;
                    break;
                }
            }

            /* save config */
            config_write();
            EndDialog(hwndDlg, wParam);
            return TRUE;
        }
    }
    return FALSE;
}

void config(HWND hwndParent)
{
    DialogBox(module.hDllInstance, MAKEINTRESOURCE(IDD_CONFIG),
        hwndParent, config_dialog_proc);

    return;
}

void about(HWND hwndParent)
{
    MessageBox(hwndParent,
        "AudioCoding.com MPEG-4 General Audio player.\n"
        "Visit the website for more info.\n"
        "Copyright 2002 AudioCoding.com",
        "About",
        MB_OK);
}

int isourfile(char *fn)
{
    if(StringComp(fn + strlen(fn) - 3, "mp4", 3) == 0)
    {
        return 1;
    }

    return 0;
}

int play(char *fn)
{
    int maxlatency;
    int thread_id;
    int avg_bitrate;
    void *sample_buffer;
    unsigned char *buffer;
    int buffer_size;
    faacDecFrameInfo frameInfo;
    faacDecConfigurationPtr config;

    mp4state.channels = 0;
    mp4state.samplerate = 0;

    strcpy(mp4state.filename, fn);

    mp4state.mp4file = MP4Read(fn, 0);
    if (!mp4state.mp4file)
    {
        show_error(module.hMainWindow, "Unable to open file.");
        return -1;
    }

    if ((mp4state.mp4track = GetAACTrack(mp4state.mp4file)) < 0)
    {
        show_error(module.hMainWindow, "Unsupported Audio track type.");
        MP4Close(mp4state.mp4file);
        return -1;
    }

    mp4state.hDecoder = faacDecOpen();

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

    config = faacDecGetCurrentConfiguration(mp4state.hDecoder);
    config->outputFormat = m_resolution + 1;
    faacDecSetConfiguration(mp4state.hDecoder, config);

    if (mp4state.channels == 0)
    {
        show_error(module.hMainWindow, "Number of channels not supported for playback.");
        faacDecClose(mp4state.hDecoder);
        MP4Close(mp4state.mp4file);
        return -1;
    }

    maxlatency = module.outMod->Open(mp4state.samplerate, mp4state.channels,
        res_table[m_resolution], -1,-1);
    if (maxlatency < 0) // error opening device
    {
        faacDecClose(mp4state.hDecoder);
        MP4Close(mp4state.mp4file);
        return -1;
    }

    avg_bitrate = MP4GetTrackIntegerProperty(mp4state.mp4file, mp4state.mp4track,
        "mdia.minf.stbl.stsd.mp4a.esds.decConfigDescr.avgBitrate");

    mp4state.numSamples = MP4GetTrackNumberOfSamples(mp4state.mp4file, mp4state.mp4track);
    mp4state.sampleId = 1;

    mp4state.paused        =  0;
    mp4state.decode_pos_ms =  0;
    mp4state.seek_needed   = -1;

    module.is_seekable = 1;

    // initialize vis stuff
    module.SAVSAInit(maxlatency, mp4state.samplerate);
    module.VSASetInfo(mp4state.samplerate, mp4state.channels);

    module.SetInfo((avg_bitrate + 500)/1000, mp4state.samplerate/1000, mp4state.channels, 1);

    module.outMod->SetVolume(-666); // set the output plug-ins default volume

    killPlayThread = 0;

    if((play_thread_handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PlayThread, (void *)&killPlayThread, 0, &thread_id)) == NULL)
    {
        show_error(module.hMainWindow, "Cannot create playback thread");
        faacDecClose(mp4state.hDecoder);
        MP4Close(mp4state.mp4file);
        return -1;
    }

    if (m_priority != 3)
        SetThreadPriority(play_thread_handle, priority_table[m_priority]);

    return 0;
}

void pause()
{
    mp4state.paused = 1;
    module.outMod->Pause(1);
}

void unpause()
{
    mp4state.paused = 0;
    module.outMod->Pause(0);
}

int ispaused()
{
    return mp4state.paused;
}

void setvolume(int volume)
{
    module.outMod->SetVolume(volume);
}

void setpan(int pan)
{
    module.outMod->SetPan(pan);
}

void stop()
{
    killPlayThread = 1;

    if (play_thread_handle != INVALID_HANDLE_VALUE)
    {
        if (WaitForSingleObject(play_thread_handle, INFINITE) == WAIT_TIMEOUT)
            TerminateThread(play_thread_handle,0);
        CloseHandle(play_thread_handle);
        play_thread_handle = INVALID_HANDLE_VALUE;
    }

    faacDecClose(mp4state.hDecoder);
    MP4Close(mp4state.mp4file);
    module.outMod->Close();
    module.SAVSADeInit();
}

int getsonglength(char *fn)
{
    int track;
    long msDuration;
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

int getlength()
{
    int track;
    long msDuration;
	MP4Duration length;

    if ((track = GetAACTrack(mp4state.mp4file)) < 0)
    {
        return -1;
    }

    length = MP4GetTrackDuration(mp4state.mp4file, track);

	msDuration = MP4ConvertFromTrackDuration(mp4state.mp4file, track,
        length, MP4_MSECS_TIME_SCALE);

    return msDuration;
}

int getoutputtime()
{
    return mp4state.decode_pos_ms+(module.outMod->GetOutputTime()-module.outMod->GetWrittenTime());
}

void setoutputtime(int time_in_ms)
{
    mp4state.seek_needed = time_in_ms;
}

LPTSTR PathFindFileName(LPCTSTR pPath)
{
    LPCTSTR pT;

    for (pT = pPath; *pPath; pPath = CharNext(pPath)) {
        if ((pPath[0] == TEXT('\\') || pPath[0] == TEXT(':')) && pPath[1] && (pPath[1] != TEXT('\\')))
            pT = pPath + 1;
    }

    return (LPTSTR)pT;   // const -> non const
}

void getfileinfo(char *filename, char *title, int *length_in_ms)
{
    if (!filename || !*filename)  /* currently playing file */
    {
        if (length_in_ms)
            *length_in_ms = getlength();

        if (title)
        {
            char *tmp = PathFindFileName(mp4state.filename);
            strcpy(title, tmp);
        }
    }
    else // some other file
    {
        if (length_in_ms)
            *length_in_ms = getsonglength(filename);

        if (title)
        {
            char *tmp = PathFindFileName(filename);
            strcpy(title, tmp);
        }
    }
}

void eq_set(int on, char data[10], int preamp)
{
}

int last_frame;

DWORD WINAPI PlayThread(void *b)
{
    int done = 0;
    int l;
	int decoded_frames=0;
	int br_calc_frames=0;
	int br_bytes_consumed=0;
    unsigned long bytesconsumed;

    void *sample_buffer;
    unsigned char *buffer;
    int buffer_size;
    faacDecFrameInfo frameInfo;

	PlayThreadAlive = 1;
    last_frame = 0;

    while (!*((int *)b))
    {
        /* seeking */
        if (mp4state.seek_needed != -1)
        {
            MP4Duration duration;

            module.outMod->Flush(mp4state.decode_pos_ms);
            duration = MP4ConvertToTrackDuration(mp4state.mp4file,
                mp4state.mp4track, mp4state.seek_needed, MP4_MSECS_TIME_SCALE);
            mp4state.sampleId = MP4GetSampleIdFromTime(mp4state.mp4file,
                mp4state.mp4track, duration, 0);

            mp4state.decode_pos_ms = mp4state.seek_needed;
			mp4state.seek_needed = -1;
        }

        if (done)
        {
            module.outMod->CanWrite();

            if (!module.outMod->IsPlaying())
            {
                PostMessage(module.hMainWindow, WM_WA_AAC_EOF, 0, 0);
                PlayThreadAlive = 0;
                return 0;
            }

            Sleep(10);
        }
        else if (module.outMod->CanWrite() >=
            ((1024*mp4state.channels*sizeof(short))<<(module.dsp_isactive()?1:0)))
        {
            if(last_frame)
            {
                done = 1;
            } else {
                int rc;

                /* get acces unit from MP4 file */
                buffer = NULL;
                buffer_size = 0;

                rc = MP4ReadSample(mp4state.mp4file, mp4state.mp4track,
                    mp4state.sampleId++, &buffer, &buffer_size,
                    NULL, NULL, NULL, NULL);
                if (rc == 0 || buffer == NULL)
                {
                    last_frame = 1;
                    sample_buffer = NULL;
                    frameInfo.samples = 0;
                } else {
                    sample_buffer = faacDecDecode(mp4state.hDecoder, &frameInfo, buffer);
                }
                if (frameInfo.error > 0)
                {
                    show_error(module.hMainWindow, faacDecGetErrorMessage(frameInfo.error));
                    last_frame = 1;
                }
                if (mp4state.sampleId >= mp4state.numSamples)
                    last_frame = 1;

                if (buffer) free(buffer);

                if (!killPlayThread && (frameInfo.samples > 0))
                {
                    module.SAAddPCMData(sample_buffer, mp4state.channels, res_table[m_resolution],
                        mp4state.decode_pos_ms);
                    module.VSAAddPCMData(sample_buffer, mp4state.channels, res_table[m_resolution],
                        mp4state.decode_pos_ms);
                    mp4state.decode_pos_ms += (1024*1000)/mp4state.samplerate;

                    if (module.dsp_isactive())
                    {
                        l = module.dsp_dosamples((short*)sample_buffer,
                            frameInfo.samples*sizeof(short)/mp4state.channels/(res_table[m_resolution]/8),
                            res_table[m_resolution],
                            mp4state.channels,mp4state.samplerate)*(mp4state.channels*(res_table[m_resolution]/8));
                    } else {
                        l = frameInfo.samples*(res_table[m_resolution]/8);
                    }

                    module.outMod->Write(sample_buffer, l);
                }
            }
        }
        else
        {
            Sleep(10);
        }
    }

	PlayThreadAlive = 0;
	
    return 0;
}

static In_Module module =
{
    IN_VER,
    "AudioCoding.com MPEG-4 General Audio player: " __DATE__,
    0,  // hMainWindow
    0,  // hDllInstance
    "MP4\0MPEG-4 Files (*.MP4)\0"
    ,
    1, // is_seekable
    1, // uses output
    config,
    about,
    init,
    quit,
    getfileinfo,
    infoDlg,
    isourfile,
    play,
    pause,
    unpause,
    ispaused,
    stop,

    getlength,
    getoutputtime,
    setoutputtime,

    setvolume,
    setpan,

    0,0,0,0,0,0,0,0,0, // vis stuff


    0,0, // dsp

    eq_set,

    NULL,       // setinfo

    0 // out_mod
};

__declspec(dllexport) In_Module* winampGetInModule2()
{
    return &module;
}
