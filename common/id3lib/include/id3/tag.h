// $Id: tag.h,v 1.1 2002/01/21 08:16:21 menno Exp $

// id3lib: a software library for creating and manipulating id3v1/v2 tags
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

#ifndef __ID3LIB_TAG_H__
#define __ID3LIB_TAG_H__

#include <stdio.h>
#include "header_tag.h"
#include "frame.h"
#include "spec.h"

struct ID3_Elem
{
  virtual ~ID3_Elem() { if (pFrame) { delete pFrame; } }
  ID3_Elem  *pNext;
  ID3_Frame *pFrame;
};

/** String used for the description field of a comment tag converted from an
 ** id3v1 tag to an id3v2 tag
 **
 ** \sa #ID3V1_Tag
 **/
//const char STR_V1_COMMENT_DESC[] = "ID3v1_Comment";
const char STR_V1_COMMENT_DESC[] = "";

class ID3_Tag : public ID3_Speccable
{
public:
  ID3_Tag(const char *name = NULL);
  ID3_Tag(const ID3_Tag &tag);
  virtual ~ID3_Tag();
  
  void       Clear();
  bool       HasChanged() const;
  size_t     Size() const;

  bool       SetUnsync(bool bSync);
  bool       SetExtendedHeader(bool bExt);
  bool       SetPadding(bool bPad);

  void       AddFrame(const ID3_Frame&);
  void       AddFrame(const ID3_Frame*);
  void       AttachFrame(ID3_Frame*);
  ID3_Frame* RemoveFrame(const ID3_Frame *);

  size_t     Parse(const uchar*, size_t);
  size_t     Parse(const uchar header[ID3_TAGHEADERSIZE], const uchar *buffer);
  size_t     Render(uchar*) const;
  size_t     Render(uchar*, ID3_TagType) const;
  size_t     RenderV1(uchar*) const;

  size_t     Link(const char *fileInfo, flags_t = (flags_t) ID3TT_ALL);
  flags_t    Update(flags_t = (flags_t) ID3TT_ID3V2);
  flags_t    Strip(flags_t = (flags_t) ID3TT_ALL);

  /// Finds frame with given frame id
  ID3_Frame* Find(ID3_FrameID id) const;

  /// Finds frame with given frame id, fld id, and integer data
  ID3_Frame* Find(ID3_FrameID id, ID3_FieldID fld, uint32 data) const;

  /// Finds frame with given frame id, fld id, and ascii data
  ID3_Frame* Find(ID3_FrameID id, ID3_FieldID fld, const char *) const;

  /// Finds frame with given frame id, fld id, and unicode data
  ID3_Frame* Find(ID3_FrameID id, ID3_FieldID fld, const unicode_t *) const;

  /** Returns the number of frames present in the tag object.
   ** 
   ** This includes only those frames that id3lib recognises.  This is used as
   ** the upper bound on calls to the GetFrame() and operator[]() methods.
   ** 
   ** \return The number of frames present in the tag object.
   **/
  size_t     NumFrames() const { return __num_frames; }
  ID3_Frame* GetFrameNum(index_t) const;
  ID3_Frame* operator[](index_t) const;
  ID3_Tag&   operator=( const ID3_Tag & );

  bool       GetUnsync() const { return __hdr.GetUnsync(); }

  bool       HasTagType(uint16 tt) const { return __file_tags.test(tt); }
  ID3_V2Spec GetSpec() const;

  static size_t IsV2Tag(const uchar*);

  /* Deprecated! */
  void       AddNewFrame(ID3_Frame* f) { this->AttachFrame(f); }
  size_t     Link(const char *fileInfo, bool parseID3v1, bool parseLyrics3);
  void       SetCompression(bool) { ; }
  void       AddFrames(const ID3_Frame *, size_t);
  bool       HasLyrics() const { return this->HasTagType(ID3TT_LYRICS); }
  bool       HasV2Tag()  const { return this->HasTagType(ID3TT_ID3V2); }
  bool       HasV1Tag()  const { return this->HasTagType(ID3TT_ID3V1); }

protected:
  bool       SetSpec(ID3_V2Spec);

  ID3_Elem*  Find(const ID3_Frame *) const;
  size_t     PaddingSize(size_t) const;

  void       RenderExtHeader(uchar *);
  ID3_Err    OpenFileForWriting();
  ID3_Err    OpenFileForReading();
  ID3_Err    CreateFile();
  bool       CloseFile();

  void       RenderV1ToHandle();
  void       RenderV2ToHandle();
  size_t     ParseFromHandle();
  void       ParseID3v1();
  void       ParseLyrics3();

private:
  ID3_TagHeader __hdr;          // information relevant to the tag header
  bool       __is_padded;       // add padding to tags?

  ID3_Elem*  __frames;          // frames attached to the tag
  size_t     __num_frames;      // the current number of frames

  mutable ID3_Elem*  __cursor;  // which frame in list are we at
  mutable bool       __changed; // has tag changed since last parse or render?

  // file-related member variables
  char*      __file_name;       // name of the file we are linked to
  FILE*      __file_handle;     // a handle to the file we are linked to
  size_t     __file_size;       // the size of the file (without any tag(s))
  size_t     __starting_bytes;  // number of tag bytes at start of file
  size_t     __ending_bytes;    // number of tag bytes at end of file
  bool       __is_file_writable;// is the associated file (via Link) writable?
  ID3_Flags  __tags_to_parse;   // which tag types should attempt to be parsed
  ID3_Flags  __file_tags;       // which tag types does the file contain
};

//@{
/// Copies
ID3_Tag& operator<<(ID3_Tag&, const ID3_Frame &);
/// Attaches a pointer to a frame
ID3_Tag& operator<<(ID3_Tag&, const ID3_Frame *);
//@}

// deprecated!
int32 ID3_IsTagHeader(const uchar header[ID3_TAGHEADERSIZE]);

#endif /* __ID3LIB_TAG_H__ */
