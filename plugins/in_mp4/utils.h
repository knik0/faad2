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
** $Id: utils.h,v 1.1 2002/08/09 21:48:12 menno Exp $
**/

LPTSTR PathFindFileName(LPCTSTR pPath);
int GetVideoTrack(MP4FileHandle infile);
int GetAudioTrack(MP4FileHandle infile);
int GetAACTrack(MP4FileHandle infile);
int StringComp(char const *str1, char const *str2, unsigned long len);
