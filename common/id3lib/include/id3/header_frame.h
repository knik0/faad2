// $Id: header_frame.h,v 1.1 2002/01/21 08:16:21 menno Exp $

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

#ifndef __ID3LIB_HEADER_FRAME_H__
#define __ID3LIB_HEADER_FRAME_H__

#include "header.h"
#include "field.h"

class ID3_FrameHeader : public ID3_Header
{
public:

  enum
  {
    TAGALTER    = 1 << 15,
    FILEALTER   = 1 << 14,
    READONLY    = 1 << 13,
    COMPRESSION = 1 <<  7,
    ENCRYPTION  = 1 <<  6,
    GROUPING    = 1 <<  5
  };

  ID3_FrameHeader() : __frame_def(NULL), __dyn_frame_def(false) { ; }
  virtual ~ID3_FrameHeader() { this->Clear(); }
  
  /* */ size_t        Size() const;
  /* */ size_t        Parse(const uchar*, size_t);
  /* */ size_t        Render(uchar*) const;
  /* */ bool          SetFrameID(ID3_FrameID id);
  /* */ ID3_FrameID   GetFrameID() const;
  const char*         GetTextID() const;
  const ID3_FrameDef* GetFrameDef() const;
  /* */ bool          Clear();
  ID3_FrameHeader&    operator=(const ID3_FrameHeader&);

  bool SetCompression(bool b) { return this->SetFlags(COMPRESSION, b); }
  bool SetEncryption(bool b)  { return this->SetFlags(ENCRYPTION, b); }
  bool SetGrouping(bool b)    { return this->SetFlags(GROUPING, b); }

  bool GetCompression() const { return __flags.test(COMPRESSION); }
  bool GetEncryption() const  { return __flags.test(ENCRYPTION); }
  bool GetGrouping() const    { return __flags.test(GROUPING); }
  bool GetReadOnly() const    { return __flags.test(READONLY); }

protected:
  bool                SetFlags(uint16 f, bool b)
  {
    bool changed = __flags.set(f, b);
    __changed = __changed || changed;
    return changed;
  }
  void                SetUnknownFrame(const char*);

private:
  ID3_FrameDef*       __frame_def;
  bool                __dyn_frame_def;
}
;

#endif /* __ID3LIB_HEADER_FRAME__ */
