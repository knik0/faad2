// $Id: header_frame.cpp,v 1.1 2002/01/21 08:16:22 menno Exp $

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
#include <memory.h>
#include "header_frame.h"
#include "error.h"
#include "utils.h"

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

void ID3_FrameHeader::SetUnknownFrame(const char* id)
{
  Clear();
  __frame_def = new ID3_FrameDef;
  if (NULL == __frame_def)
  {
    // log this;
    return;
  }
  __frame_def->eID = ID3FID_NOFRAME;
  __frame_def->bTagDiscard = false;
  __frame_def->bFileDiscard = false;
  __frame_def->aeFieldDefs = (ID3_FieldDef *) ID3_FieldDef::DEFAULT;
  if (strlen(id) <= 3)
  {
    strcpy(__frame_def->sShortTextID, id);
    strcpy(__frame_def->sLongTextID, "");
  }
  else
  {
    strncpy(__frame_def->sLongTextID, id, 4);
    strcpy(__frame_def->sShortTextID, "");
  }
  __dyn_frame_def = true;
}

bool ID3_FrameHeader::SetFrameID(ID3_FrameID id)
{
  if (id == ID3FID_NOFRAME || id == this->GetFrameID())
  {
    return false;
  }
  __frame_def = ID3_FindFrameDef(id);
  __flags.set(TAGALTER, __frame_def->bTagDiscard);
  __flags.set(FILEALTER, __frame_def->bFileDiscard);

  __changed = true;
  return true;
}

size_t ID3_FrameHeader::Size() const
{
  if (!__info)
  {
    return 0;
  }
  return 
    __info->frame_bytes_id   + 
    __info->frame_bytes_size + 
    __info->frame_bytes_flags;
}

size_t ID3_FrameHeader::Parse(const uchar *buffer, size_t size)
{
  size_t index = 0;
  char text_id[5];

  if (!__info)
  {
    return 0;
  }

  strncpy(text_id, (char *) buffer, __info->frame_bytes_id);
  text_id[__info->frame_bytes_id] = '\0';
  index += __info->frame_bytes_id;

  ID3_FrameID fid = ID3_FindFrameID(text_id);
  if (ID3FID_NOFRAME == fid)
  {
    this->SetUnknownFrame(text_id);
  }
  else
  {
    this->SetFrameID(fid);
  }

  this->SetDataSize(ParseNumber(&buffer[index], __info->frame_bytes_size));
  index += __info->frame_bytes_size;

  __flags.add(ParseNumber(&buffer[index], __info->frame_bytes_flags));
  index += __info->frame_bytes_flags;
  
  return index;
}

size_t ID3_FrameHeader::Render(uchar *buffer) const
{
  size_t size = 0;

  if (NULL == __frame_def)
  {
    // TODO: log this
    return 0;
    //ID3_THROW(ID3E_InvalidFrameID);
  }
  char *text_id;
  if (__info->frame_bytes_id == strlen(__frame_def->sShortTextID))
  {
    text_id = __frame_def->sShortTextID;
  }
  else
  {
    text_id = __frame_def->sLongTextID;
  }

  memcpy(&buffer[size], (uchar *) text_id, __info->frame_bytes_id);
  size += __info->frame_bytes_id;
  
  size += RenderNumber(&buffer[size], __data_size, __info->frame_bytes_size);
  size += RenderNumber(&buffer[size], __flags.get(), __info->frame_bytes_flags);
  
  return size;
}

const char* ID3_FrameHeader::GetTextID() const
{
  char *text_id = "";
  if (__info && __frame_def)
  {
    if (__info->frame_bytes_id == strlen(__frame_def->sShortTextID))
    {
      text_id = __frame_def->sShortTextID;
    }
    else
    {
      text_id = __frame_def->sLongTextID;
    }
  }
  return text_id;
}

ID3_FrameHeader& ID3_FrameHeader::operator=(const ID3_FrameHeader& hdr)
{
  if (this != &hdr)
  {
    this->Clear();
    this->ID3_Header::operator=(hdr);
    if (!hdr.__dyn_frame_def)
    {
      __frame_def = hdr.__frame_def;
    }
    else
    {
      __frame_def = new ID3_FrameDef;
      if (NULL == __frame_def)
      {
        // TODO: throw something here...
      }
      __frame_def->eID = hdr.__frame_def->eID;
      __frame_def->bTagDiscard = hdr.__frame_def->bTagDiscard;
      __frame_def->bFileDiscard = hdr.__frame_def->bFileDiscard;
      __frame_def->aeFieldDefs = hdr.__frame_def->aeFieldDefs;
      strcpy(__frame_def->sShortTextID, hdr.__frame_def->sShortTextID);
      strcpy(__frame_def->sLongTextID, hdr.__frame_def->sLongTextID);
      __dyn_frame_def = true;
    }
  }
  return *this;
}

ID3_FrameID ID3_FrameHeader::GetFrameID() const
{
  ID3_FrameID eID = ID3FID_NOFRAME;
  if (NULL != __frame_def)
  {
    eID = __frame_def->eID;
  }

  return eID;
}

const ID3_FrameDef *ID3_FrameHeader::GetFrameDef() const
{
  return __frame_def;
}

bool ID3_FrameHeader::Clear()
{
  bool changed = this->ID3_Header::Clear();
  if (__dyn_frame_def)
  {
    delete __frame_def;
    __dyn_frame_def = false;
    changed = true;
  }
  if (__frame_def)
  {
    __frame_def = NULL;
    changed = true;
  }
  return changed;
}
