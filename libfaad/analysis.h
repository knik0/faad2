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
** $Id: analysis.h,v 1.1 2002/01/22 09:11:24 menno Exp $
**/

#ifndef __ANALYSIS_H__
#define __ANALYSIS_H__

#ifdef __cplusplus
extern "C" {
#endif


#ifdef ANALYSIS
#define DEBUGDEC        ,int print,int var,char *dbg
#define DEBUGVAR(A,B,C) ,A,B,C
extern int dbg_count;
#else
#define DEBUGDEC
#define DEBUGVAR(A,B,C)
#endif


#ifdef __cplusplus
}
#endif
#endif
