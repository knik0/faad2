// $Id: header_tag.cpp,v 1.1 2002/01/21 08:16:22 menno Exp $

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

#include "header_tag.h"
#include "uint28.h"
#include "utils.h"
#include "tag.h"
#include <string.h>

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

const char* const ID3_TagHeader::ID = "ID3";

bool ID3_TagHeader::SetSpec(ID3_V2Spec spec)
{
  bool changed = this->ID3_Header::SetSpec(spec);
  if (changed)
  {
    if (__info)
    {
      __flags.set(EXPERIMENTAL, __info->is_experimental);
      __flags.set(EXTENDED,     __info->is_extended);
    }
  }
  return changed;
}

size_t ID3_TagHeader::Size() const
{
  size_t bytesUsed = ID3_TagHeader::SIZE;
  
  if (__info->is_extended)
  {
    bytesUsed += __info->extended_bytes + sizeof(uint32);
  }
  
  return bytesUsed;
}


size_t ID3_TagHeader::Render(uchar *buffer) const
{
  size_t size = 0;
  
  memcpy(&buffer[size], (uchar *) ID, strlen(ID));
  size += strlen(ID);
  
  buffer[size++] = ID3_V2SpecToVer(ID3V2_LATEST);
  buffer[size++] = ID3_V2SpecToRev(ID3V2_LATEST);
  
  // set the flags byte in the header
  buffer[size++] = static_cast<uchar>(__flags.get() & MASK8);
  
  uint28 size28(this->GetDataSize());
  size28.Render(&buffer[size]);
  size += sizeof(uint32);

  // now we render the extended header
  if (__flags.test(EXTENDED))
  {
    size += RenderNumber(&buffer[size], __info->extended_bytes);
  }
  
  return size;
}

size_t ID3_TagHeader::Parse(const uchar* data, size_t size)
{
  if (!(ID3_IsTagHeader(data) > 0))
  {
    return 0;
  }

  size_t tag_size = SIZE;

  // The spec version is determined with the MAJOR and MINOR OFFSETs
  uchar major = data[MAJOR_OFFSET];
  uchar minor = data[MINOR_OFFSET];
  this->SetSpec(ID3_VerRevToV2Spec(major, minor));

  // Get the flags at the appropriate offset
  __flags.set(static_cast<ID3_Flags::TYPE>(data[FLAGS_OFFSET]));

  // set the data size
  uint28 data_size = uint28(&data[SIZE_OFFSET]);
  this->SetDataSize(data_size.to_uint32());
  
  if (__flags.test(EXTENDED))
  {
    if (this->GetSpec() == ID3V2_2_1)
    {
      // okay, if we are ID3v2.2.1, then let's skip over the extended header
      // for now because I am lazy
    }

    if (this->GetSpec() == ID3V2_3_0)
    {
      // okay, if we are ID3v2.3.0, then let's actually parse the extended
      // header (for now, we skip it because we are lazy)
    }
  }
  
  return tag_size;
}
