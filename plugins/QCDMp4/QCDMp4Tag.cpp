#include <mp4.h>
#include <faad.h>
#include "QCDTagsDLL.h"


//..............................................................................
// Global Variables

QCDModInitTag	ModInitTag;
BOOL uSetDlgItemText(void *tagHandle, int fieldId, const char *str);
UINT uGetDlgItemText(void *tagHandle, int fieldId, char *str, int max);

//------------------------------------------------------------------------------

PLUGIN_API QCDModInitTag* TAGEDITORDLL_ENTRY_POINT()
{	
	ModInitTag.size			= sizeof(QCDModInitTag);
	ModInitTag.version		= PLUGIN_API_VERSION;
	ModInitTag.ShutDown		= ShutDown_Tag;

	ModInitTag.Read			= Read_Tag;
	ModInitTag.Write		= Write_Tag;	// Leave null for operations that plugin does not support
	ModInitTag.Strip		= Strip_Tag;	// ie: if plugin only reads tags, leave Write and Strip null

	ModInitTag.description	= "MP4 Tags";
	ModInitTag.defaultexts	= "MP4:M4A";

	return &ModInitTag;
}

//-----------------------------------------------------------------------------

void ShutDown_Tag(int flags)
{
	// TODO:
	// prepare plugin to be unloaded. All allocations should be freed.
	// flags param is unused
}

//-----------------------------------------------------------------------------

bool Read_Tag(LPCSTR filename, void* tagHandle)
{
	// TODO:
	// read metadata from tag and set each field to tagHandle
	// only TAGFIELD_* are supported (see QCDModTagEditor.h)

	// example of how to set value to tagHandle
	// use SetFieldA for ASCII or MultiBytes strings.
	// use SetFieldW for UNICODE strings
	//
	//	ModInitTag.SetFieldW(tagHandle, TAGFIELD_COMPOSER, szwValue);

	// return true for successfull read, false for failure

	MP4FileHandle file = MP4_INVALID_FILE_HANDLE;
	char *pVal, dummy1[1024];
	short dummy, dummy2;

	unsigned __int32 valueSize = 0;

#ifdef DEBUG_OUTPUT
	in_mp4_DebugOutput("mp4_tag_read");
#endif

	file = MP4Read(filename, 0);

	if (file == MP4_INVALID_FILE_HANDLE)
		return false;

	/* get Metadata */

	pVal = NULL;
	MP4GetMetadataName(file, &pVal);
	uSetDlgItemText(tagHandle, TAGFIELD_TITLE, pVal);

	pVal = NULL;
	MP4GetMetadataArtist(file, &pVal);
	uSetDlgItemText(tagHandle, TAGFIELD_ARTIST, pVal);

	pVal = NULL;
	MP4GetMetadataWriter(file, &pVal);
	uSetDlgItemText(tagHandle, TAGFIELD_COMPOSER, pVal);

	pVal = NULL;
	MP4GetMetadataComment(file, &pVal);
	uSetDlgItemText(tagHandle, TAGFIELD_COMMENT, pVal);

	pVal = NULL;
	MP4GetMetadataAlbum(file, &pVal);
	uSetDlgItemText(tagHandle, TAGFIELD_ALBUM, pVal);

	pVal = NULL;
	MP4GetMetadataGenre(file, &pVal);
	uSetDlgItemText(tagHandle, TAGFIELD_GENRE, pVal);

	//dummy = 0;
	//MP4GetMetadataTempo(file, &dummy);
	//wsprintf(dummy1, "%d", dummy);
	//SetDlgItemText(hwndDlg,IDC_METATEMPO, dummy1);

	dummy = 0; dummy2 = 0;
	MP4GetMetadataTrack(file, (unsigned __int16*)&dummy, (unsigned __int16*)&dummy2);
	wsprintf(dummy1, "%d", dummy);
	ModInitTag.SetFieldA(tagHandle, TAGFIELD_TRACK, dummy1);
	//wsprintf(dummy1, "%d", dummy2);
	//SetDlgItemText(hwndDlg,IDC_METATRACK2, dummy1);

	//dummy = 0; dummy2 = 0;
	//MP4GetMetadataDisk(file, &dummy, &dummy2);
	//wsprintf(dummy1, "%d", dummy);
	//SetDlgItemText(hwndDlg,IDC_METADISK1, dummy1);
	//wsprintf(dummy1, "%d", dummy2);
	//SetDlgItemText(hwndDlg,IDC_METADISK2, dummy1);

	pVal = NULL;
	MP4GetMetadataYear(file, &pVal);
	uSetDlgItemText(tagHandle, TAGFIELD_YEAR, pVal);

	//dummy3 = 0;
	//MP4GetMetadataCompilation(file, &dummy3);
	//if (dummy3)
	//	SendMessage(GetDlgItem(hwndDlg, IDC_METACOMPILATION), BM_SETCHECK, BST_CHECKED, 0);

	pVal = NULL;
	MP4GetMetadataTool(file, &pVal);
	uSetDlgItemText(tagHandle, TAGFIELD_ENCODER, pVal);

	pVal = NULL;
	MP4GetMetadataFreeForm(file, "CONDUCTOR", (unsigned __int8**)&pVal, &valueSize);
	uSetDlgItemText(tagHandle, TAGFIELD_CONDUCTOR, pVal);

	pVal = NULL;
	MP4GetMetadataFreeForm(file, "ORCHESTRA", (unsigned __int8**)&pVal, &valueSize);
	uSetDlgItemText(tagHandle, TAGFIELD_ORCHESTRA, pVal);

	pVal = NULL;
	MP4GetMetadataFreeForm(file, "YEARCOMPOSED", (unsigned __int8**)&pVal, &valueSize);
	uSetDlgItemText(tagHandle, TAGFIELD_YEARCOMPOSED, pVal);

	pVal = NULL;
	MP4GetMetadataFreeForm(file, "ORIGARTIST", (unsigned __int8**)&pVal, &valueSize);
	uSetDlgItemText(tagHandle, TAGFIELD_ORIGARTIST, pVal);

	pVal = NULL;
	MP4GetMetadataFreeForm(file, "LABEL", (unsigned __int8**)&pVal, &valueSize);
	uSetDlgItemText(tagHandle, TAGFIELD_LABEL, pVal);

	pVal = NULL;
	MP4GetMetadataFreeForm(file, "COPYRIGHT", (unsigned __int8**)&pVal, &valueSize);
	uSetDlgItemText(tagHandle, TAGFIELD_COPYRIGHT, pVal);

	pVal = NULL;
	MP4GetMetadataFreeForm(file, "CDDBTAGID", (unsigned __int8**)&pVal, &valueSize);
	uSetDlgItemText(tagHandle, TAGFIELD_CDDBTAGID, pVal);

	/* ! Metadata */

	MP4Close(file);

	return true;
}

//-----------------------------------------------------------------------------

bool Write_Tag(LPCSTR filename, void* tagHandle)
{
	// TODO:
	// read metadata from tagHandle and set each field to supported tag
	// only TAGFIELD_* are supported (see QCDModTagEditor.h)

	// example of how to get value from tagHandle
	// use SetFieldA for ASCII or MultiBytes strings.
	// use SetFieldW for UNICODE strings
	//
	// szwValue = ModInitTag.GetFieldW(tagHandle, TAGFIELD_ORCHESTRA);

	// write tag to file

	MP4FileHandle file = MP4_INVALID_FILE_HANDLE;
    char dummy1[1024];
    short dummy, dummy2;

#ifdef DEBUG_OUTPUT
    in_mp4_DebugOutput("mp4_tag_write");
#endif

	/* save Metadata changes */

	file = MP4Modify(filename, 0, 0);
	if (file == MP4_INVALID_FILE_HANDLE)
		return false;

	uGetDlgItemText(tagHandle, TAGFIELD_TITLE, dummy1, 1024);
	MP4SetMetadataName(file, dummy1);

	uGetDlgItemText(tagHandle, TAGFIELD_COMPOSER, dummy1, 1024);
	MP4SetMetadataWriter(file, dummy1);

	uGetDlgItemText(tagHandle, TAGFIELD_ARTIST, dummy1, 1024);
	MP4SetMetadataArtist(file, dummy1);

	uGetDlgItemText(tagHandle, TAGFIELD_ALBUM, dummy1, 1024);
	MP4SetMetadataAlbum(file, dummy1);

	uGetDlgItemText(tagHandle, TAGFIELD_COMMENT, dummy1, 1024);
	MP4SetMetadataComment(file, dummy1);

	uGetDlgItemText(tagHandle, TAGFIELD_GENRE, dummy1, 1024);
	MP4SetMetadataGenre(file, dummy1);

	uGetDlgItemText(tagHandle, TAGFIELD_YEAR, dummy1, 1024);
	MP4SetMetadataYear(file, dummy1);

	dummy = 0; dummy2 = 0;
	MP4GetMetadataTrack(file, (unsigned __int16*)&dummy, (unsigned __int16*)&dummy2);
	memcpy(dummy1, ModInitTag.GetFieldA(tagHandle, TAGFIELD_TRACK), sizeof(dummy1));
	//GetDlgItemText(hwndDlg, IDC_METATRACK1, dummy1, 1024);
	dummy = atoi(dummy1);
	//GetDlgItemText(hwndDlg, IDC_METATRACK2, dummy1, 1024);
	//dummy2 = atoi(dummy1);
	MP4SetMetadataTrack(file, dummy, dummy2);

	//GetDlgItemText(hwndDlg, IDC_METADISK1, dummy1, 1024);
	//dummy = atoi(dummy1);
	//GetDlgItemText(hwndDlg, IDC_METADISK2, dummy1, 1024);
	//dummy2 = atoi(dummy1);
	//MP4SetMetadataDisk(file, dummy, dummy2);

	//GetDlgItemText(hwndDlg, IDC_METATEMPO, dummy1, 1024);
	//dummy = atoi(dummy1);
	//MP4SetMetadataTempo(file, dummy);

	//dummy3 = SendMessage(GetDlgItem(hwndDlg, IDC_METACOMPILATION), BM_GETCHECK, 0, 0);
	//MP4SetMetadataCompilation(file, dummy3);

	uGetDlgItemText(tagHandle, TAGFIELD_ENCODER, dummy1, 1024);
	MP4SetMetadataTool(file, dummy1);

	uGetDlgItemText(tagHandle, TAGFIELD_CONDUCTOR, dummy1, 1024);
	MP4SetMetadataFreeForm(file, "CONDUCTOR", (unsigned __int8*)dummy1, strlen(dummy1) + 1);

	uGetDlgItemText(tagHandle, TAGFIELD_ORCHESTRA, dummy1, 1024);
	MP4SetMetadataFreeForm(file, "ORCHESTRA", (unsigned __int8*)dummy1, strlen(dummy1) + 1);

	uGetDlgItemText(tagHandle, TAGFIELD_YEARCOMPOSED, dummy1, 1024);
	MP4SetMetadataFreeForm(file, "YEARCOMPOSED", (unsigned __int8*)dummy1, strlen(dummy1) + 1);

	uGetDlgItemText(tagHandle, TAGFIELD_ORIGARTIST, dummy1, 1024);
	MP4SetMetadataFreeForm(file, "ORIGARTIST", (unsigned __int8*)dummy1, strlen(dummy1) + 1);

	uGetDlgItemText(tagHandle, TAGFIELD_LABEL, dummy1, 1024);
	MP4SetMetadataFreeForm(file, "LABEL", (unsigned __int8*)dummy1, strlen(dummy1) + 1);

	uGetDlgItemText(tagHandle, TAGFIELD_COPYRIGHT, dummy1, 1024);
	MP4SetMetadataFreeForm(file, "COPYRIGHT", (unsigned __int8*)dummy1, strlen(dummy1) + 1);

	uGetDlgItemText(tagHandle, TAGFIELD_CDDBTAGID, dummy1, 1024);
	MP4SetMetadataFreeForm(file, "CDDBTAGID", (unsigned __int8*)dummy1, strlen(dummy1) + 1);

	MP4Close(file);

	MP4Optimize(filename, NULL, 0);
	/* ! */

	return true;
}

//-----------------------------------------------------------------------------

bool Strip_Tag(LPCSTR filename)
{
	// TODO:
	// remove tag from file.
	// do whatever is need to remove the supported tag from filename

	// return true for successfull strip, false for failure

	MP4FileHandle file;

	file = MP4Modify(filename, 0, 0);
	if (file == MP4_INVALID_FILE_HANDLE)
		return false;
	
	MP4MetadataDelete(file);

	MP4Close(file);

	return true;
}

//-----------------------------------------------------------------------------

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

BOOL uSetDlgItemText(void *tagHandle, int fieldId, const char *str)
{
    char *temp;
    size_t len;
    int r;

    if (!str) return FALSE;
    len = strlen(str);
    temp = (char *)malloc(len+1);
    if (!temp) return FALSE;
	memset(temp, '\0', len+1);
    r = ConvertUTF8ToANSI(str, temp);
    if (r > 0)
        ModInitTag.SetFieldA(tagHandle, fieldId, temp);
    free(temp);

    return r>0 ? TRUE : FALSE;
}

UINT uGetDlgItemText(void *tagHandle, int fieldId, char *str, int max)
{
    char *temp, *utf8;;
    int len;

	const char *p;

    if (!str) return 0;
    len = strlen( ModInitTag.GetFieldA(tagHandle, fieldId) );
    temp = (char *)malloc(len+1);
    if (!temp) return 0;
    utf8 = (char *)malloc((len+1)*4);
    if (!utf8)
    {
        free(temp);
        return 0;
    }

	memset(temp, '\0', len+1);
	memset(utf8, '\0', (len+1)*4);
	p = ModInitTag.GetFieldA(tagHandle, fieldId);
	memcpy(temp, p, len+1);
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
