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
** $Id: config.c,v 1.1 2003/04/28 19:07:57 menno Exp $
**/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "config.h"

char app_name[] = "QCDMp4";
char INI_FILE[MAX_PATH];
int m_priority = 3;
int m_resolution = 0;
int m_show_errors = 1;
int m_use_for_aac = 1;

void _r_s(char *name,char *data, int mlen)
{
	char buf[10];
	strcpy(buf,data);
	GetPrivateProfileString(app_name,name,buf,data,mlen,INI_FILE);
}

