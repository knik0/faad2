/*
 * FAAD - Freeware Advanced Audio Decoder
 * Copyright (C) 2001 Menno Bakker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: id3v2tag.h,v 1.1 2002/01/14 19:15:49 menno Exp $
 */

#ifndef __ID3V2TAG_H__
#define __ID3V2TAG_H__

void GetID3FileTitle(char *filename, char *title, char *format);
void FillID3List(HWND hwndDlg, HWND hwndList, char *filename);
void List_OnGetDispInfo(LV_DISPINFO *pnmv);
BOOL List_EditData(HWND hwndApp, HWND hwndList);
void List_SaveID3(HWND hwndApp, HWND hwndList, char *filename);
BOOL List_DeleteSelected(HWND hwndApp, HWND hwndList);
BOOL List_AddFrame(HWND hwndApp, HWND hwndList);
BOOL List_AddStandardFrames(HWND hwndApp, HWND hwndList);
void AddFrameFromRAWData(HWND hwndList, int frameId, LPSTR data1, LPSTR data2);

HINSTANCE hInstance_for_id3editor;

typedef struct ID3GENRES_TAG
{
    BYTE id;
    char name[30];
} ID3GENRES;

typedef struct id3item_tag {
    int frameId;
    LPSTR aCols[2];
} ID3ITEM;

#endif