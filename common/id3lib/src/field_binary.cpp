// $Id: field_binary.cpp,v 1.1 2002/01/21 08:16:21 menno Exp $

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


#include <stdio.h>
#include <memory.h>
#include "field.h"

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

void ID3_Field::Set(const uchar *newData, size_t newSize)
{
  Clear();
  
  if (newSize > 0)
  {
    __data = new uchar[newSize];
    if (NULL == __data)
    {
      ID3_THROW(ID3E_NoMemory);
    }
    
    if (newData != NULL)
    {
      memcpy(__data, newData, newSize);
    }
    else
    {
      memset(__data, 0, newSize);
    }
    __size = newSize;
    
    __type = ID3FTY_BINARY;
    __changed = true;
  }
  
  return ;
}


void ID3_Field::Get(uchar *buffer, size_t buffLength) const
{
  if (NULL == buffer)
  {
    ID3_THROW(ID3E_NoBuffer);
  }
    
  if (__data != NULL && __size > 0)
  {
    memcpy(buffer, __data, MIN(buffLength, __size));
  }
}


void ID3_Field::FromFile(const char *info)
{
  if (!info)
  {
    return;
  }
    
  FILE* temp_file = fopen(info, "rb");
  if (temp_file != NULL)
  {
    fseek(temp_file, 0, SEEK_END);
    size_t fileSize = ftell(temp_file);
    fseek(temp_file, 0, SEEK_SET);
    
    uchar* buffer = new uchar[fileSize];
    if (buffer != NULL)
    {
      fread(buffer, 1, fileSize, temp_file);
      
      this->Set(buffer, fileSize);
      
      delete [] buffer;
    }
    
    fclose(temp_file);
  }
  
  return ;
}


void ID3_Field::ToFile(const char *info) const
{
  if (NULL == info)
  {
    ID3_THROW(ID3E_NoData);
  }
    
  if ((__data != NULL) && (__size > 0))
  {
    FILE* temp_file = fopen(info, "wb");
    if (temp_file != NULL)
    {
      fwrite(__data, 1, __size, temp_file);
      fclose(temp_file);
    }
  }
  
  return ;
}


size_t
ID3_Field::ParseBinary(const uchar *buffer, size_t nSize)
{
  // copy the remaining bytes, unless we're fixed length, in which case copy
  // the minimum of the remaining bytes vs. the fixed length
  size_t bytesUsed = (__length > 0 ? MIN(nSize, __length) : nSize);
  this->Set(buffer, bytesUsed);
  __changed = false;
  
  return bytesUsed;
}


size_t ID3_Field::RenderBinary(uchar *buffer) const
{
  size_t bytesUsed = BinSize();
  memcpy(buffer, (uchar *) __data, bytesUsed);
  __changed = false;
  return bytesUsed;
}
