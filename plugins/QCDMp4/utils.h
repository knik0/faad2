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
** $Id: utils.h,v 1.1 2003/04/28 19:07:57 menno Exp $
**/

#ifndef UTILS_INCLUDED
#define UTILS_INCLUDED

#include <mp4.h>

LPTSTR PathFindFileName(LPCTSTR pPath);
int GetVideoTrack(MP4FileHandle infile);
int GetAudioTrack(MP4FileHandle infile);
int GetAACTrack(MP4FileHandle infile);
int StringComp(char const *str1, char const *str2, unsigned long len);
char *convert3in4to3in3(void *sample_buffer, int samples);

#endif