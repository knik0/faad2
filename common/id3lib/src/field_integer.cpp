// $Id: field_integer.cpp,v 1.1 2002/01/21 08:16:21 menno Exp $

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

#include "field.h"
#include "utils.h"

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

ID3_Field& ID3_Field::operator=(uint32 newData)
{
  this->Set(newData);
  return *this;
}


void ID3_Field::Set(uint32 newData)
{
  Clear();
  
  // Ack!  This is terrible!
  __data = (uchar *) newData;
  __size = sizeof(uint32);
  __type = ID3FTY_INTEGER;
  __changed = true;
  
  return ;
}


uint32 ID3_Field::Get() const
{
  return (uint32) __data;
}


size_t ID3_Field::ParseInteger(const uchar *buffer, size_t nSize)
{
  size_t nBytes = 0;

  if (buffer != NULL && nSize > 0)
  {
    nBytes = MIN(nSize, sizeof(uint32));
    
    if (__length > 0)
    {
      nBytes = MIN(__length, nBytes);
    }

    Set(ParseNumber(buffer, nBytes));
    __changed = false;
  }
  
  return nBytes;
}


size_t ID3_Field::RenderInteger(uchar *buffer) const
{
  size_t bytesUsed = RenderNumber(buffer, (uint32) __data, this->BinSize());
  __changed = false;
  return bytesUsed;
}
