// $Id: c_wrapper.cpp,v 1.1 2002/01/21 08:16:21 menno Exp $

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
#include "tag.h"
#include "id3.h"

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  // tag wrappers

  ID3_C_EXPORT
  ID3Tag *ID3Tag_New(void)
  {
    ID3_Tag* tag = NULL;
    try
    {
      tag = new ID3_Tag;
    }
    catch (...)
    {
    }
  
    return (ID3Tag *) tag;
  }


  ID3_C_EXPORT
  void ID3Tag_Delete(ID3Tag *tag)
  {
    try
    {
      if (tag)
      {
        delete (ID3_Tag*) tag;
      }
    }
    catch (...)
    {
    }
  }


  ID3_C_EXPORT
  void ID3Tag_Clear(ID3Tag *tag)
  {
    try
    {
      if (tag)
      {
        ((ID3_Tag*) tag)->Clear();
      }
    }
    catch (...)
    {
    }
  }


  ID3_C_EXPORT
  bool ID3Tag_HasChanged(const ID3Tag *tag)
  {
    bool changed = false;
  
    try
    {
      if (tag)
      {
        changed = ((const ID3_Tag * ) tag)->HasChanged();
      }
    }
    catch (...)
    {
    }
    
    return changed;
  }


  ID3_C_EXPORT
  void ID3Tag_SetUnsync(ID3Tag *tag, bool unsync)
  {
    try
    {
      if (tag)
      {
        ((ID3_Tag *) tag)->SetUnsync(unsync);
      }
    }
    catch (...)
    {
    }
  }


  ID3_C_EXPORT
  void ID3Tag_SetExtendedHeader(ID3Tag *tag, bool ext)
  {
    try
    {
      if (tag)
      {
        ((ID3_Tag *) tag)->SetExtendedHeader(ext);
      }
    }
    catch (...)
    {
    }
  }
  
  
  ID3_C_EXPORT 
  void ID3Tag_SetCompression(ID3Tag *tag, bool comp) 
  { 
    try 
    { 
      if (tag) 
      { 
        ((ID3_Tag *) tag)->SetCompression(comp); 
      } 
    } 
    catch (...) 
    { 
    } 
  } 


  ID3_C_EXPORT
  void ID3Tag_SetPadding(ID3Tag *tag, bool pad)
  {
    try
    {
      if (tag)
      {
        ((ID3_Tag *) tag)->SetPadding(pad);
      }
    }
    catch (...)
    {
    }
  }


  ID3_C_EXPORT
  void ID3Tag_AddFrame(ID3Tag *tag, const ID3Frame *frame)
  {
    try
    {
      if (tag)
      {
        ((ID3_Tag *) tag)->AddFrame((const ID3_Frame *) frame);
      }
    }
    catch (...)
    {
    }
  }


  ID3_C_EXPORT
  void ID3Tag_AttachFrame(ID3Tag *tag, ID3Frame *frame)
  {
    try
    {
      if (tag)
      {
        ((ID3_Tag *) tag)->AttachFrame((ID3_Frame *) frame);
      }
    }
    catch (...)
    {
    }
  }


  ID3_C_EXPORT
  void ID3Tag_AddFrames(ID3Tag *tag, const ID3Frame *frames, size_t num)
  {
    try
    {
      if (tag)
      {
        ((ID3_Tag *) tag)->AddFrames((const ID3_Frame *) frames, num);
      }
    }
    catch (...)
    {
    }
  }


  ID3_C_EXPORT
  ID3Frame* ID3Tag_RemoveFrame(ID3Tag *tag, const ID3Frame *frame)
  {
    ID3_Frame* rem_frame = NULL;
    try
    {
      if (tag)
      {
        rem_frame = (((ID3_Tag *) tag)->RemoveFrame((const ID3_Frame *) frame));
      }
    }
    catch (...)
    {
    }
    return (ID3Frame*) rem_frame;
  }


  ID3_C_EXPORT
  ID3_Err ID3Tag_Parse(ID3Tag *tag, const uchar header[ ID3_TAGHEADERSIZE ],
                       const uchar *buffer)
  {
    try
    {
      if (tag)
      {
        ((ID3_Tag *) tag)->Parse(header, buffer);
      }
    }
    catch (ID3_Error &err)
    {
      return err.GetErrorID();
    }
    return ID3E_NoError;
  }


  ID3_C_EXPORT
  size_t ID3Tag_Link(ID3Tag *tag, const char *fileName)
  {
    size_t offset = 0;
  
    try
    {
      if (tag)
      {
        offset = ((ID3_Tag *) tag)->Link(fileName);
      }
    }
    catch (...)
    {
    }
    
    return offset;
  }


  ID3_C_EXPORT
  ID3_Err ID3Tag_Update(ID3Tag *tag)
  {
    try
    {
      if (tag)
      {
        ((ID3_Tag *) tag)->Update();
      }
    }
    catch (ID3_Error &err)
    {
      return err.GetErrorID();
    }

    return ID3E_NoError;
  }

  ID3_C_EXPORT
  ID3_Err ID3Tag_UpdateByTagType(ID3Tag *tag, flags_t tag_type)
  {
    try
    {
      if (tag)
      {
        ((ID3_Tag *) tag)->Update(tag_type);
      }
    }
    catch (ID3_Error &err)
    {
      return err.GetErrorID();
    }

    return ID3E_NoError;
  }


  ID3_C_EXPORT
  ID3_Err ID3Tag_Strip(ID3Tag *tag, flags_t ulTagFlags)
  {
    try
    {
      if (tag)
      {
        ((ID3_Tag *) tag)->Strip(ulTagFlags);
      }
    }
    catch (ID3_Error &err)
    {
      return err.GetErrorID();
    }

    return ID3E_NoError;
  }


  ID3_C_EXPORT
  ID3Frame *ID3Tag_FindFrameWithID(const ID3Tag *tag, ID3_FrameID id)
  {
    ID3_Frame *frame = NULL;
  
    try
    {
      if (tag)
      {
        frame = ((const ID3_Tag *) tag)->Find(id);
      }
    }
    catch (...)
    {
    }

    return (ID3Frame *) frame;
  }


  ID3_C_EXPORT
  ID3Frame *ID3Tag_FindFrameWithINT(const ID3Tag *tag, ID3_FrameID id, 
                                    ID3_FieldID fld, uint32 data)
  {
    ID3_Frame *frame = NULL;
  
    try
    {
      if (tag)
      {
        frame = ((const ID3_Tag *) tag)->Find(id, fld, data);
      }
    }
    catch (...)
    {
    }
    
    return (ID3Frame *) frame;
  }


  ID3_C_EXPORT
  ID3Frame *ID3Tag_FindFrameWithASCII(const ID3Tag *tag, ID3_FrameID id, 
                                      ID3_FieldID fld, const char *data)
  {
    ID3_Frame *frame = NULL;
  
    try
    {
      if (tag)
      {
        frame = ((const ID3_Tag *) tag)->Find(id, fld, data);
      }
    }
    catch (...)
    {
    }
    
    return (ID3Frame *) frame;
  }


  ID3_C_EXPORT
  ID3Frame *ID3Tag_FindFrameWithUNICODE(const ID3Tag *tag, ID3_FrameID id, 
                                        ID3_FieldID fld, const unicode_t *data)
  {
    ID3_Frame *frame = NULL;
  
    try
    {
      if (tag)
      {
        frame = ((const ID3_Tag *) tag)->Find(id, fld, data);
      }
    }
    catch (...)
    {
    }
    
    return (ID3Frame *) frame;
  }


  ID3_C_EXPORT
  size_t ID3Tag_NumFrames(const ID3Tag *tag)
  {
    size_t num = 0;
  
    try
    {
      if (tag)
      {
        num = ((const ID3_Tag *) tag)->NumFrames();
      }
    }
    catch (...)
    {
    }
    
    return num;
  }


  ID3_C_EXPORT
  ID3Frame *ID3Tag_GetFrameNum(const ID3Tag *tag, index_t num)
  {
    ID3_Frame *frame = NULL;
  
    try
    {
      if (tag)
      {
        frame = ((const ID3_Tag *) tag)->GetFrameNum(num);
      }
    }
    catch (...)
    {
    }
    
    return (ID3Frame *) frame;
  }


  // frame wrappers

  ID3_C_EXPORT
  ID3Frame *ID3Frame_New(void)
  {
    ID3_Frame* frame = NULL;
    try
    {
      frame = new ID3_Frame;
    }
    catch (...)
    {
    }
  
    return (ID3Frame *) frame;
  }

  ID3_C_EXPORT
  ID3Frame *ID3Frame_NewID(ID3_FrameID id)
  {
    ID3_Frame* frame = NULL;
    try
    {
      frame = new ID3_Frame(id);
    }
    catch (...)
    {
    }
  
    return (ID3Frame *) frame;
  }

  ID3_C_EXPORT
  void ID3Frame_Delete(ID3Frame *frame)
  {
    try
    {
      if (frame)
      {
        delete (ID3_Frame *) frame;
      }
    }
    catch (...)
    {
    }
  }


  ID3_C_EXPORT
  void ID3Frame_Clear(ID3Frame *frame)
  {
    try
    {
      if (frame)
      {
        ((ID3_Frame *) frame)->Clear();
      }
    }
    catch (...)
    {
    }
  }


  ID3_C_EXPORT
  void ID3Frame_SetID(ID3Frame *frame, ID3_FrameID id)
  {
    try
    {
      if (frame)
      {
        ((ID3_Frame *) frame)->SetID(id);
      }
    }
    catch (...)
    {
    }
  }


  ID3_C_EXPORT
  ID3_FrameID ID3Frame_GetID(const ID3Frame *frame)
  {
    ID3_FrameID id = ID3FID_NOFRAME;
  
    try
    {
      if (frame)
      {
        id = ((const ID3_Frame *) frame)->GetID();
      }
    }
    catch (...)
    {
    }

    return id;
  }


  ID3_C_EXPORT
  ID3Field *ID3Frame_GetField(const ID3Frame *frame, ID3_FieldID name)
  {
    ID3_Field *field = NULL;
  
    try
    {
      if (frame)
      {
        field = &( ((const ID3_Frame *) frame)->Field(name));
      }
    }
    catch (...)
    {
    }
    
    return (ID3Field *) field;
  }


  ID3_C_EXPORT
  void ID3Frame_SetCompression(ID3Frame *frame, bool comp)
  {
    try
    {
      if (frame)
      {
        ((ID3_Frame *) frame)->SetCompression(comp);
      }
    }
    catch (...)
    {
    }
  }


  ID3_C_EXPORT
  bool ID3Frame_GetCompression(const ID3Frame *frame)
  {
    try
    {
      if (frame)
      {
        return ((const ID3_Frame *) frame)->GetCompression();
      }
    }
    catch (...)
    {
    }
    return false;
  }


  // field wrappers


  ID3_C_EXPORT
  void ID3Field_Clear(ID3Field *field)
  {
    try
    {
      if (field)
      {
        ((ID3_Field *) field)->Clear();
      }
    }
    catch (...)
    {
    }
  }


  ID3_C_EXPORT
  size_t ID3Field_Size(const ID3Field *field)
  {
    size_t size = 0;
  
    try
    {
      if (field)
      {
        size = ((const ID3_Field *) field)->Size();
      }
    }
    catch (...)
    {
    }
    
    return size;
  }


  ID3_C_EXPORT
  size_t ID3Field_GetNumTextItems(const ID3Field *field)
  {
    size_t items = 0;
  
    try
    {
      if (field)
      {
        items = ((const ID3_Field *) field)->GetNumTextItems();
      }
    }
    catch (...)
    {
    }
    
    return items;
  }


  ID3_C_EXPORT
  void ID3Field_SetINT(ID3Field *field, uint32 data)
  {
    try
    {
      if (field)
      {
        ((ID3_Field *) field)->Set(data);
      }
    }
    catch (...)
    {
    }
  }


  ID3_C_EXPORT
  uint32 ID3Field_GetINT(const ID3Field *field)
  {
    uint32 value = 0;
  
    try
    {
      if (field)
      {
        value = ((const ID3_Field *) field)->Get();
      }
    }
    catch (...)
    {
    }
    
    return value;
  }


  ID3_C_EXPORT
  void ID3Field_SetUNICODE(ID3Field *field, const unicode_t *string)
  {
    try
    {
      if (field)
      {
        ((ID3_Field *) field)->Set(string);
      }
    }
    catch (...)
    {
    }
  }


  ID3_C_EXPORT
  size_t ID3Field_GetUNICODE(const ID3Field *field, unicode_t *buffer, 
                             size_t maxChars, index_t itemNum)
  {
    size_t numChars = 0;
  
    try
    {
      if (field)
      {
        numChars = ((const ID3_Field *) field)->Get(buffer, maxChars, itemNum);
      }
    }
    catch (...)
    {
    }
    
    return numChars;
  }


  ID3_C_EXPORT
  void ID3Field_AddUNICODE(ID3Field *field, const unicode_t *string)
  {
    try
    {
      if (field)
      {
        ((ID3_Field *) field)->Add(string);
      }
    }
    catch (...)
    {
    }
  }


  ID3_C_EXPORT
  void ID3Field_SetASCII(ID3Field *field, const char *string)
  {
    try
    {
      if (field)
      {
        ((ID3_Field *) field)->Set(string);
      }
    }
    catch (...)
    {
    }
  }


  ID3_C_EXPORT
  size_t ID3Field_GetASCII(const ID3Field *field, char *buffer, 
                           size_t maxChars, index_t itemNum)
  {
    size_t numChars = 0;
  
    try
    {
      if (field)
      {
        numChars = ((const ID3_Field *) field)->Get(buffer, maxChars, itemNum);
      }
    }
    catch (...)
    {
    }
    
    return numChars;
  }


  ID3_C_EXPORT
  void ID3Field_AddASCII(ID3Field *field, const char *string)
  {
    try
    {
      if (field)
      {
        ((ID3_Field *) field)->Add(string);
      }
    }
    catch (...)
    {
    }
  }


  ID3_C_EXPORT
  void ID3Field_SetBINARY(ID3Field *field, const uchar *data, size_t size)
  {
    try
    {
      if (field)
      {
        ((ID3_Field *) field)->Set(data, size);
      }
    }
    catch (...)
    {
    }
  }


  ID3_C_EXPORT
  void ID3Field_GetBINARY(const ID3Field *field, uchar *buffer, size_t buffLength)
  {
    try
    {
      if (field)
      {
        ((const ID3_Field *) field)->Get(buffer, buffLength);
      }
    }
    catch (...)
    {
    }
  }


  ID3_C_EXPORT
  void ID3Field_FromFile(ID3Field *field, const char *fileName)
  {
    try
    {
      if (field)
      {
        ((ID3_Field *) field)->FromFile(fileName);
      }
    }
    catch (...)
    {
    }
  }


  ID3_C_EXPORT
  void ID3Field_ToFile(const ID3Field *field, const char *fileName)
  {
    try
    {
      if (field)
      {
        ((const ID3_Field *) field)->ToFile(fileName);
      }
    }
    catch (...)
    {
    }
  }

#ifdef __cplusplus
}
#endif /* __cplusplus */
