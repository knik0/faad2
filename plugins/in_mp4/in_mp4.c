/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003 M. Bakker, Ahead Software AG, http://www.nero.com
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
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: in_mp4.c,v 1.43 2003/10/17 17:11:38 ca5e Exp $
**/

//#define DEBUG_OUTPUT

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdlib.h>
#include <math.h>
#include <faad.h>
#include <mp4.h>

#include "resource.h"
#include "in2.h"
#include "utils.h"
#include "config.h"
#include "aacinfo.h"
#include "aac2mp4.h"

const char *long_ext_list = "MP4\0MPEG-4 Files (*.MP4)\0M4A\0MPEG-4 Files (*.M4A)\0AAC\0AAC Files (*.AAC)\0";
const char *short_ext_list = "MP4\0MPEG-4 Files (*.MP4)\0M4A\0MPEG-4 Files (*.M4A)\0";

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
    0,
    IDC_16BITS_DITHERED
};
static int res_table[] = {
    16,
    24,
    32,
    0,
    0,
    16
};
static char info_fn[_MAX_PATH];

// post this to the main window at end of file (after playback has stopped)
#define WM_WA_AAC_EOF WM_USER+2

struct seek_list
{
    struct seek_list *next;
    __int64 offset;
};

typedef struct state
{
    /* general stuff */
    faacDecHandle hDecoder;
    int samplerate;
    unsigned char channels;
    double decode_pos_ms; // current decoding position, in milliseconds
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
    long m_aac_bytes_into_buffer;
    long m_aac_bytes_consumed;
    __int64 m_file_offset;
    unsigned char *m_aac_buffer;
    int m_at_eof;
    double cur_pos_sec;
    int m_header_type;
    struct seek_list *m_head;
    struct seek_list *m_tail;
    unsigned long m_length;

    /* for gapless decoding */
    unsigned int useAacLength;
    unsigned int framesize;
    unsigned int initial;
    unsigned long timescale;
} state;

static state mp4state;

static In_Module module; // the output module (declared near the bottom of this file)

static int killPlayThread;
static int PlayThreadAlive = 0; // 1=play thread still running
HANDLE play_thread_handle = INVALID_HANDLE_VALUE; // the handle to the decode thread

/* Function definitions */
void *decode_aac_frame(state *st, faacDecFrameInfo *frameInfo);
DWORD WINAPI MP4PlayThread(void *b); // the decode thread procedure
DWORD WINAPI AACPlayThread(void *b); // the decode thread procedure


#ifdef DEBUG_OUTPUT
void in_mp4_DebugOutput(char *message)
{
    char s[1024];

    sprintf(s, "in_mp4: %s: %s", mp4state.filename, message);
    OutputDebugString(s);
}
#endif

int file_length(FILE *f)
{
    long end = 0;
    long cur = ftell(f);
    fseek(f, 0, SEEK_END);
    end = ftell(f);
    fseek(f, cur, SEEK_SET);

    return end;
}

static void show_error(HWND hwnd, char *message, ...)
{
    if (m_show_errors)
        MessageBox(hwnd, message, "Error", MB_OK);
}

static void config_init()
{
    char *p=INI_FILE;
    GetModuleFileName(NULL,INI_FILE,_MAX_PATH);
    while (*p) p++;
    while (p >= INI_FILE && *p != '.') p--;
    strcpy(p+1,"ini");
}

void config_read()
{
    char priority[10];
    char resolution[10];
    char show_errors[10];
    char use_for_aac[10];
    char downmix[10];
    char vbr_display[10];

    config_init();

    strcpy(show_errors, "1");
    strcpy(priority, "3");
    strcpy(resolution, "0");
    strcpy(use_for_aac, "1");
    strcpy(downmix, "0");
    strcpy(vbr_display, "1");
    strcpy(titleformat, "%7");

    RS(priority);
    RS(resolution);
    RS(show_errors);
    RS(use_for_aac);
    RS(downmix);
    RS(vbr_display);
    RS(titleformat);

    m_priority = atoi(priority);
    m_resolution = atoi(resolution);
    m_show_errors = atoi(show_errors);
    m_use_for_aac = atoi(use_for_aac);
    m_downmix = atoi(downmix);
    m_vbr_display = atoi(vbr_display);
}

void config_write()
{
    char priority[10];
    char resolution[10];
    char show_errors[10];
    char use_for_aac[10];
    char downmix[10];
    char vbr_display[10];

    itoa(m_priority, priority, 10);
    itoa(m_resolution, resolution, 10);
    itoa(m_show_errors, show_errors, 10);
    itoa(m_use_for_aac, use_for_aac, 10);
    itoa(m_downmix, downmix, 10);
    itoa(m_vbr_display, vbr_display, 10);

    WS(priority);
    WS(resolution);
    WS(show_errors);
    WS(use_for_aac);
    WS(downmix);
    WS(vbr_display);
    WS(titleformat);
}

void init()
{
    config_read();
}

void quit()
{
}

/* Convert UNICODE to UTF-8
   Return number of bytes written */
int unicodeToUtf8 ( const WCHAR* lpWideCharStr, char* lpMultiByteStr, int cwcChars )
{
    const unsigned short*   pwc = (unsigned short *)lpWideCharStr;
    unsigned char*          pmb = (unsigned char  *)lpMultiByteStr;
    const unsigned short*   pwce;
    size_t  cBytes = 0;

    if ( cwcChars >= 0 ) {
        pwce = pwc + cwcChars;
    } else {
        pwce = (unsigned short *)((size_t)-1);
    }

    while ( pwc < pwce ) {
        unsigned short  wc = *pwc++;

        if ( wc < 0x00000080 ) {
            *pmb++ = (char)wc;
            cBytes++;
        } else
        if ( wc < 0x00000800 ) {
            *pmb++ = (char)(0xC0 | ((wc >>  6) & 0x1F));
            cBytes++;
            *pmb++ = (char)(0x80 |  (wc        & 0x3F));
            cBytes++;
        } else
        if ( wc < 0x00010000 ) {
            *pmb++ = (char)(0xE0 | ((wc >> 12) & 0x0F));
            cBytes++;
            *pmb++ = (char)(0x80 | ((wc >>  6) & 0x3F));
            cBytes++;
            *pmb++ = (char)(0x80 |  (wc        & 0x3F));
            cBytes++;
        }
        if ( wc == L'\0' )
            return cBytes;
    }

    return cBytes;
}

/* Convert UTF-8 coded string to UNICODE
   Return number of characters converted */
int utf8ToUnicode ( const char* lpMultiByteStr, WCHAR* lpWideCharStr, int cmbChars )
{
    const unsigned char*    pmb = (unsigned char  *)lpMultiByteStr;
    unsigned short*         pwc = (unsigned short *)lpWideCharStr;
    const unsigned char*    pmbe;
    size_t  cwChars = 0;

    if ( cmbChars >= 0 ) {
        pmbe = pmb + cmbChars;
    } else {
        pmbe = (unsigned char *)((size_t)-1);
    }

    while ( pmb < pmbe ) {
        char            mb = *pmb++;
        unsigned int    cc = 0;
        unsigned int    wc;

        while ( (cc < 7) && (mb & (1 << (7 - cc)))) {
            cc++;
        }

        if ( cc == 1 || cc > 6 )                    // illegal character combination for UTF-8
            continue;

        if ( cc == 0 ) {
            wc = mb;
        } else {
            wc = (mb & ((1 << (7 - cc)) - 1)) << ((cc - 1) * 6);
            while ( --cc > 0 ) {
                if ( pmb == pmbe )                  // reached end of the buffer
                    return cwChars;
                mb = *pmb++;
                if ( ((mb >> 6) & 0x03) != 2 )      // not part of multibyte character
                    return cwChars;
                wc |= (mb & 0x3F) << ((cc - 1) * 6);
            }
        }

        if ( wc & 0xFFFF0000 )
            wc = L'?';
        *pwc++ = wc;
        cwChars++;
        if ( wc == L'\0' )
            return cwChars;
    }

    return cwChars;
}

/* convert Windows ANSI to UTF-8 */
int ConvertANSIToUTF8 ( const char* ansi, char* utf8 )
{
    WCHAR*  wszValue;          // Unicode value
    size_t  ansi_len;
    size_t  len;

    *utf8 = '\0';
    if ( ansi == NULL )
        return 0;

    ansi_len = strlen ( ansi );

    if ( (wszValue = (WCHAR *)malloc ( (ansi_len + 1) * 2 )) == NULL )
        return 0;

    /* Convert ANSI value to Unicode */
    if ( (len = MultiByteToWideChar ( CP_ACP, 0, ansi, ansi_len + 1, wszValue, (ansi_len + 1) * 2 )) == 0 ) {
        free ( wszValue );
        return 0;
    }

    /* Convert Unicode value to UTF-8 */
    if ( (len = unicodeToUtf8 ( wszValue, utf8, -1 )) == 0 ) {
        free ( wszValue );
        return 0;
    }

    free ( wszValue );

    return len-1;
}

/* convert UTF-8 to Windows ANSI */
int ConvertUTF8ToANSI ( const char* utf8, char* ansi )
{
    WCHAR*  wszValue;          // Unicode value
    size_t  utf8_len;
    size_t  len;

    *ansi = '\0';
    if ( utf8 == NULL )
        return 0;

    utf8_len = strlen ( utf8 );

    if ( (wszValue = (WCHAR *)malloc ( (utf8_len + 1) * 2 )) == NULL )
        return 0;

    /* Convert UTF-8 value to Unicode */
    if ( (len = utf8ToUnicode ( utf8, wszValue, utf8_len + 1 )) == 0 ) {
        free ( wszValue );
        return 0;
    }

    /* Convert Unicode value to ANSI */
    if ( (len = WideCharToMultiByte ( CP_ACP, 0, wszValue, -1, ansi, (utf8_len + 1) * 2, NULL, NULL )) == 0 ) {
        free ( wszValue );
        return 0;
    }

    free ( wszValue );

    return len-1;
}

BOOL uSetDlgItemText(HWND hwnd, int id, const char *str)
{
    char *temp;
    size_t len;
    int r;

    if (!str) return FALSE;
    len = strlen(str);
    temp = malloc(len+1);
    if (!temp) return FALSE;
    r = ConvertUTF8ToANSI(str, temp);
    if (r > 0)
        SetDlgItemText(hwnd, id, temp);
    free(temp);

    return r>0 ? TRUE : FALSE;
}

UINT uGetDlgItemText(HWND hwnd, int id, char *str, int max)
{
    char *temp, *utf8;
    int len;
    HWND w;

    if (!str) return 0;
    w = GetDlgItem(hwnd, id);
    len = GetWindowTextLength(w);
    temp = malloc(len+1);
    if (!temp) return 0;
    utf8 = malloc((len+1)*4);
    if (!utf8)
    {
        free(temp);
        return 0;
    }

    len = GetWindowText(w, temp, len+1);
    if (len > 0)
    {
        len = ConvertANSIToUTF8(temp, utf8);
        if (len > max-1)
        {
            len = max-1;
            utf8[max] = '\0';
        }
        memcpy(str, utf8, len+1);
    }

    free(temp);
    free(utf8);

    return len;
}

BOOL CALLBACK mp4_info_dialog_proc(HWND hwndDlg, UINT message,
                                   WPARAM wParam, LPARAM lParam)
{
    char *file_info;
    MP4FileHandle file;
    char *pVal, dummy1[1024], dummy3;
    short dummy, dummy2;

#ifdef DEBUG_OUTPUT
    in_mp4_DebugOutput("mp4_info_dialog_proc");
#endif

    switch (message) {
    case WM_INITDIALOG:
        EnableWindow(GetDlgItem(hwndDlg,IDC_CONVERT), FALSE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_CONVERT), SW_HIDE);
        EnableWindow(GetDlgItem(hwndDlg,IDC_CONVERT1), FALSE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_CONVERT1), SW_HIDE);
        EnableWindow(GetDlgItem(hwndDlg,IDC_CONVERT2), FALSE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_CONVERT2), SW_HIDE);


        file = MP4Read(info_fn, 0);

        if (file == MP4_INVALID_FILE_HANDLE)
            return FALSE;

        file_info = MP4Info(file, MP4_INVALID_TRACK_ID);
        SetDlgItemText(hwndDlg, IDC_INFOTEXT, file_info);
        free(file_info);

        /* get Metadata */

        pVal = NULL;
        MP4GetMetadataName(file, &pVal);
        uSetDlgItemText(hwndDlg,IDC_METANAME, pVal);

        pVal = NULL;
        MP4GetMetadataArtist(file, &pVal);
        uSetDlgItemText(hwndDlg,IDC_METAARTIST, pVal);

        pVal = NULL;
        MP4GetMetadataWriter(file, &pVal);
        uSetDlgItemText(hwndDlg,IDC_METAWRITER, pVal);

        pVal = NULL;
        MP4GetMetadataComment(file, &pVal);
        uSetDlgItemText(hwndDlg,IDC_METACOMMENTS, pVal);

        pVal = NULL;
        MP4GetMetadataAlbum(file, &pVal);
        uSetDlgItemText(hwndDlg,IDC_METAALBUM, pVal);

        pVal = NULL;
        MP4GetMetadataGenre(file, &pVal);
        uSetDlgItemText(hwndDlg,IDC_METAGENRE, pVal);

        dummy = 0;
        MP4GetMetadataTempo(file, &dummy);
        wsprintf(dummy1, "%d", dummy);
        SetDlgItemText(hwndDlg,IDC_METATEMPO, dummy1);

        dummy = 0; dummy2 = 0;
        MP4GetMetadataTrack(file, &dummy, &dummy2);
        wsprintf(dummy1, "%d", dummy);
        SetDlgItemText(hwndDlg,IDC_METATRACK1, dummy1);
        wsprintf(dummy1, "%d", dummy2);
        SetDlgItemText(hwndDlg,IDC_METATRACK2, dummy1);

        dummy = 0; dummy2 = 0;
        MP4GetMetadataDisk(file, &dummy, &dummy2);
        wsprintf(dummy1, "%d", dummy);
        SetDlgItemText(hwndDlg,IDC_METADISK1, dummy1);
        wsprintf(dummy1, "%d", dummy2);
        SetDlgItemText(hwndDlg,IDC_METADISK2, dummy1);

        pVal = NULL;
        MP4GetMetadataYear(file, &pVal);
        uSetDlgItemText(hwndDlg,IDC_METAYEAR, pVal);

        dummy3 = 0;
        MP4GetMetadataCompilation(file, &dummy3);
        if (dummy3)
            SendMessage(GetDlgItem(hwndDlg, IDC_METACOMPILATION), BM_SETCHECK, BST_CHECKED, 0);

        /* ! Metadata */

        MP4Close(file);

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hwndDlg, wParam);
            return TRUE;
        case IDOK:

            /* save Metadata changes */

            file = MP4Modify(info_fn, 0, 0);
            if (file == MP4_INVALID_FILE_HANDLE)
            {
                EndDialog(hwndDlg, wParam);
                return FALSE;
            }

            uGetDlgItemText(hwndDlg, IDC_METANAME, dummy1, 1024);
            MP4SetMetadataName(file, dummy1);

            uGetDlgItemText(hwndDlg, IDC_METAWRITER, dummy1, 1024);
            MP4SetMetadataWriter(file, dummy1);

            uGetDlgItemText(hwndDlg, IDC_METAARTIST, dummy1, 1024);
            MP4SetMetadataArtist(file, dummy1);

            uGetDlgItemText(hwndDlg, IDC_METAALBUM, dummy1, 1024);
            MP4SetMetadataAlbum(file, dummy1);

            uGetDlgItemText(hwndDlg, IDC_METACOMMENTS, dummy1, 1024);
            MP4SetMetadataComment(file, dummy1);

            uGetDlgItemText(hwndDlg, IDC_METAGENRE, dummy1, 1024);
            MP4SetMetadataGenre(file, dummy1);

            uGetDlgItemText(hwndDlg, IDC_METAYEAR, dummy1, 1024);
            MP4SetMetadataYear(file, dummy1);

            GetDlgItemText(hwndDlg, IDC_METATRACK1, dummy1, 1024);
            dummy = atoi(dummy1);
            GetDlgItemText(hwndDlg, IDC_METATRACK2, dummy1, 1024);
            dummy2 = atoi(dummy1);
            MP4SetMetadataTrack(file, dummy, dummy2);

            GetDlgItemText(hwndDlg, IDC_METADISK1, dummy1, 1024);
            dummy = atoi(dummy1);
            GetDlgItemText(hwndDlg, IDC_METADISK2, dummy1, 1024);
            dummy2 = atoi(dummy1);
            MP4SetMetadataDisk(file, dummy, dummy2);

            GetDlgItemText(hwndDlg, IDC_METATEMPO, dummy1, 1024);
            dummy = atoi(dummy1);
            MP4SetMetadataTempo(file, dummy);

            dummy3 = SendMessage(GetDlgItem(hwndDlg, IDC_METACOMPILATION), BM_GETCHECK, 0, 0);
            MP4SetMetadataCompilation(file, dummy3);

            MP4Close(file);

            MP4Optimize(info_fn, NULL, 0);
            /* ! */

            EndDialog(hwndDlg, wParam);
            return TRUE;
        }
    }
    return FALSE;
}

/* returns the name of the object type */
char *get_ot_string(int ot)
{
    switch (ot)
    {
    case 0:
        return "Main";
    case 1:
        return "LC";
    case 2:
        return "SSR";
    case 3:
        return "LTP";
    }
    return NULL;
}

BOOL CALLBACK aac_info_dialog_proc(HWND hwndDlg, UINT message,
                                   WPARAM wParam, LPARAM lParam)
{
    faadAACInfo aacInfo;
    char *info_text, *header_string;

#ifdef DEBUG_OUTPUT
    in_mp4_DebugOutput("aac_info_dialog_proc");
#endif

    switch (message) {
    case WM_INITDIALOG:
        EnableWindow(GetDlgItem(hwndDlg,IDC_USERDATA), FALSE) ;
        ShowWindow(GetDlgItem(hwndDlg,IDC_USERDATA), SW_HIDE);

        ShowWindow(GetDlgItem(hwndDlg,IDC_STATIC1), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_STATIC2), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_STATIC3), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_STATIC4), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_STATIC5), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_STATIC6), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_STATIC7), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_STATIC8), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_STATIC9), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_STATIC10), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_STATIC11), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_STATIC12), SW_HIDE);

        ShowWindow(GetDlgItem(hwndDlg,IDC_METANAME), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_METAARTIST), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_METAWRITER), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_METACOMMENTS), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_METAALBUM), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_METAGENRE), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_METATEMPO), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_METATRACK1), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_METATRACK2), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_METADISK1), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_METADISK2), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_METAYEAR), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_METACOMPILATION), SW_HIDE);

        info_text = malloc(1024*sizeof(char));

        get_AAC_format(info_fn, &aacInfo);

        switch (aacInfo.headertype)
        {
        case 0: /* RAW */
            header_string = " RAW";
            break;
        case 1: /* ADIF */
            header_string = " ADIF";
            break;
        case 2: /* ADTS */
            header_string = " ADTS";
            break;
        }

        sprintf(info_text, "%s AAC %s%s, %d sec, %d kbps, %d Hz",
            (aacInfo.version==2)?"MPEG-2":"MPEG-4", get_ot_string(aacInfo.object_type),
            header_string,
            (int)((float)aacInfo.length/1000.0), (int)((float)aacInfo.bitrate/1000.0+0.5),
            aacInfo.sampling_rate);

        SetDlgItemText(hwndDlg, IDC_INFOTEXT, info_text);

        free(info_text);

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_CONVERT:
            {
                char mp4FileName[256];
                char *extension;
                OPENFILENAME ofn;

                lstrcpy(mp4FileName, info_fn);
                extension = strrchr(mp4FileName, '.');
                lstrcpy(extension, ".mp4");

                memset(&ofn, 0, sizeof(OPENFILENAME));
                ofn.lStructSize = sizeof(OPENFILENAME);
                ofn.hwndOwner = hwndDlg;
                ofn.hInstance = module.hDllInstance;
                ofn.nMaxFileTitle = 31;
                ofn.lpstrFile = (LPSTR)mp4FileName;
                ofn.nMaxFile = _MAX_PATH;
                ofn.lpstrFilter = "MP4 Files (*.mp4)\0*.mp4\0";
                ofn.lpstrDefExt = "mp4";
                ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
                ofn.lpstrTitle = "Select Output File";

                if (GetSaveFileName(&ofn))
                {
                    if (covert_aac_to_mp4(info_fn, mp4FileName))
                    {
                        MessageBox(hwndDlg, "An error occured while converting AAC to MP4!", "An error occured!", MB_OK);
                        return FALSE;
                    }
                }
                return TRUE;
            }
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
    if(!stricmp(fn + strlen(fn) - 3,"AAC"))
    {
        lstrcpy(info_fn, fn);

        DialogBox(module.hDllInstance, MAKEINTRESOURCE(IDD_INFO),
            hwndParent, aac_info_dialog_proc);
    } else {
        lstrcpy(info_fn, fn);

        DialogBox(module.hDllInstance, MAKEINTRESOURCE(IDD_INFO),
            hwndParent, mp4_info_dialog_proc);
    }

    return 0;
}

/* Get the title from the file */
void ConstructTitle(MP4FileHandle file, char *filename, char *title, char *format)
{
    char temp[4096];
    int some_info = 0;
    char *in = format;
    char *out = temp;//title;
    char *bound = out + sizeof(temp) - 256; //out + (MAX_PATH - 10 - 1);
    char *pVal, dummy1[1024];
    short dummy, dummy2;


    while (*in && out < bound)
    {
        switch (*in)
        {
        case '%':
            ++in;
            break;

        default:
            *out++ = *in++;
            continue;
        }

        /* handle % escape sequence */
        switch (*in++)
        {
        case '0':
            dummy = 0; dummy2 = 0;
            if (MP4GetMetadataTrack(file, &dummy, &dummy2))
            {
                out += wsprintf(out, "%d", (int)dummy);
                some_info = 1;
            }
            break;

        case '1':
            pVal = NULL;
            if (MP4GetMetadataArtist(file, &pVal))
            {
                out += wsprintf(out, "%s", pVal);
                some_info = 1;
            }
            break;

        case '2':
            pVal = NULL;
            if (MP4GetMetadataName(file, &pVal))
            {
                out += wsprintf(out, "%s", pVal);
                some_info = 1;
            }
            break;

        case '3':
            pVal = NULL;
            if (MP4GetMetadataAlbum(file, &pVal))
            {
                out += wsprintf(out, "%s", pVal);
                some_info = 1;
            }
            break;

        case '4':
            pVal = NULL;
            if (MP4GetMetadataYear(file, &pVal))
            {
                out += wsprintf(out, "%s", pVal);
                some_info = 1;
            }
            break;

        case '5':
            pVal = NULL;
            if (MP4GetMetadataComment(file, &pVal))
            {
                out += wsprintf(out, "%s", pVal);
                some_info = 1;
            }
            break;

        case '6':
            pVal = NULL;
            if (MP4GetMetadataGenre(file, &pVal))
            {
                out += wsprintf(out, "%s", pVal);
                some_info = 1;
            }
            break;

        case '7':
            {
                const char *p=strrchr(filename,'\\');
                if (!p) p=filename; else p++;
                out += ConvertANSIToUTF8(p, out);
                some_info = 1;
                break;
            }

        default:
            break;
        }
    }

    *out = '\0';

    if (!some_info)
    {
        char *p=filename+lstrlen(filename);
        while (*p != '\\' && p >= filename) p--;
        lstrcpy(title,++p);
    }
    else
    {
        int len = ConvertUTF8ToANSI(temp, dummy1);
        if (len > (MAX_PATH - 10 - 1)) len = (MAX_PATH - 10 - 1);
        memcpy(title, dummy1, len);
        title[len] = '\0';
    }
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
        if (m_use_for_aac)
            SendMessage(GetDlgItem(hwndDlg, IDC_USEFORAAC), BM_SETCHECK, BST_CHECKED, 0);
        if (m_downmix)
            SendMessage(GetDlgItem(hwndDlg, IDC_DOWNMIX), BM_SETCHECK, BST_CHECKED, 0);
        if (m_vbr_display)
            SendMessage(GetDlgItem(hwndDlg, IDC_VBR), BM_SETCHECK, BST_CHECKED, 0);
        SetDlgItemText(hwndDlg, IDC_TITLEFORMAT, titleformat);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hwndDlg, wParam);
            return TRUE;
        case IDOK:
            m_show_errors = SendMessage(GetDlgItem(hwndDlg, IDC_ERROR), BM_GETCHECK, 0, 0);
            m_use_for_aac = SendMessage(GetDlgItem(hwndDlg, IDC_USEFORAAC), BM_GETCHECK, 0, 0);
            m_downmix = SendMessage(GetDlgItem(hwndDlg, IDC_DOWNMIX), BM_GETCHECK, 0, 0);
            m_vbr_display = SendMessage(GetDlgItem(hwndDlg, IDC_VBR), BM_GETCHECK, 0, 0);
            GetDlgItemText(hwndDlg, IDC_TITLEFORMAT, titleformat, MAX_PATH);

            m_priority = SendMessage(GetDlgItem(hwndDlg, IDC_PRIORITY), TBM_GETPOS, 0, 0);
            for (i = 0; i < 6; i++)
            {
                if (SendMessage(GetDlgItem(hwndDlg, res_id_table[i]), BM_GETCHECK, 0, 0))
                {
                    m_resolution = i;
                    break;
                }
            }

            /* save config */
            config_write();

            if (!m_use_for_aac)
            {
                module.FileExtensions = short_ext_list;
            } else {
                module.FileExtensions = long_ext_list;
            }

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
        "AudioCoding.com MPEG-4 General Audio player " FAAD2_VERSION " compiled on " __DATE__ ".\n"
        "Visit the website for more info.\n"
        "Copyright 2002-2003 AudioCoding.com",
        "About",
        MB_OK);
}

int isourfile(char *fn)
{
    if (!stricmp(fn + strlen(fn) - 3,"MP4") || !stricmp(fn + strlen(fn) - 3,"M4A"))
    {
        return 1;
    } else if (m_use_for_aac) {
        if (!stricmp(fn + strlen(fn) - 3,"AAC"))
        {
            return 1;
        }
    }

    return 0;
}

int fill_buffer(state *st)
{
    int bread;

    if (st->m_aac_bytes_consumed > 0)
    {
        if (st->m_aac_bytes_into_buffer)
        {
            memmove((void*)st->m_aac_buffer, (void*)(st->m_aac_buffer + st->m_aac_bytes_consumed),
                st->m_aac_bytes_into_buffer*sizeof(unsigned char));
        }

        if (!st->m_at_eof)
        {
            bread = fread((void*)(st->m_aac_buffer + st->m_aac_bytes_into_buffer),
                1, st->m_aac_bytes_consumed, st->aacfile);

            if (bread != st->m_aac_bytes_consumed)
                st->m_at_eof = 1;

            st->m_aac_bytes_into_buffer += bread;
        }

        st->m_aac_bytes_consumed = 0;

        if (st->m_aac_bytes_into_buffer > 3)
        {
            if (memcmp(st->m_aac_buffer, "TAG", 3) == 0)
                st->m_aac_bytes_into_buffer = 0;
        }
        if (st->m_aac_bytes_into_buffer > 11)
        {
            if (memcmp(st->m_aac_buffer, "LYRICSBEGIN", 11) == 0)
                st->m_aac_bytes_into_buffer = 0;
        }
        if (st->m_aac_bytes_into_buffer > 8)
        {
            if (memcmp(st->m_aac_buffer, "APETAGEX", 8) == 0)
                st->m_aac_bytes_into_buffer = 0;
        }
    }

    return 1;
}

void advance_buffer(state *st, int bytes)
{
    st->m_file_offset += bytes;
    st->m_aac_bytes_consumed = bytes;
    st->m_aac_bytes_into_buffer -= bytes;
}

int adts_parse(state *st, __int64 *bitrate, double *length)
{
    static int sample_rates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000};
    int frames, frame_length;
    int t_framelength = 0;
    int samplerate;
    double frames_per_sec, bytes_per_frame;

    /* Read all frames to ensure correct time and bitrate */
    for (frames = 0; /* */; frames++)
    {
        fill_buffer(st);

        if (st->m_aac_bytes_into_buffer > 7)
        {
            /* check syncword */
            if (!((st->m_aac_buffer[0] == 0xFF)&&((st->m_aac_buffer[1] & 0xF6) == 0xF0)))
                break;

            st->m_tail->offset = st->m_file_offset;
            st->m_tail->next = (struct seek_list*)malloc(sizeof(struct seek_list));
            st->m_tail = st->m_tail->next;
            st->m_tail->next = NULL;

            if (frames == 0)
                samplerate = sample_rates[(st->m_aac_buffer[2]&0x3c)>>2];

            frame_length = ((((unsigned int)st->m_aac_buffer[3] & 0x3)) << 11)
                | (((unsigned int)st->m_aac_buffer[4]) << 3) | (st->m_aac_buffer[5] >> 5);

            t_framelength += frame_length;

            if (frame_length > st->m_aac_bytes_into_buffer)
                break;

            advance_buffer(st, frame_length);
        } else {
            break;
        }
    }

    frames_per_sec = (double)samplerate/1024.0;
    if (frames != 0)
        bytes_per_frame = (double)t_framelength/(double)(frames*1000);
    else
        bytes_per_frame = 0;
    *bitrate = (__int64)(8. * bytes_per_frame * frames_per_sec + 0.5);
    if (frames_per_sec != 0)
        *length = (double)frames/frames_per_sec;
    else
        *length = 1;

    return 1;
}

int skip_id3v2_tag()
{
    unsigned char buf[10];
    int bread, tagsize = 0;

    bread = fread(buf, 1, 10, mp4state.aacfile);
    if (bread != 10) return -1;

    if (!memcmp(buf, "ID3", 3))
    {
        /* high bit is not used */
        tagsize = (buf[6] << 21) | (buf[7] << 14) | (buf[8] << 7) | (buf[9] << 0);

        tagsize += 10;
        fseek(mp4state.aacfile, tagsize, SEEK_SET);
    } else {
        fseek(mp4state.aacfile, 0, SEEK_SET);
    }

    return tagsize;
}

int play(char *fn)
{
    int maxlatency;
    int thread_id;
    int avg_bitrate, br, sr;
    unsigned char *buffer;
    int buffer_size;
    faacDecConfigurationPtr config;

#ifdef DEBUG_OUTPUT
    in_mp4_DebugOutput("play");
#endif

    memset(&mp4state, 0, sizeof(state));

    lstrcpy(mp4state.filename, fn);

    if (!(mp4state.mp4file = MP4Read(mp4state.filename, 0)))
    {
        mp4state.filetype = 1;
    } else {
        MP4Close(mp4state.mp4file);
        mp4state.filetype = 0;
    }

    if (mp4state.filetype)
    {
        int tagsize = 0, tmp = 0, init;
        int bread = 0;
        double length = 0.;
        __int64 bitrate = 128;
        faacDecFrameInfo frameInfo;

        module.is_seekable = 1;

        if (!(mp4state.aacfile = fopen(mp4state.filename, "rb")))
        {
            // error
            return -1;
        }

        tagsize = skip_id3v2_tag();
        if (tagsize<0) return 0;

        if (!(mp4state.m_aac_buffer = (unsigned char*)malloc(768*6)))
        {
            show_error(module.hMainWindow, "Memory allocation error.");
            return -1;
        }

        for (init=0; init<2; init++)
        {
            mp4state.hDecoder = faacDecOpen();
            if (!mp4state.hDecoder)
            {
                show_error(module.hMainWindow, "Unable to open decoder library.");
                return -1;
            }

            config = faacDecGetCurrentConfiguration(mp4state.hDecoder);
            config->outputFormat = m_resolution + 1;
            config->downMatrix = m_downmix;
            faacDecSetConfiguration(mp4state.hDecoder, config);

            memset(mp4state.m_aac_buffer, 0, 768*6);
            bread = fread(mp4state.m_aac_buffer, 1, 768*6, mp4state.aacfile);
            mp4state.m_aac_bytes_into_buffer = bread;
            mp4state.m_aac_bytes_consumed = 0;
            mp4state.m_file_offset = 0;
            mp4state.m_at_eof = (bread != 768*6) ? 1 : 0;

            if (init==0)
            {
                faacDecFrameInfo frameInfo;

                fill_buffer(&mp4state);

                if ((mp4state.m_aac_bytes_consumed = faacDecInit(mp4state.hDecoder,
                    mp4state.m_aac_buffer, mp4state.m_aac_bytes_into_buffer,
                    &mp4state.samplerate, &mp4state.channels)) < 0)
                {
                    show_error(module.hMainWindow, "Can't initialize decoder library.");
                    return -1;
                }
                advance_buffer(&mp4state, mp4state.m_aac_bytes_consumed);

                do {
                    memset(&frameInfo, 0, sizeof(faacDecFrameInfo));
                    fill_buffer(&mp4state);
                    faacDecDecode(mp4state.hDecoder, &frameInfo, mp4state.m_aac_buffer, mp4state.m_aac_bytes_into_buffer);
                } while (!frameInfo.samples && !frameInfo.error);

                if (frameInfo.error)
                {
                    show_error(module.hMainWindow, faacDecGetErrorMessage(frameInfo.error));
                    return -1;
                }

                mp4state.channels = frameInfo.channels;
                mp4state.samplerate = frameInfo.samplerate;
                mp4state.framesize = (frameInfo.channels != 0) ? frameInfo.samples/frameInfo.channels : 0;
                /*
                sbr = frameInfo.sbr;
                profile = frameInfo.object_type;
                header_type = frameInfo.header_type;
                */

                faacDecClose(mp4state.hDecoder);
                fseek(mp4state.aacfile, tagsize, SEEK_SET);
            }
        }

        mp4state.m_head = (struct seek_list*)malloc(sizeof(struct seek_list));
        mp4state.m_tail = mp4state.m_head;
        mp4state.m_tail->next = NULL;

        mp4state.m_header_type = 0;
        if ((mp4state.m_aac_buffer[0] == 0xFF) && ((mp4state.m_aac_buffer[1] & 0xF6) == 0xF0))
        {
            if (1) //(can_seek)
            {
                adts_parse(&mp4state, &bitrate, &length);
                fseek(mp4state.aacfile, tagsize, SEEK_SET);

                bread = fread(mp4state.m_aac_buffer, 1, 768*6, mp4state.aacfile);
                if (bread != 768*6)
                    mp4state.m_at_eof = 1;
                else
                    mp4state.m_at_eof = 0;
                mp4state.m_aac_bytes_into_buffer = bread;
                mp4state.m_aac_bytes_consumed = 0;

                mp4state.m_header_type = 1;
            }
        } else if (memcmp(mp4state.m_aac_buffer, "ADIF", 4) == 0) {
            int skip_size = (mp4state.m_aac_buffer[4] & 0x80) ? 9 : 0;
            bitrate = ((unsigned int)(mp4state.m_aac_buffer[4 + skip_size] & 0x0F)<<19) |
                ((unsigned int)mp4state.m_aac_buffer[5 + skip_size]<<11) |
                ((unsigned int)mp4state.m_aac_buffer[6 + skip_size]<<3) |
                ((unsigned int)mp4state.m_aac_buffer[7 + skip_size] & 0xE0);

            length = (double)file_length(mp4state.aacfile);
            if (length == -1)
            {
                module.is_seekable = 0;
                length = 0;
            } else {
                length = ((double)length*8.)/((double)bitrate) + 0.5;
            }

            mp4state.m_header_type = 2;
        } else {
            length = (double)file_length(mp4state.aacfile);
            length = ((double)length*8.)/((double)bitrate*1000.) + 0.5;

            module.is_seekable = 1;
        }

        mp4state.m_length = (int)(length*1000.);

        fill_buffer(&mp4state);
        if ((mp4state.m_aac_bytes_consumed = faacDecInit(mp4state.hDecoder,
            mp4state.m_aac_buffer, mp4state.m_aac_bytes_into_buffer,
            &mp4state.samplerate, &mp4state.channels)) < 0)
        {
            show_error(module.hMainWindow, "Can't initialize decoder library.");
            return -1;
        }
        advance_buffer(&mp4state, mp4state.m_aac_bytes_consumed);

        if (mp4state.m_header_type == 2)
            avg_bitrate = bitrate;
        else
            avg_bitrate = bitrate*1000;
    } else {
        mp4state.hDecoder = faacDecOpen();
        if (!mp4state.hDecoder)
        {
            show_error(module.hMainWindow, "Unable to open decoder library.");
            return -1;
        }

        config = faacDecGetCurrentConfiguration(mp4state.hDecoder);
        config->outputFormat = m_resolution + 1;
        config->downMatrix = m_downmix;
        faacDecSetConfiguration(mp4state.hDecoder, config);

        mp4state.mp4file = MP4Read(mp4state.filename, 0);
        if (!mp4state.mp4file)
        {
            show_error(module.hMainWindow, "Unable to open file.");
            faacDecClose(mp4state.hDecoder);
            return -1;
        }

        if ((mp4state.mp4track = GetAACTrack(mp4state.mp4file)) < 0)
        {
            show_error(module.hMainWindow, "Unsupported Audio track type.");
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
            if (buffer) free (buffer);
            return -1;
        }

        /* for gapless decoding */
        {
            mp4AudioSpecificConfig mp4ASC;

            mp4state.timescale = MP4GetTrackTimeScale(mp4state.mp4file, mp4state.mp4track);
            mp4state.framesize = 1024;
            mp4state.useAacLength = 0;

            if (buffer)
            {
                if (AudioSpecificConfig(buffer, buffer_size, &mp4ASC) >= 0)
                {
                    if (mp4ASC.frameLengthFlag == 1) mp4state.framesize = 960;
                    if (mp4ASC.sbr_present_flag == 1) mp4state.framesize *= 2;
                }
            }
        }

        free(buffer);

        avg_bitrate = MP4GetTrackIntegerProperty(mp4state.mp4file, mp4state.mp4track,
            "mdia.minf.stbl.stsd.mp4a.esds.decConfigDescr.avgBitrate");

        mp4state.numSamples = MP4GetTrackNumberOfSamples(mp4state.mp4file, mp4state.mp4track);
        mp4state.sampleId = 1;

        module.is_seekable = 1;
    }

    if (mp4state.channels == 0)
    {
        show_error(module.hMainWindow, "Number of channels not supported for playback.");
        faacDecClose(mp4state.hDecoder);
        if (mp4state.filetype)
            fclose(mp4state.aacfile);
        else
            MP4Close(mp4state.mp4file);
        return -1;
    }

    if (m_downmix && (mp4state.channels == 5 || mp4state.channels == 6))
        mp4state.channels = 2;

    maxlatency = module.outMod->Open(mp4state.samplerate, (int)mp4state.channels,
        res_table[m_resolution], -1, -1);

    if (maxlatency < 0) // error opening device
    {
        faacDecClose(mp4state.hDecoder);
        if (mp4state.filetype)
            fclose(mp4state.aacfile);
        else
            MP4Close(mp4state.mp4file);
        return -1;
    }

    mp4state.paused        =  0;
    mp4state.decode_pos_ms =  0;
    mp4state.seek_needed   = -1;

    // initialize vis stuff
    module.SAVSAInit(maxlatency, mp4state.samplerate);
    module.VSASetInfo((int)mp4state.channels, mp4state.samplerate);

    br = (int)floor(((float)avg_bitrate + 500.0)/1000.0 + 0.5);
    sr = (int)floor((float)mp4state.samplerate/1000.0 + 0.5);
    module.SetInfo(br, sr, (int)mp4state.channels, 1);

    module.outMod->SetVolume(-666); // set the output plug-ins default volume

    killPlayThread = 0;

    if (mp4state.filetype)
    {
        if ((play_thread_handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AACPlayThread,
            (void *)&killPlayThread, 0, &thread_id)) == NULL)
        {
            show_error(module.hMainWindow, "Cannot create playback thread");
            faacDecClose(mp4state.hDecoder);
            fclose(mp4state.aacfile);
            return -1;
        }
    } else {
        if ((play_thread_handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MP4PlayThread,
            (void *)&killPlayThread, 0, &thread_id)) == NULL)
        {
            show_error(module.hMainWindow, "Cannot create playback thread");
            faacDecClose(mp4state.hDecoder);
            MP4Close(mp4state.mp4file);
            return -1;
        }
    }

    SetThreadAffinityMask(play_thread_handle, 1);

    SetThreadPriority(play_thread_handle, priority_table[m_priority]);

    return 0;
}

void pause()
{
#ifdef DEBUG_OUTPUT
    in_mp4_DebugOutput("pause");
#endif

    mp4state.paused = 1;
    module.outMod->Pause(1);
}

void unpause()
{
#ifdef DEBUG_OUTPUT
    in_mp4_DebugOutput("unpause");
#endif

    mp4state.paused = 0;
    module.outMod->Pause(0);
}

int ispaused()
{
#ifdef DEBUG_OUTPUT
    in_mp4_DebugOutput("ispaused");
#endif

    return mp4state.paused;
}

void setvolume(int volume)
{
#ifdef DEBUG_OUTPUT
    in_mp4_DebugOutput("setvolume");
#endif

    module.outMod->SetVolume(volume);
}

void setpan(int pan)
{
#ifdef DEBUG_OUTPUT
    in_mp4_DebugOutput("setpan");
#endif

    module.outMod->SetPan(pan);
}

void stop()
{
    struct seek_list *target = mp4state.m_head;

#ifdef DEBUG_OUTPUT
    in_mp4_DebugOutput("stop");
#endif

    killPlayThread = 1;

    if (play_thread_handle != INVALID_HANDLE_VALUE)
    {
        if (WaitForSingleObject(play_thread_handle, INFINITE) == WAIT_TIMEOUT)
            TerminateThread(play_thread_handle,0);
        CloseHandle(play_thread_handle);
        play_thread_handle = INVALID_HANDLE_VALUE;
    }


    if (mp4state.m_aac_buffer)
        free(mp4state.m_aac_buffer);

    while (target)
    {
        struct seek_list *tmp = target;
        target = target->next;
        if (tmp) free(tmp);
    }
    faacDecClose(mp4state.hDecoder);
    if (mp4state.filetype)
        fclose(mp4state.aacfile);
    else
        MP4Close(mp4state.mp4file);

    module.outMod->Close();
    module.SAVSADeInit();
}

int getsonglength(char *fn)
{
    long msDuration = 0;

    if(!stricmp(fn + strlen(fn) - 3,"MP4") || !stricmp(fn + strlen(fn) - 3,"M4A"))
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
    } else {
        int tagsize = 0;
        int bread = 0;
        double length = 0.;
        __int64 bitrate = 128;
        struct seek_list *target;
        state len_state;

        memset(&len_state, 0, sizeof(state));

        if (!(len_state.aacfile = fopen(fn, "rb")))
        {
            // error
            return 0;
        }

        len_state.m_at_eof = 0;

        if (!(len_state.m_aac_buffer = (unsigned char*)malloc(768*6)))
        {
            //console::error("Memory allocation error.", "foo_mp4");
            return 0;
        }
        memset(len_state.m_aac_buffer, 0, 768*6);

        bread = fread(len_state.m_aac_buffer, 1, 768*6, len_state.aacfile);
        len_state.m_aac_bytes_into_buffer = bread;
        len_state.m_aac_bytes_consumed = 0;
        len_state.m_file_offset = 0;

        if (bread != 768*6)
            len_state.m_at_eof = 1;

        if (!memcmp(len_state.m_aac_buffer, "ID3", 3))
        {
            /* high bit is not used */
            tagsize = (len_state.m_aac_buffer[6] << 21) | (len_state.m_aac_buffer[7] << 14) |
                (len_state.m_aac_buffer[8] <<  7) | (len_state.m_aac_buffer[9] <<  0);

            tagsize += 10;
            advance_buffer(&len_state, tagsize);
        }

        len_state.m_head = (struct seek_list*)malloc(sizeof(struct seek_list));
        len_state.m_tail = len_state.m_head;
        len_state.m_tail->next = NULL;

        len_state.m_header_type = 0;
        if ((len_state.m_aac_buffer[0] == 0xFF) && ((len_state.m_aac_buffer[1] & 0xF6) == 0xF0))
        {
            if (1) //(m_reader->can_seek())
            {
                adts_parse(&len_state, &bitrate, &length);
                fseek(len_state.aacfile, tagsize, SEEK_SET);

                bread = fread(len_state.m_aac_buffer, 1, 768*6, len_state.aacfile);
                if (bread != 768*6)
                    len_state.m_at_eof = 1;
                else
                    len_state.m_at_eof = 0;
                len_state.m_aac_bytes_into_buffer = bread;
                len_state.m_aac_bytes_consumed = 0;

                len_state.m_header_type = 1;
            }
        } else if (memcmp(len_state.m_aac_buffer, "ADIF", 4) == 0) {
            int skip_size = (len_state.m_aac_buffer[4] & 0x80) ? 9 : 0;
            bitrate = ((unsigned int)(len_state.m_aac_buffer[4 + skip_size] & 0x0F)<<19) |
                ((unsigned int)len_state.m_aac_buffer[5 + skip_size]<<11) |
                ((unsigned int)len_state.m_aac_buffer[6 + skip_size]<<3) |
                ((unsigned int)len_state.m_aac_buffer[7 + skip_size] & 0xE0);

            length = (double)file_length(len_state.aacfile);
            if (length == -1)
                length = 0;
            else
                length = ((double)length*8.)/((double)bitrate) + 0.5;

            len_state.m_header_type = 2;
        } else {
            length = (double)file_length(len_state.aacfile);
            length = ((double)length*8.)/((double)bitrate*1000.) + 0.5;

            len_state.m_header_type = 0;
        }

        if (len_state.m_aac_buffer)
            free(len_state.m_aac_buffer);

        target = len_state.m_head;
        while (target)
        {
            struct seek_list *tmp = target;
            target = target->next;
            if (tmp) free(tmp);
        }

        fclose(len_state.aacfile);

        return (int)(length*1000.);
    }
}

int getlength()
{
    if (!mp4state.filetype)
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
    } else {
        return mp4state.m_length;
    }
    return 0;
}

int getoutputtime()
{
    return mp4state.decode_pos_ms+(module.outMod->GetOutputTime()-module.outMod->GetWrittenTime());
}

void setoutputtime(int time_in_ms)
{
#ifdef DEBUG_OUTPUT
    in_mp4_DebugOutput("setoutputtime");
#endif

    mp4state.seek_needed = time_in_ms;
}

void getfileinfo(char *filename, char *title, int *length_in_ms)
{
    if (!filename || !*filename)  /* currently playing file */
    {
        if (length_in_ms)
            *length_in_ms = getlength();

        if (title)
        {
            if (mp4state.filetype == 0)
            {
                ConstructTitle(mp4state.mp4file, mp4state.filename, title, titleformat);
            } else {
                char *tmp2;
                char *tmp = PathFindFileName(mp4state.filename);
                strcpy(title, tmp);
                tmp2 = strrchr(title, '.');
                tmp2[0] = '\0';
            }
        }
    } else {
        if (length_in_ms)
            *length_in_ms = getsonglength(filename);

        if (title)
        {
            MP4FileHandle file = MP4_INVALID_FILE_HANDLE;
            if (!(file = MP4Read(filename, 0)))
            {
                char *tmp2;
                char *tmp = PathFindFileName(filename);
                strcpy(title, tmp);
                tmp2 = strrchr(title, '.');
                tmp2[0] = '\0';
            } else {
                ConstructTitle(file, filename, title, titleformat);
                MP4Close(file);
            }
        }
    }
}

void eq_set(int on, char data[10], int preamp)
{
}

DWORD WINAPI MP4PlayThread(void *b)
{
    int done = 0;
    int l;
    int seq_frames = 0;
    int seq_bytes = 0;

    void *sample_buffer;
    unsigned char *buffer;
    int buffer_size;
    faacDecFrameInfo frameInfo;

#ifdef DEBUG_OUTPUT
    in_mp4_DebugOutput("MP4PlayThread");
#endif

    PlayThreadAlive = 1;
    mp4state.last_frame = 0;
    mp4state.initial = 1;

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
        } else if (module.outMod->CanWrite() >= (2048*mp4state.channels*sizeof(short))) {

            if (mp4state.last_frame)
            {
                done = 1;
            } else {
                int rc;

                /* for gapless decoding */
                char *buf;
                MP4Duration dur;
                unsigned int sample_count;
                unsigned int delay = 0;

                /* get acces unit from MP4 file */
                buffer = NULL;
                buffer_size = 0;

                rc = MP4ReadSample(mp4state.mp4file, mp4state.mp4track,
                    mp4state.sampleId++, &buffer, &buffer_size,
                    NULL, &dur, NULL, NULL);
                if (mp4state.sampleId-1 == 1) dur = 0;
                if (rc == 0 || buffer == NULL)
                {
                    mp4state.last_frame = 1;
                    sample_buffer = NULL;
                    frameInfo.samples = 0;
                } else {
                    sample_buffer = faacDecDecode(mp4state.hDecoder, &frameInfo,
                        buffer, buffer_size);
                }
                if (frameInfo.error > 0)
                {
                    show_error(module.hMainWindow, faacDecGetErrorMessage(frameInfo.error));
                    mp4state.last_frame = 1;
                }
                if (mp4state.sampleId > mp4state.numSamples)
                    mp4state.last_frame = 1;

                if (buffer) free(buffer);

                if (mp4state.useAacLength || (mp4state.timescale != mp4state.samplerate)) {
                    sample_count = frameInfo.samples;
                } else {
                    sample_count = (unsigned int)(dur * frameInfo.channels);

                    if (!mp4state.useAacLength && !mp4state.initial && (mp4state.sampleId < mp4state.numSamples/2) && (dur*frameInfo.channels != frameInfo.samples))
                    {
                        //fprintf(stderr, "MP4 seems to have incorrect frame duration, using values from AAC data.\n");
                        mp4state.useAacLength = 1;
                        sample_count = frameInfo.samples;
                    }
                }

                if (mp4state.initial && (sample_count < mp4state.framesize*mp4state.channels) && (frameInfo.samples > sample_count))
                {
                    delay = frameInfo.samples - sample_count;
                }

                if (!killPlayThread && (sample_count > 0))
                {
                    buf = (char *)sample_buffer;
                    mp4state.initial = 0;

                    switch (res_table[m_resolution])
                    {
                    case 8:
                        buf += delay;
                        break;
                    case 16:
                    default:
                        buf += delay * 2;
                        break;
                    case 24:
                    case 32:
                        buf += delay * 4;
                        break;
                    case 64:
                        buf += delay * 8;
                    }

                    if (res_table[m_resolution] == 24)
                    {
                        /* convert libfaad output (3 bytes packed in 4) */
                        char *temp_buffer = convert3in4to3in3(buf, sample_count);
                        memcpy((void*)buf, (void*)temp_buffer, sample_count*3);
                        free(temp_buffer);
                    }

                    module.SAAddPCMData(buf, (int)mp4state.channels, res_table[m_resolution],
                        mp4state.decode_pos_ms);
                    module.VSAAddPCMData(buf, (int)mp4state.channels, res_table[m_resolution],
                        mp4state.decode_pos_ms);
                    mp4state.decode_pos_ms += (double)sample_count * 1000.0 /
                        ((double)frameInfo.samplerate * (double)frameInfo.channels);

                    l = sample_count * res_table[m_resolution] / 8;

                    if (module.dsp_isactive())
                    {
                        void *dsp_buffer = malloc(l*2);
                        memcpy(dsp_buffer, buf, l);

                        l = module.dsp_dosamples((short*)dsp_buffer,
                            sample_count/frameInfo.channels,
                            res_table[m_resolution],
                            frameInfo.channels,
                            frameInfo.samplerate) *
                            (frameInfo.channels*(res_table[m_resolution]/8));

                        module.outMod->Write(dsp_buffer, l);
                        if (dsp_buffer) free(dsp_buffer);
                    } else {
                        module.outMod->Write(buf, l);
                    }

                    /* VBR bitrate display */
                    if (m_vbr_display)
                    {
                        seq_frames++;
                        seq_bytes += frameInfo.bytesconsumed;
                        if (seq_frames == (int)(floor((float)frameInfo.samplerate/(float)(sample_count/frameInfo.channels) + 0.5)))
                        {
                            module.SetInfo((int)floor(((float)seq_bytes*8.)/1000. + 0.5),
                                (int)floor(frameInfo.samplerate/1000. + 0.5),
                                mp4state.channels, 1);

                            seq_frames = 0;
                            seq_bytes = 0;
                        }
                    }
                }
            }
        } else {
            Sleep(10);
        }
    }

    PlayThreadAlive = 0;

    return 0;
}

void *decode_aac_frame(state *st, faacDecFrameInfo *frameInfo)
{
    void *sample_buffer;

    do
    {
        fill_buffer(st);

        if (st->m_aac_bytes_into_buffer != 0)
        {
            sample_buffer = faacDecDecode(st->hDecoder, frameInfo,
                st->m_aac_buffer, st->m_aac_bytes_into_buffer);

            if (st->m_header_type != 1)
            {
                st->m_tail->offset = st->m_file_offset;
                st->m_tail->next = (struct seek_list*)malloc(sizeof(struct seek_list));
                st->m_tail = st->m_tail->next;
                st->m_tail->next = NULL;
            }

            advance_buffer(st, frameInfo->bytesconsumed);
        } else {
            break;
        }

    } while (!frameInfo->samples && !frameInfo->error);

    return sample_buffer;
}

int aac_seek(state *st, double seconds)
{
    int i, frames;
    int bread;
    struct seek_list *target = st->m_head;

    if (1 /*can_seek*/ && ((st->m_header_type == 1) || (seconds < st->cur_pos_sec)))
    {
        frames = (int)(seconds*((double)st->samplerate/(double)st->framesize) + 0.5);

        for (i = 0; i < frames; i++)
        {
            if (target->next)
                target = target->next;
            else
                return 0;
        }
        if (target->offset == 0 && frames > 0)
            return 0;
        fseek(st->aacfile, target->offset, SEEK_SET);

        bread = fread(st->m_aac_buffer, 1, 768*6, st->aacfile);
        if (bread != 768*6)
            st->m_at_eof = 1;
        else
            st->m_at_eof = 0;
        st->m_aac_bytes_into_buffer = bread;
        st->m_aac_bytes_consumed = 0;

        return 1;
    } else {
        if (seconds > st->cur_pos_sec)
        {
            faacDecFrameInfo frameInfo;

            frames = (int)((seconds - st->cur_pos_sec)*((double)st->samplerate/(double)st->framesize));

            if (frames > 0)
            {
                for (i = 0; i < frames; i++)
                {
                    memset(&frameInfo, 0, sizeof(faacDecFrameInfo));
                    decode_aac_frame(st, &frameInfo);

                    if (frameInfo.error || (st->m_aac_bytes_into_buffer == 0))
                    {
                        if (frameInfo.error)
                        {
                            if (faacDecGetErrorMessage(frameInfo.error) != NULL)
                                show_error(module.hMainWindow, faacDecGetErrorMessage(frameInfo.error));
                        }
                        return 0;
                    }
                }
            }

            return 1;
        } else {
            return 0;
        }
    }
}

DWORD WINAPI AACPlayThread(void *b)
{
    int done = 0;
    int l;
    int seq_frames = 0;
    int seq_bytes = 0;

#ifdef DEBUG_OUTPUT
    in_mp4_DebugOutput("AACPlayThread");
#endif

    PlayThreadAlive = 1;
    mp4state.last_frame = 0;

    while (!*((int *)b))
    {
        /* seeking */
        if (mp4state.seek_needed != -1)
        {
            double ms;

            ms = mp4state.seek_needed/1000;
            if (aac_seek(&mp4state, ms)!=0)
            {
                module.outMod->Flush(mp4state.decode_pos_ms);
                mp4state.cur_pos_sec = ms;
                mp4state.decode_pos_ms = mp4state.seek_needed;
            }
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
        } else if (module.outMod->CanWrite() >= (2048*mp4state.channels*sizeof(short))) {
            faacDecFrameInfo frameInfo;
            void *sample_buffer;

            memset(&frameInfo, 0, sizeof(faacDecFrameInfo));

            sample_buffer = decode_aac_frame(&mp4state, &frameInfo);

            if (frameInfo.error || (mp4state.m_aac_bytes_into_buffer == 0))
            {
                if (frameInfo.error)
                {
                    if (faacDecGetErrorMessage(frameInfo.error) != NULL)
                        show_error(module.hMainWindow, faacDecGetErrorMessage(frameInfo.error));
                }
                done = 1;
            }

            if (!killPlayThread && (frameInfo.samples > 0))
            {
                if (res_table[m_resolution] == 24)
                {
                    /* convert libfaad output (3 bytes packed in 4 bytes) */
                    char *temp_buffer = convert3in4to3in3(sample_buffer, frameInfo.samples);
                    memcpy((void*)sample_buffer, (void*)temp_buffer, frameInfo.samples*3);
                    free(temp_buffer);
                }

                module.SAAddPCMData(sample_buffer, (int)mp4state.channels, res_table[m_resolution],
                    mp4state.decode_pos_ms);
                module.VSAAddPCMData(sample_buffer, (int)mp4state.channels, res_table[m_resolution],
                    mp4state.decode_pos_ms);
                mp4state.decode_pos_ms += (double)frameInfo.samples * 1000.0 /
                    ((double)frameInfo.samplerate* (double)frameInfo.channels);

                l = frameInfo.samples * res_table[m_resolution] / 8;

                if (module.dsp_isactive())
                {
                    void *dsp_buffer = malloc(l*2);
                    memcpy(dsp_buffer, sample_buffer, l);

                    l = module.dsp_dosamples((short*)dsp_buffer,
                        frameInfo.samples/frameInfo.channels,
                        res_table[m_resolution],
                        frameInfo.channels,
                        frameInfo.samplerate) *
                        (frameInfo.channels*(res_table[m_resolution]/8));

                    module.outMod->Write(dsp_buffer, l);
                    if (dsp_buffer) free(dsp_buffer);
                } else {
                    module.outMod->Write(sample_buffer, l);
                }

                /* VBR bitrate display */
                if (m_vbr_display)
                {
                    seq_frames++;
                    seq_bytes += frameInfo.bytesconsumed;
                    if (seq_frames == (int)(floor((float)frameInfo.samplerate/(float)(frameInfo.samples/frameInfo.channels) + 0.5)))
                    {
                        module.SetInfo((int)floor(((float)seq_bytes*8.)/1000. + 0.5),
                            (int)floor(frameInfo.samplerate/1000. + 0.5),
                            mp4state.channels, 1);

                        seq_frames = 0;
                        seq_bytes = 0;
                    }
                }
            }

            if (frameInfo.channels > 0 && mp4state.samplerate > 0)
                mp4state.cur_pos_sec += ((double)(frameInfo.samples/frameInfo.channels))/(double)mp4state.samplerate;
        } else {
            Sleep(10);
        }
    }

    PlayThreadAlive = 0;

    return 0;
}

static In_Module module =
{
    IN_VER,
    "AudioCoding.com MPEG-4 General Audio player: " FAAD2_VERSION " compiled on " __DATE__,
    0,  // hMainWindow
    0,  // hDllInstance
    NULL,
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
    config_read();

    if (!m_use_for_aac)
    {
        module.FileExtensions = short_ext_list;
    } else {
        module.FileExtensions = long_ext_list;
    }

    return &module;
}
