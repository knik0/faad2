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
*/

//    WAVE_OUT.H - Necessary stuff for WIN_AUDIO

#include <stdio.h>
#include <windows.h>

#define Cdecl               __cdecl
#define __attribute__(x)
#define sleep(__sec)        Sleep ((__sec) * 1000)
#define inline              __inline
#define restrict

//// constants /////////////////////////////////////////////////////

#define CD_SAMPLE_FREQ         44.1e3
#define SAMPLE_SIZE            16
#define SAMPLE_SIZE_STRING     ""
#define WINAUDIO_FD            ((FILE_T)-128)
#define FILE_T                 FILE*
#define INVALID_FILEDESC       NULL

//// procedures/functions //////////////////////////////////////////
// wave_out.c
int        Set_WIN_Params             ( FILE_T dummyFile , long double SampleFreq, unsigned int BitsPerSample, unsigned int Channels, unsigned int play_priority );
int        WIN_Play_Samples           ( const void* buff, size_t len );
int        WIN_Audio_close            ( void );

