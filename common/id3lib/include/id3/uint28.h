// id3lib: a C++ library for creating and manipulating id3v1/v2 tags
// $Id: uint28.h,v 1.1 2002/01/21 08:16:21 menno Exp $

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

#ifndef __ID3LIB_UINT28_H__
#define __ID3LIB_UINT28_H__

#include "globals.h"
#include <iostream.h>

class uint28
{
  uint32 __value;
public:
  uint28(uint32 val = 0) : __value(val) { ; }
  uint28(const uchar* const data) { *this = data; }
  uint28(const uint28& rhs) : __value(rhs.to_uint32()) { ; }
  ~uint28() { ; }
  
  uint28&    operator=(const uchar* const);
  
  uint28&    operator=(uint32 val) 
  { 
    __value = val & MASK(28); 
    return *this; 
  }
  
  uint28&    operator=(const uint28& rhs) 
  { 
    if (this != &rhs)
    {
      __value = rhs.to_uint32(); 
    }
    return *this; 
  }
  
  uint32    to_uint32() const 
  { 
    return __value; 
  }
  
  size_t    Parse(const uchar* const data) 
  { 
    *this = data;
    return sizeof(uint32);
  }
  
  void   Render(uchar*) const;
};

ostream& operator<<(ostream&, const uint28&);
istream& operator>>(istream&, uint28&);

#endif /* __ID3LIB_UINT28_H__ */
