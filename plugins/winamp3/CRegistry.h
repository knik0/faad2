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

#ifndef registry_h
#define registry_h

class CRegistry 
{
public:
			CRegistry();
			~CRegistry();

	void	ShowLastError(char *Caption);

	BOOL	Open(HKEY hKey, char *SubKey);
	BOOL	OpenCreate(HKEY hKey, char *SubKey);
	BOOL	Close();
	BOOL	DeleteVal(char *SubKey);
	BOOL	DeleteKey(char *SubKey);
	BOOL	RecurseDeleteKey(char *SubKey);
	BOOL	EnumKey(DWORD Index, TCHAR *buf, DWORD size);

	void	SetBool(char *keyStr , BOOL val);
	void	SetBool(char *keyStr , bool val);
	void	SetByte(char *keyStr , BYTE val);
	void	SetWord(char *keyStr , WORD val);
	void	SetDword(char *keyStr , DWORD val);
	void	SetFloat(char *keyStr , float val);
	void	SetStr(char *keyStr , char *valStr);
	void	SetValN(char *keyStr , BYTE *addr,  DWORD size);

	BOOL	GetSetBool(char *keyStr, BOOL var);
	bool	GetSetBool(char *keyStr, bool var);
	BYTE	GetSetByte(char *keyStr, BYTE var);
	WORD	GetSetWord(char *keyStr, WORD var);
	DWORD	GetSetDword(char *keyStr, DWORD var);
	float	GetSetFloat(char *keyStr, float var);
	int		GetSetStr(char *keyStr, char *tempString, char *dest, int maxLen);
	int		GetSetValN(char *keyStr, BYTE *tempAddr, BYTE *addr, DWORD size);

	const HKEY	GetKey();
	const char	*GetPath();

private:
	HKEY	Key;
	char	*Path;
};
#endif