// $Id: utils.h,v 1.1 2002/01/21 08:16:21 menno Exp $

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

#ifndef __ID3LIB_UTILS_H__
#define __ID3LIB_UTILS_H__

#include "globals.h"

#if defined ID3_UNDEFINED
namespace id3
{
#endif /* ID3_UNDEFINED */
  uint32 ParseNumber(const uchar *buffer, size_t size = sizeof(uint32));
  size_t RenderNumber(uchar *buffer, uint32 val, size_t size = sizeof(uint32));
  
  void   mbstoucs(unicode_t *unicode, const char *ascii, const size_t len);
  void   ucstombs(char *ascii, const unicode_t *unicode, const size_t len);
  size_t ucslen(const unicode_t *unicode);
  void   ucscpy(unicode_t *dest, const unicode_t *src);
  void   ucsncpy(unicode_t *dest, const unicode_t *src, size_t len);
  int    ucscmp(const unicode_t *s1, const unicode_t *s2);
  int    ucsncmp(const unicode_t *s1, const unicode_t *s2, size_t len);

  // these can be utility functions
  size_t     ID3_GetUnSyncSize(uchar *, size_t);
  void       ID3_UnSync(uchar *, size_t, const uchar *, size_t);
  size_t     ID3_ReSync(uchar *, size_t);

#if defined ID3_UNDEFINED
}
#endif  /* ID3_UNDEFINED */
  
  
#endif /* __ID3LIB_UTILS_H__ */
