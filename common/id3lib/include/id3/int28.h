// $Id: int28.h,v 1.1 2002/01/21 08:16:21 menno Exp $

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

#ifndef __ID3LIB_INT28_H__
#define __ID3LIB_INT28_H__

#include <iostream.h>
#include "globals.h"

class int28
{
  friend ostream &operator<<(ostream& out, int28& val);
  friend istream &operator>>(istream& in, int28& val);
public:
  int28(uint32 val = 0);
  int28(uchar val[sizeof(uint32)]);
  
  uchar operator[](size_t posn);
  uint32 get(void);
  
protected:
  void set(uint32 val);
  void set(uchar val[sizeof(uint32)]);

private:
  uchar __acValue[sizeof(uint32)]; // the integer stored as a uchar array
  uint32 __nValue;
}
;

#endif /* __ID3LIB_INT28_H__ */
