// $Id: misc_support.cpp,v 1.1 2002/01/21 08:16:22 menno Exp $

// id3lib: a C++ library for creating and manipulating id3v1/v2 tags
// Copyright 1999, 2000  Scott Thomas Haug

// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Library General Public License as published by
// the Free Software Foundation; either version 2 of the License, or (at your
// option) any later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
// License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// The id3lib authors encourage improvements and optimisations to be sent to
// the id3lib coordinator.  Please see the README file for details on where to
// send such submissions.  See the AUTHORS file for a list of people who have
// contributed to id3lib.  See the ChangeLog file for a list of changes to
// id3lib.  These files are distributed with id3lib at
// http://download.sourceforge.net/id3lib/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "misc_support.h"

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

char *ID3_GetString(const ID3_Frame *frame, ID3_FieldID fldName, size_t nIndex)
{
  char *sText = NULL;
  if (NULL != frame)
  {
    size_t nText = frame->Field(fldName).Size();
    sText = new char[nText + 1];
    try 
    {
      frame->Field(fldName).Get(sText, nText, nIndex);
    }
    catch (ID3_Err&)
    {
      delete [] sText;
      return NULL;
    }
    sText[nText] = '\0';
  }
  return sText;
}

char *ID3_GetArtist(const ID3_Tag *tag)
{
  char *sArtist = NULL;
  if (NULL == tag)
  {
    return sArtist;
  }

  ID3_Frame *pFrame = NULL;
  if ((pFrame = tag->Find(ID3FID_LEADARTIST)) ||
      (pFrame = tag->Find(ID3FID_BAND))       ||
      (pFrame = tag->Find(ID3FID_CONDUCTOR))  ||
      (pFrame = tag->Find(ID3FID_COMPOSER)))
  {
    sArtist = ID3_GetString(pFrame, ID3FN_TEXT);
  }
  return sArtist;
}

ID3_Frame* ID3_AddArtist(ID3_Tag *tag, const char *text, bool bReplace)
{
  ID3_Frame* pFrame = NULL;
  if (NULL != tag && NULL != text && strlen(text) > 0)
  {
    if (bReplace)
    {
      ID3_RemoveArtists(tag);
    }
    if (bReplace ||
        (tag->Find(ID3FID_LEADARTIST) == NULL &&
         tag->Find(ID3FID_BAND)       == NULL &&
         tag->Find(ID3FID_CONDUCTOR)  == NULL &&
         tag->Find(ID3FID_COMPOSER)   == NULL))
    {
      pFrame = new ID3_Frame(ID3FID_LEADARTIST);
      if (pFrame)
      {
        pFrame->Field(ID3FN_TEXT) = text;
        tag->AttachFrame(pFrame);
      }
    }
  }

  return pFrame;
}

size_t ID3_RemoveArtists(ID3_Tag *tag)
{
  size_t nRemoved = 0;
  ID3_Frame *pFrame = NULL;

  if (NULL == tag)
  {
    return nRemoved;
  }

  while ((pFrame = tag->Find(ID3FID_LEADARTIST)))
  {
    tag->RemoveFrame(pFrame);
    nRemoved++;
  }
  while ((pFrame = tag->Find(ID3FID_BAND)))
  {
    tag->RemoveFrame(pFrame);
    nRemoved++;
  }
  while ((pFrame = tag->Find(ID3FID_CONDUCTOR)))
  {
    tag->RemoveFrame(pFrame);
    nRemoved++;
  }
  while ((pFrame = tag->Find(ID3FID_COMPOSER)))
  {
    tag->RemoveFrame(pFrame);
    nRemoved++;
  }

  return nRemoved;
}

char *ID3_GetAlbum(const ID3_Tag *tag)
{
  char *sAlbum = NULL;
  if (NULL == tag)
  {
    return sAlbum;
  }

  ID3_Frame *pFrame = tag->Find(ID3FID_ALBUM);
  if (pFrame != NULL)
  {
    sAlbum = ID3_GetString(pFrame, ID3FN_TEXT);
  }
  return sAlbum;
}

ID3_Frame* ID3_AddAlbum(ID3_Tag *tag, const char *text, bool bReplace)
{
  ID3_Frame* pFrame = NULL;
  if (NULL != tag && NULL != text && strlen(text) > 0)
  {
    if (bReplace)
    {
      ID3_RemoveAlbums(tag);
    }
    if (bReplace || tag->Find(ID3FID_ALBUM) == NULL)
    {
      pFrame = new ID3_Frame(ID3FID_ALBUM);
      if (pFrame)
      {
        pFrame->Field(ID3FN_TEXT) = text;
        tag->AttachFrame(pFrame);
      }
    }
  }
  
  return pFrame;
}

size_t ID3_RemoveAlbums(ID3_Tag *tag)
{
  size_t nRemoved = 0;
  ID3_Frame *pFrame = NULL;

  if (NULL == tag)
  {
    return nRemoved;
  }

  while ((pFrame = tag->Find(ID3FID_ALBUM)))
  {
    tag->RemoveFrame(pFrame);
    nRemoved++;
  }

  return nRemoved;
}

char *ID3_GetTitle(const ID3_Tag *tag)
{
  char *sTitle = NULL;
  if (NULL == tag)
  {
    return sTitle;
  }

  ID3_Frame *pFrame = tag->Find(ID3FID_TITLE);
  if (pFrame != NULL)
  {
    sTitle = ID3_GetString(pFrame, ID3FN_TEXT);
  }
  return sTitle;
}

ID3_Frame* ID3_AddTitle(ID3_Tag *tag, const char *text, bool bReplace)
{
  ID3_Frame* pFrame = NULL;
  if (NULL != tag && NULL != text && strlen(text) > 0)
  {
    if (bReplace)
    {
      ID3_RemoveTitles(tag);
    }
    if (bReplace || tag->Find(ID3FID_TITLE) == NULL)
    {
      pFrame = new ID3_Frame(ID3FID_TITLE);
      if (pFrame)
      {
        pFrame->Field(ID3FN_TEXT) = text;
        tag->AttachFrame(pFrame);
      }
    }
  }
  
  return pFrame;
}

size_t ID3_RemoveTitles(ID3_Tag *tag)
{
  size_t nRemoved = 0;
  ID3_Frame *pFrame = NULL;

  if (NULL == tag)
  {
    return nRemoved;
  }

  while ((pFrame = tag->Find(ID3FID_TITLE)))
  {
    tag->RemoveFrame(pFrame);
    nRemoved++;
  }

  return nRemoved;
}

char *ID3_GetYear(const ID3_Tag *tag)
{
  char *sYear = NULL;
  if (NULL == tag)
  {
    return sYear;
  }

  ID3_Frame *pFrame = tag->Find(ID3FID_YEAR);
  if (pFrame != NULL)
  {
    sYear = ID3_GetString(pFrame, ID3FN_TEXT);
  }
  return sYear;
}

ID3_Frame* ID3_AddYear(ID3_Tag *tag, const char *text, bool bReplace)
{
  ID3_Frame* pFrame = NULL;
  if (NULL != tag && NULL != text && strlen(text) > 0)
  {
    if (bReplace)
    {
      ID3_RemoveYears(tag);
    }
    if (bReplace || tag->Find(ID3FID_YEAR) == NULL)
    {
      pFrame = new ID3_Frame(ID3FID_YEAR);
      if (NULL != pFrame)
      {
        pFrame->Field(ID3FN_TEXT) = text;
        tag->AttachFrame(pFrame);
      }
    }
  }
  
  return pFrame;
}

size_t ID3_RemoveYears(ID3_Tag *tag)
{
  size_t nRemoved = 0;
  ID3_Frame *pFrame = NULL;

  if (NULL == tag)
  {
    return nRemoved;
  }

  while ((pFrame = tag->Find(ID3FID_YEAR)))
  {
    tag->RemoveFrame(pFrame);
    nRemoved++;
  }

  return nRemoved;
}

char *ID3_GetComment(const ID3_Tag *tag)
{
  char *sComment = NULL;
  if (NULL == tag)
  {
    return sComment;
  }

  ID3_Frame *pFrame = tag->Find(ID3FID_COMMENT);
  if (pFrame != NULL)
  {
    sComment = ID3_GetString(pFrame, ID3FN_TEXT);
  }
  return sComment;
}

ID3_Frame* ID3_AddComment(ID3_Tag *tag, const char *sComment,
                          const char *sDescription, bool bReplace)
{
  ID3_Frame* pFrame = NULL;
  if (NULL != tag          &&
      NULL != sComment     &&
      NULL != sDescription && 
      strlen(sComment) > 0)
  {
    bool bAdd = true;
    if (bReplace)
    {
      ID3_RemoveComments(tag, sDescription);
    }
    else
    {
      // See if there is already a comment with this description
      for (size_t nCount = 0; nCount < tag->NumFrames(); nCount++)
      {
        pFrame = tag->GetFrameNum(nCount);
        if (pFrame->GetID() == ID3FID_COMMENT)
        {
          char *sDesc = ID3_GetString(pFrame, ID3FN_DESCRIPTION);
          if (strcmp(sDesc, sDescription) == 0)
          {
            bAdd = false;
          }
          delete [] sDesc;
          if (!bAdd)
          {
            break;
          }
        }
      }
    }
    if (bAdd)
    {
      pFrame = new ID3_Frame(ID3FID_COMMENT);
      if (NULL != pFrame)
      {
        pFrame->Field(ID3FN_LANGUAGE) = "eng";
        pFrame->Field(ID3FN_DESCRIPTION) = sDescription;
        pFrame->Field(ID3FN_TEXT) = sComment;
        tag->AttachFrame(pFrame);
      }
    }
  }
  return pFrame;
}

// Remove all comments with the given description (remove all comments if
// sDescription is NULL)
size_t ID3_RemoveComments(ID3_Tag *tag, const char *sDescription)
{
  size_t nRemoved = 0;

  if (NULL == tag)
  {
    return nRemoved;
  }

  for (size_t nCount = 0; nCount < tag->NumFrames(); nCount++)
  {
    ID3_Frame *pFrame = tag->GetFrameNum(nCount);
    if (pFrame->GetID() == ID3FID_COMMENT)
    {
      bool bRemove = false;
      // A null description means remove all comments
      if (NULL == sDescription)
      {
        bRemove = true;
      }
      else
      {
        // See if the description we have matches the description of the 
        // current comment.  If so, set the "remove the comment" flag to true.
        char *sDesc = ID3_GetString(pFrame, ID3FN_DESCRIPTION);
        if (strcmp(sDesc, sDescription) == 0)
        {
          bRemove = true;
        }
        delete [] sDesc;
      }
      if (bRemove)
      {
        tag->RemoveFrame(pFrame);
        nRemoved++;
      }
    }
  }

  return nRemoved;
}

char *ID3_GetTrack(const ID3_Tag *tag)
{
  char *sTrack = NULL;
  if (NULL == tag)
  {
    return sTrack;
  }

  ID3_Frame *pFrame = tag->Find(ID3FID_TRACKNUM);
  if (pFrame != NULL)
  {
    sTrack = ID3_GetString(pFrame, ID3FN_TEXT);
  }
  return sTrack;
}

size_t ID3_GetTrackNum(const ID3_Tag *tag)
{
  char *sTrack = ID3_GetTrack(tag);
  size_t nTrack = 0;
  if (NULL != sTrack)
  {
    nTrack = atoi(sTrack);
    delete [] sTrack;
  }
  return nTrack;
}

ID3_Frame* ID3_AddTrack(ID3_Tag *tag, uchar ucTrack, uchar ucTotal, bool bReplace)
{
  ID3_Frame* pFrame = NULL;
  if (NULL != tag && ucTrack > 0)
  {
    if (bReplace)
    {
      ID3_RemoveTracks(tag);
    }
    if (bReplace || NULL == tag->Find(ID3FID_TRACKNUM))
    {
      ID3_Frame *trackFrame = new ID3_Frame(ID3FID_TRACKNUM);
      if (trackFrame)
      {
        char *sTrack = NULL;
        if (0 == ucTotal)
        {
          sTrack = new char[4];
          sprintf(sTrack, "%lu", (luint) ucTrack);
        }
        else
        {
          sTrack = new char[8];
          sprintf(sTrack, "%lu/%lu", (luint) ucTrack, (luint) ucTotal);
        }
        
        trackFrame->Field(ID3FN_TEXT) = sTrack;
        tag->AttachFrame(trackFrame);

        delete [] sTrack;
      }
    }
  }
  
  return pFrame;
}

size_t ID3_RemoveTracks(ID3_Tag *tag)
{
  size_t nRemoved = 0;
  ID3_Frame *pFrame = NULL;

  if (NULL == tag)
  {
    return nRemoved;
  }

  while ((pFrame = tag->Find(ID3FID_TRACKNUM)))
  {
    tag->RemoveFrame(pFrame);
    nRemoved++;
  }

  return nRemoved;
}

char *ID3_GetGenre(const ID3_Tag *tag)
{
  char *sGenre = NULL;
  if (NULL == tag)
  {
    return sGenre;
  }

  ID3_Frame *pFrame = tag->Find(ID3FID_CONTENTTYPE);
  if (pFrame != NULL)
  {
    sGenre = ID3_GetString(pFrame, ID3FN_TEXT);
  }

  return sGenre;
}

size_t ID3_GetGenreNum(const ID3_Tag *tag)
{
  char *sGenre = ID3_GetGenre(tag);
  size_t ulGenre = 0xFF;
  if (NULL == sGenre)
  {
    return ulGenre;
  }

  // If the genre string begins with "(ddd)", where "ddd" is a number, then 
  // "ddd" is the genre number---get it
  if (sGenre[0] == '(')
  {
    char *pCur = &sGenre[1];
    while (isdigit(*pCur))
    {
      pCur++;
    }
    if (*pCur == ')')
    {
      // if the genre number is greater than 255, its invalid.
      ulGenre = MIN(0xFF, atoi(&sGenre[1]));
    }
  }

  delete [] sGenre;
  return ulGenre;
}

ID3_Frame* ID3_AddGenre(ID3_Tag *tag, size_t ucGenre, bool bReplace)
{
  ID3_Frame* pFrame = NULL;
  if (NULL != tag && 0xFF != ucGenre)
  {
    if (bReplace)
    {
      ID3_RemoveGenres(tag);
    }
    if (bReplace || NULL == tag->Find(ID3FID_CONTENTTYPE))
    {
      pFrame = new ID3_Frame(ID3FID_CONTENTTYPE);
      if (NULL != pFrame)
      {
        char sGenre[6];
        sprintf(sGenre, "(%lu)", (luint) ucGenre);

        pFrame->Field(ID3FN_TEXT) = sGenre;
        tag->AttachFrame(pFrame);
      }
    }
  }
  
  return pFrame;
}

size_t ID3_RemoveGenres(ID3_Tag *tag)
{
  size_t nRemoved = 0;
  ID3_Frame *pFrame = NULL;

  if (NULL == tag)
  {
    return nRemoved;
  }

  while ((pFrame = tag->Find(ID3FID_CONTENTTYPE)))
  {
    tag->RemoveFrame(pFrame);
    nRemoved++;
  }

  return nRemoved;
}

char *ID3_GetLyrics(const ID3_Tag *tag)
{
  char *sLyrics = NULL;
  if (NULL == tag)
  {
    return sLyrics;
  }

  ID3_Frame *pFrame = tag->Find(ID3FID_UNSYNCEDLYRICS);
  if (pFrame != NULL)
  {
    sLyrics = ID3_GetString(pFrame, ID3FN_TEXT);
  }
  return sLyrics;
}

ID3_Frame* ID3_AddLyrics(ID3_Tag *tag, const char *text, bool bReplace)
{
  ID3_Frame* pFrame = NULL;
  if (NULL != tag && strlen(text) > 0)
  {
    if (bReplace)
    {
      ID3_RemoveLyrics(tag);
    }
    if (bReplace || tag->Find(ID3FID_UNSYNCEDLYRICS) == NULL)
    {
      pFrame = new ID3_Frame(ID3FID_UNSYNCEDLYRICS);
      if (NULL != pFrame)
      {
        pFrame->Field(ID3FN_LANGUAGE) = "eng";
        pFrame->Field(ID3FN_TEXT) = text;
        tag->AttachFrame(pFrame);
      }
    }
  }
  
  return pFrame;
}

size_t ID3_RemoveLyrics(ID3_Tag *tag)
{
  size_t nRemoved = 0;
  ID3_Frame *pFrame = NULL;

  if (NULL == tag)
  {
    return nRemoved;
  }

  while ((pFrame = tag->Find(ID3FID_UNSYNCEDLYRICS)))
  {
    tag->RemoveFrame(pFrame);
    nRemoved++;
  }

  return nRemoved;
}

char *ID3_GetLyricist(const ID3_Tag *tag)
{
  char *sLyricist = NULL;
  if (NULL == tag)
  {
    return sLyricist;
  }

  ID3_Frame *pFrame = tag->Find(ID3FID_LYRICIST);
  if (pFrame != NULL)
  {
    sLyricist = ID3_GetString(pFrame, ID3FN_TEXT);
  }
  return sLyricist;
}

ID3_Frame* ID3_AddLyricist(ID3_Tag *tag, const char *text, bool bReplace)
{
  ID3_Frame* pFrame = NULL;
  if (NULL != tag && NULL != text && strlen(text) > 0)
  {
    if (bReplace)
    {
      ID3_RemoveLyricist(tag);
    }
    if (bReplace || (tag->Find(ID3FID_LYRICIST) == NULL))
    {    
      pFrame = new ID3_Frame(ID3FID_LYRICIST);
      if (pFrame)
      {
        pFrame->Field(ID3FN_TEXT) = text;
        tag->AttachFrame(pFrame);
      }
    }
  }

  return pFrame;
}

size_t ID3_RemoveLyricist(ID3_Tag *tag)
{
  size_t nRemoved = 0;
  ID3_Frame *pFrame = NULL;

  if (NULL == tag)
  {
    return nRemoved;
  }

  while ((pFrame = tag->Find(ID3FID_LYRICIST)))
  {
    tag->RemoveFrame(pFrame);
    nRemoved++;
  }

  return nRemoved;
}

ID3_Frame* ID3_AddSyncLyrics(ID3_Tag *tag, const char *lang, const char *desc,
                             const uchar *text, size_t textsize, bool bReplace)
{
  ID3_Frame* pFrame = NULL;
  // language and descriptor should be mandatory
  if ((NULL == lang) || (NULL == desc))
  {
    return NULL;
  }

  // check if a SYLT frame of this language or descriptor already exists
  ID3_Frame* pFrameExist = tag->Find(ID3FID_SYNCEDLYRICS, ID3FN_LANGUAGE, 
                                     (char *) lang);
  if (!pFrameExist)
  {
    pFrameExist = tag->Find(ID3FID_SYNCEDLYRICS, ID3FN_DESCRIPTION, 
                            (char *) desc);
  }

  if (NULL != tag && NULL != text)
  {
    if (bReplace && pFrameExist)
    {
      tag->RemoveFrame (pFrameExist);
      pFrameExist = NULL;
    }

    // if the frame still exist, cannot continue
    if (pFrameExist)
    {
      return NULL;
    }

    ID3_Frame* pFrame = new ID3_Frame(ID3FID_SYNCEDLYRICS);
    if (NULL == pFrame)
    {
      ID3_THROW(ID3E_NoMemory);
    }

    pFrame->Field(ID3FN_LANGUAGE) = lang;
    pFrame->Field(ID3FN_DESCRIPTION) = desc;
    pFrame->Field(ID3FN_DATA).Set(text, textsize);
    tag->AttachFrame(pFrame);
  }

  return pFrame;
}

ID3_Frame *ID3_GetSyncLyricsInfo(const ID3_Tag *tag, const char *lang, 
                                 const char *desc, size_t& stampformat, 
                                 size_t& type, size_t& size)
{
  // check if a SYLT frame of this language or descriptor exists
  ID3_Frame* pFrameExist = NULL;
  if (NULL != lang)
  {
    // search through language
    pFrameExist = tag->Find(ID3FID_SYNCEDLYRICS, ID3FN_LANGUAGE, lang);
  }
  else if (NULL != desc)
  {
    // search through descriptor
    pFrameExist = tag->Find(ID3FID_SYNCEDLYRICS, ID3FN_DESCRIPTION, desc);
  }
  else
  {
    // both language and description not specified, search the first SYLT frame
    pFrameExist = tag->Find(ID3FID_SYNCEDLYRICS);
  }
  
  if (!pFrameExist)
  {
    return NULL;
  }
  
  // get the lyrics time stamp format
  stampformat = pFrameExist->Field (ID3FN_TIMESTAMPFORMAT).Get ();
  
  // get the lyrics content type
  type = pFrameExist->Field (ID3FN_CONTENTTYPE).Get ();
  
  // get the lyrics size
  size = pFrameExist->Field (ID3FN_DATA).Size ();
  
  // return the frame pointer for further uses
  return pFrameExist;
}

ID3_Frame *ID3_GetSyncLyrics(const ID3_Tag *tag, const char *lang, 
                             const char *desc, const uchar *pData, size_t& size)
{
  // check if a SYLT frame of this language or descriptor exists
  ID3_Frame* pFrameExist = NULL;
  if (NULL != lang)
  {
    // search through language
    pFrameExist = tag->Find(ID3FID_SYNCEDLYRICS, ID3FN_LANGUAGE, lang);
  }
  else if (NULL != desc)
  {
    // search through descriptor
    pFrameExist = tag->Find(ID3FID_SYNCEDLYRICS, ID3FN_DESCRIPTION, desc);
  }
  else
  {
    // both language and description not specified, search the first SYLT frame
    pFrameExist = tag->Find(ID3FID_SYNCEDLYRICS);
  }

  if (NULL == pFrameExist)
  {
    return NULL;
  }
  
  // get the lyrics size
  size_t datasize = pFrameExist->Field(ID3FN_DATA).Size();
  size = MIN(size, datasize);

  // get the lyrics data
  pData = pFrameExist->Field (ID3FN_DATA).GetBinary();

  // return the frame pointer for further uses
  return pFrameExist;
}

