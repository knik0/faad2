/*
CRegistry class
Copyright (C) 2002 Antonio Foranna

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation.
	
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
		
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
			
The author can be contacted at:
kreel@tiscali.it
*/

//#include "stdafx.h"
#include <windows.h>
#include <string.h>
#include <memory.h>
#include "CRegistry.h"



// *****************************************************************************



CRegistry::CRegistry()
{
	Key=NULL;
	Path=NULL;
}
// -----------------------------------------------------------------------------------------------

CRegistry::~CRegistry()
{
	Close();
}
// *****************************************************************************

inline const HKEY CRegistry::GetKey()
{
	return Key;
}
// -----------------------------------------------------------------------------------------------

inline const char *CRegistry::GetPath()
{
	return Path;
}
// *****************************************************************************

void CRegistry::ShowLastError(char *Caption)
{
LPVOID MsgBuf;
	if(FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &MsgBuf,
		0,
		NULL 
	))
		MessageBox(NULL, (LPCTSTR)MsgBuf, Caption, MB_OK|MB_ICONSTOP);
	if(MsgBuf)
		LocalFree(MsgBuf);
}



// *****************************************************************************



#define SetPath(SubKeyName) \
	if(Path) \
		free(Path); \
	Path=strdup(SubKeyName);

// -----------------------------------------------------------------------------------------------

BOOL CRegistry::Open(HKEY hKey, char *SubKeyName)
{
	if(Key)
		Close();
	if(RegOpenKeyEx(hKey, SubKeyName, NULL , KEY_ALL_ACCESS , &Key)==ERROR_SUCCESS)
	{
		SetPath(SubKeyName);
		return TRUE;
	}
	else // can't open the key -> error
	{
		Key=0;
		SetPath("");
		return FALSE;
	}
}
// -----------------------------------------------------------------------------------------------

BOOL CRegistry::OpenCreate(HKEY hKey, char *SubKeyName)
{
	if(Key)
		Close();
	if(RegOpenKeyEx(hKey, SubKeyName, NULL , KEY_ALL_ACCESS , &Key)==ERROR_SUCCESS)
	{
		SetPath(SubKeyName);
		return TRUE;
	}
	else // open failed -> create the key
	{
	DWORD disp;
		RegCreateKeyEx(hKey , SubKeyName, NULL , NULL, REG_OPTION_NON_VOLATILE , KEY_ALL_ACCESS , NULL , &Key , &disp );
		if(disp==REG_CREATED_NEW_KEY) 
		{
			SetPath(SubKeyName);
			return TRUE;
		}
		else // can't create the key -> error
		{
			Key=0;
			SetPath("");
			return FALSE;
		}
	}
}
// -----------------------------------------------------------------------------------------------

BOOL CRegistry::Close()
{
BOOL retVal=TRUE;
	if(Key)
		retVal=RegCloseKey(Key)==ERROR_SUCCESS;
	Key=NULL;
	if(Path) 
		delete Path; 
	Path=NULL;
	return retVal;
}



// *****************************************************************************



inline BOOL CRegistry::DeleteVal(char *SubKeyName)
{
	return RegDeleteValue(Key,SubKeyName)==ERROR_SUCCESS;
}
// -----------------------------------------------------------------------------------------------

inline BOOL CRegistry::DeleteKey(char *SubKeyName)
{
	return RegDeleteKey(Key,SubKeyName)==ERROR_SUCCESS;
}
// -----------------------------------------------------------------------------------------------

BOOL CRegistry::RecurseDeleteKey(char *SubKeyName)
{
CRegistry	SubKey;
FILETIME	time;
TCHAR		buf[256];
DWORD		size=sizeof(buf)*sizeof(TCHAR),
			len=size;
DWORD		retVal;

	if(SubKey.Open(Key,SubKeyName)!=ERROR_SUCCESS)
		return FALSE;
	while((retVal=RegEnumKeyEx(SubKey.Key, 0, buf, &len, NULL, NULL, NULL, &time))==ERROR_SUCCESS)
	{
		if(!SubKey.RecurseDeleteKey(buf))
		{
			SubKey.Close();
			return FALSE;
		}
		len=size;
	}
	SubKey.Close();
	if(retVal!=ERROR_NO_MORE_ITEMS)
		return FALSE;
	return DeleteKey(SubKeyName);
}
// -----------------------------------------------------------------------------------------------

inline BOOL CRegistry::EnumKey(DWORD Index, TCHAR *buf, DWORD size)
{
FILETIME	time;

	return RegEnumKeyEx(Key, Index, buf, &size, NULL, NULL, NULL, &time)==ERROR_SUCCESS;
}



// *****************************************************************************



void CRegistry::SetBool(char *keyStr , BOOL val)
{
BOOL tempVal;
DWORD len;
	if(RegQueryValueEx(Key , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS ||
		tempVal!=val)
		RegSetValueEx(Key , keyStr , NULL , REG_BINARY , (BYTE *)&val , sizeof(BOOL));
}
// -----------------------------------------------------------------------------------------------

void CRegistry::SetBool(char *keyStr , bool val)
{
bool tempVal;
DWORD len;
	if(RegQueryValueEx(Key , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS ||
		tempVal!=val)
		RegSetValueEx(Key , keyStr , NULL , REG_BINARY , (BYTE *)&val , sizeof(bool));
}
// -----------------------------------------------------------------------------------------------

void CRegistry::SetByte(char *keyStr , BYTE val)
{
DWORD	t=val;
DWORD	tempVal;
DWORD	len;
	if(RegQueryValueEx(Key , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS ||
		tempVal!=val)
		RegSetValueEx(Key , keyStr , NULL , REG_DWORD , (BYTE *)&t , sizeof(DWORD));
}
// -----------------------------------------------------------------------------------------------

void CRegistry::SetWord(char *keyStr , WORD val)
{
DWORD	t=val;
DWORD	tempVal;
DWORD	len;
	if(RegQueryValueEx(Key , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS ||
		tempVal!=val)
		RegSetValueEx(Key , keyStr , NULL , REG_DWORD , (BYTE *)&t , sizeof(DWORD));
}
// -----------------------------------------------------------------------------------------------

void CRegistry::SetDword(char *keyStr , DWORD val)
{
DWORD tempVal;
DWORD len;
	if(RegQueryValueEx(Key , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS ||
		tempVal!=val)
		RegSetValueEx(Key , keyStr , NULL , REG_DWORD , (BYTE *)&val , sizeof(DWORD));
}
// -----------------------------------------------------------------------------------------------

void CRegistry::SetFloat(char *keyStr , float val)
{
float tempVal;
DWORD len;
	if(RegQueryValueEx(Key , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS ||
		tempVal!=val)
		RegSetValueEx(Key , keyStr , NULL , REG_BINARY , (BYTE *)&val , sizeof(float));
}
// -----------------------------------------------------------------------------------------------

void CRegistry::SetStr(char *keyStr , char *valStr)
{
DWORD len;
DWORD slen=strlen(valStr)+1;

	if(!valStr || !*valStr)
		return;

	if(RegQueryValueEx(Key , keyStr , NULL , NULL, NULL , &len )!=ERROR_SUCCESS ||
		len!=slen)
		RegSetValueEx(Key , keyStr , NULL , REG_SZ , (BYTE *)valStr , slen);
	else
	{
	char *tempVal=new char[slen];
		if(RegQueryValueEx(Key , keyStr , NULL , NULL, (BYTE *)tempVal , &len )!=ERROR_SUCCESS ||
			strcmpi(tempVal,valStr))
			RegSetValueEx(Key , keyStr , NULL , REG_SZ , (BYTE *)valStr , slen);
		delete tempVal;
	}
}
// -----------------------------------------------------------------------------------------------

void CRegistry::SetValN(char *keyStr , BYTE *addr,  DWORD size)
{
DWORD len;
	if(!addr || !size)
		return;

	if(RegQueryValueEx(Key , keyStr , NULL , NULL, NULL , &len )!=ERROR_SUCCESS ||
		len!=size)
		RegSetValueEx(Key , keyStr , NULL , REG_BINARY , addr , size);
	else
	{
	BYTE *tempVal=new BYTE[size];
		if(RegQueryValueEx(Key , keyStr , NULL , NULL, (BYTE *)tempVal , &len )!=ERROR_SUCCESS ||
			memcmp(tempVal,addr,len))
			RegSetValueEx(Key , keyStr , NULL , REG_BINARY , addr , size);
		delete tempVal;
	}
}



// *****************************************************************************



BOOL CRegistry::GetSetBool(char *keyStr, BOOL val)
{
BOOL tempVal;
DWORD len=sizeof(BOOL);

	if(RegQueryValueEx(Key , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS)
	{
		RegSetValueEx(Key , keyStr , NULL , REG_BINARY , (BYTE *)&val , sizeof(BOOL));
		return val;
	}
	return tempVal;
}
// -----------------------------------------------------------------------------------------------

bool CRegistry::GetSetBool(char *keyStr, bool val)
{
bool tempVal;
DWORD len=sizeof(bool);

	if(RegQueryValueEx(Key , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS)
	{
		RegSetValueEx(Key , keyStr , NULL , REG_BINARY , (BYTE *)&val , sizeof(bool));
		return val;
	}
	return tempVal;
}
// -----------------------------------------------------------------------------------------------

BYTE CRegistry::GetSetByte(char *keyStr, BYTE val)
{
DWORD tempVal;
DWORD len=sizeof(DWORD);

	if(RegQueryValueEx(Key , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS)
	{
		tempVal=val;
		RegSetValueEx(Key , keyStr , NULL , REG_DWORD , (BYTE *)&tempVal , sizeof(DWORD));
		return val;
	}
	return (BYTE)tempVal;
}
// -----------------------------------------------------------------------------------------------

WORD CRegistry::GetSetWord(char *keyStr, WORD val)
{
DWORD tempVal;
DWORD len=sizeof(DWORD);

	if(RegQueryValueEx(Key , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS)
	{
		tempVal=val;
		RegSetValueEx(Key , keyStr , NULL , REG_DWORD , (BYTE *)&tempVal , sizeof(DWORD));
		return val;
	}
	return (WORD)tempVal;
}
// -----------------------------------------------------------------------------------------------

DWORD CRegistry::GetSetDword(char *keyStr, DWORD val)
{
DWORD tempVal;
DWORD len=sizeof(DWORD);

	if(RegQueryValueEx(Key , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS)
	{
		RegSetValueEx(Key , keyStr , NULL , REG_DWORD , (BYTE *)&val , sizeof(DWORD));
		return val;
	}
	return (DWORD)tempVal;
}
// -----------------------------------------------------------------------------------------------

float CRegistry::GetSetFloat(char *keyStr, float val)
{
float tempVal;
DWORD len=sizeof(float);

	if(RegQueryValueEx(Key , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS)
	{
		RegSetValueEx(Key , keyStr , NULL , REG_BINARY , (BYTE *)&val , sizeof(float));
		return val;
	}
	return tempVal;
}
// -----------------------------------------------------------------------------------------------

int CRegistry::GetSetStr(char *keyStr, char *tempString, char *dest, int maxLen)
{
DWORD tempLen=maxLen;
	
	if(RegQueryValueEx(Key , keyStr , NULL , NULL, (BYTE *) dest , &tempLen )!=ERROR_SUCCESS)
	{
		if(!tempString)
		{
			*dest=0;
			return 0;
		}
		strcpy(dest,tempString);
		tempLen=strlen(tempString)+1;
		RegSetValueEx(Key , keyStr , NULL , REG_SZ , (BYTE *)tempString , tempLen);
	}
	return tempLen;
}
// -----------------------------------------------------------------------------------------------

int CRegistry::GetSetValN(char *keyStr, BYTE *tempAddr, BYTE *addr, DWORD size)
{
DWORD tempLen=size;

	if(RegQueryValueEx(Key , keyStr , NULL , NULL, (BYTE *)addr , &tempLen )!=ERROR_SUCCESS)
	{
		if(!tempAddr)
		{
			*addr=0;
			return 0;
		}
		memcpy(addr,tempAddr,size);
		RegSetValueEx(Key , keyStr , NULL , REG_BINARY , (BYTE *)addr , size);
	}
	return tempLen;
}
