// id3lib: a C++ library for creating and manipulating id3v1/v2 tags
// $Id: uint28.cpp,v 1.1 2002/01/21 08:16:22 menno Exp $

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

#include "uint28.h"
#include <string.h>
#include <iomanip.h>

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

uint28& uint28::operator=(const uchar* const data)
{
  __value = 0;
  for (size_t i = 0; i < sizeof(uint32); ++i)
  {
    __value = (__value << 7) | static_cast<uint32>(data[i]) & MASK7;
  }
  return *this;
}

void uint28::Render(uchar* data) const
{
  memset(data, '\0', sizeof(uint32));
  
  for (uint32 val = this->to_uint32(), i = 0; i < sizeof(uint32); 
       val >>= 7, ++i)
  {
    data[sizeof(uint32) - i - 1] = static_cast<uchar>(val & MASK7);
  }
  
  // return data;
}

ostream& operator<<(ostream& os, uint28& val)
{
  uchar data[sizeof(uint32)];
  val.Render(data);
  for (uchar* p = data; p != data + sizeof(uint32); ++p)
  {
    os << *p;
  }
  return os;
}


istream& operator>>(istream& in, uint28& val)
{
  uchar data[sizeof(uint32) + 1];
  in >> setw(sizeof(uint32) + 1) >> data;
  val = data;
  return in;
}
