// $Id: tag_find.cpp,v 1.1 2002/01/21 08:16:22 menno Exp $

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
#include "tag.h"
#include "utils.h"

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

  /** Returns a pointer to the next ID3_Frame with the given ID3_FrameID;
   ** returns NULL if no such frame found.
   ** 
   ** If there are multiple frames in the tag with the same ID (which, for some
   ** frames, is allowed), then subsequent calls to <a href="#Find">Find</a>
   ** will return subsequent frame pointers, wrapping if necessary.
   ** 
   ** \code
   **   ID3_Frame *myFrame;
   **   if (myFrame = myTag.Find(ID3FID_TITLE))
   **   {
   **     // do something with the frame, like copy
   **     // the contents into a buffer, display the
   **     // contents in a window, etc.
   **     // ...
   **   }
   ** \endcode
   ** 
   ** You may optionally supply to more parameters ot this method, being an
   ** ID3_FieldID and a value of some sort.  Depending on the field name/ID you
   ** supply, you may supply an integer, a char* or a unicode_t* as the third
   ** parameter.  If you supply an ID3_FrameID, you must also supply a data
   ** value to compare against.
   ** 
   ** This method will then return the first frame that has a matching frame
   ** ID, and which has a field with the same name as that which you supplied
   ** in the second parameter, whose calue matches that which you supplied as
   ** the third parameter.  For example:
   ** 
   ** \code
   **   ID3_Frame *myFrame;
   **   if (myFrame = myTag.Find(ID3FID_TITLE, ID3FN_TEXT, "Nirvana"))
   **   {
   **     // found it, do something with it.
   **     // ...
   **   }
   ** \endcode
   **     
   ** This example will return the first TITLE frame and whose TEXT field is
   ** 'Nirvana'.  Currently there is no provision for things like 'contains',
   ** 'greater than', or 'less than'.  If there happens to be more than one of
   ** these frames, subsequent calls to the <a href="#Find">Find</a> method
   ** will return subsequent frames and will wrap around to the beginning.
   ** 
   ** Another example...
   ** 
   ** \code
   **   ID3_Frame *myFrame;
   **   if (myFrame = myTag.Find(ID3FID_COMMENT, ID3FN_TEXTENC, ID3TE_UNICODE))
   **   {
   **     // found it, do something with it.
   **     // ...
   **   }
   ** \endcode
   ** 
   ** This returns the first COMMENT frame that uses Unicode as its text
   ** encdoing.
   **  
   ** @name   Find
   ** @param  id The ID of the frame that is to be located
   ** @return A pointer to the first frame found that has the given frame id,
   **         or NULL if no such frame.
   **/
ID3_Elem *ID3_Tag::Find(const ID3_Frame *frame) const
{
  ID3_Elem *elem = NULL;
  
  for (ID3_Elem *cur = __frames; NULL != cur; cur = cur->pNext)
  {
    if (cur->pFrame == frame)
    {
      elem = cur;
      break;
    }
  }
  
  return elem;
}

ID3_Frame *ID3_Tag::Find(ID3_FrameID id) const
{
  ID3_Frame *frame = NULL;
  
  // reset the cursor if it isn't set
  if (NULL == __cursor)
  {
    __cursor = __frames;
  }

  for (int iCount = 0; iCount < 2 && frame == NULL; iCount++)
  {
    // We want to cycle through the list to find the matching frame.  We
    // should start from the cursor, search each successive frame, wrapping
    // if necessary.  The enclosing loop and the assignment statments below
    // ensure that we first start at the cursor and search to the end of the
    // list and, if unsuccessful, start from the beginning of the list and
    // search to the cursor.
    ID3_Elem
      *pStart  = (0 == iCount ? __cursor : __frames), 
      *pFinish = (0 == iCount ? NULL          : __cursor);
    // search from the cursor to the end
    for (ID3_Elem *cur = pStart; cur != pFinish; cur = cur->pNext)
    {
      if ((cur->pFrame != NULL) && (cur->pFrame->GetID() == id))
      {
        // We've found a valid frame.  Set the cursor to be the next element
        frame = cur->pFrame;
        __cursor = cur->pNext;
        break;
      }
    }
  }
  
  return frame;
}

ID3_Frame *ID3_Tag::Find(ID3_FrameID id, ID3_FieldID fld, const char *data) const
{
  ID3_Frame *frame = NULL;
  unicode_t *temp;
  
  temp = new unicode_t[strlen(data) + 1];
  if (NULL == temp)
    ID3_THROW(ID3E_NoMemory);

  mbstoucs(temp, data, strlen(data) + 1);
    
  frame = Find(id, fld, temp);
    
  delete[] temp;
  
  return frame;
}

ID3_Frame *ID3_Tag::Find(ID3_FrameID id, ID3_FieldID fld, const unicode_t *data) const
{
  ID3_Frame *frame = NULL;
  
  // reset the cursor if it isn't set
  if (NULL == __cursor)
    __cursor = __frames;

  for (int iCount = 0; iCount < 2 && frame == NULL; iCount++)
  {
    // We want to cycle through the list to find the matching frame.  We
    // should start from the cursor, search each successive frame, wrapping
    // if necessary.  The enclosing loop and the assignment statments below
    // ensure that we first start at the cursor and search to the end of the
    // list and, if unsuccessful, start from the beginning of the list and
    // search to the cursor.
    ID3_Elem
      *pStart  = (0 == iCount ? __cursor : __frames), 
      *pFinish = (0 == iCount ? NULL          : __cursor);
    // search from the cursor to the end
    for (ID3_Elem *cur = pStart; cur != pFinish; cur = cur->pNext)
    {
      if ((cur->pFrame != NULL) && (cur->pFrame->GetID() == id) &&
          (data != NULL) && ucslen(data) > 0 && 
          cur->pFrame->Contains(fld))
      {
        size_t ulSize = cur->pFrame->Field(fld).BinSize();
        unicode_t *wsBuffer = new unicode_t[ulSize];
          
        if (NULL == wsBuffer)
          ID3_THROW(ID3E_NoMemory);
          
        cur->pFrame->Field(fld).Get(wsBuffer, ulSize);
          
        bool bInFrame = (ucscmp(wsBuffer, data) == 0);
          
        delete [] wsBuffer;

        if (bInFrame)
        {
          // We've found a valid frame.  Set cursor to be the next element
          frame = cur->pFrame;
          __cursor = cur->pNext;
          break;
        }
      }
    }
  }
  
  return frame;
}

ID3_Frame *ID3_Tag::Find(ID3_FrameID id, ID3_FieldID fld, uint32 data) const
{
  ID3_Frame *frame = NULL;
  
  // reset the cursor if it isn't set
  if (NULL == __cursor)
    __cursor = __frames;

  for (int iCount = 0; iCount < 2 && frame == NULL; iCount++)
  {
    // We want to cycle through the list to find the matching frame.  We
    // should start from the cursor, search each successive frame, wrapping
    // if necessary.  The enclosing loop and the assignment statments below
    // ensure that we first start at the cursor and search to the end of the
    // list and, if unsuccessful, start from the beginning of the list and
    // search to the cursor.
    ID3_Elem
      *pStart  = (0 == iCount ? __cursor : __frames), 
      *pFinish = (0 == iCount ? NULL          : __cursor);
    // search from the cursor to the end
    for (ID3_Elem *cur = pStart; cur != pFinish; cur = cur->pNext)
    {
      if ((cur->pFrame != NULL) && (cur->pFrame->GetID() == id) &&
          (cur->pFrame->Field(fld).Get() == data))
      {
        // We've found a valid frame.  Set the cursor to be the next element
        frame = cur->pFrame;
        __cursor = cur->pNext;
        break;
      }
    }
  }
  
  return frame;
}

  /** Returns a pointer to the frame with the given index; returns NULL if
   ** there is no such frame at that index.
   ** 
   ** Optionally, <a href="#operator[]">operator[]</a> can be used as an
   ** alternative to this method.  Indexing is 0-based (that is, the first
   ** frame is number 0, and the last frame in a tag that holds n frames is
   ** n-1).
   ** 
   ** If you wish to have a more comlex searching facility, then at least for
   ** now you will have to devise it yourself and implement it useing these
   ** methods.
   ** 
   ** @param nIndex The index of the frame that is to be retrieved
   ** @return A pointer to the requested frame, or NULL if no such frame.
   **/
ID3_Frame *ID3_Tag::GetFrameNum(index_t num) const
{
  const size_t num_frames = this->NumFrames();
  if (num >= num_frames)
  {
    return NULL;
  }

  ID3_Frame *frame = NULL;
  index_t curNum = num_frames;
  for (ID3_Elem *cur = __frames; cur != NULL; cur = cur->pNext)
  {
    // compare and advance counter
    if (num == --curNum)
    {
      frame = cur->pFrame;
      break;
    }
  }
  
  return frame;
}

/** Returns a pointer to the frame with the given index; returns NULL if
 ** there is no such frame at that index.
 ** 
 ** @name operator[]
 ** @param nIndex The index of the frame that is to be retrieved
 ** @return A pointer to the requested frame, or NULL if no such frame. 
 ** @see #GetFrameNum
 **/
ID3_Frame *ID3_Tag::operator[](index_t num) const
{
  return GetFrameNum(num);
}
