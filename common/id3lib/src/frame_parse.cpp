// $Id: frame_parse.cpp,v 1.1 2002/01/21 08:16:22 menno Exp $

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

#include <zlib.h>
#include "frame.h"
#include "utils.h"

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

size_t ID3_Frame::Parse(const uchar * const buffer, size_t size) 
{ 
  __bad_parse = false;  
  const size_t hdr_size = __hdr.Parse(buffer, size);  

  if (!hdr_size)  
  {  
    return 0;  
  }  
  
  // data is the part of the frame buffer that appears after the header  
  const uchar* data = &buffer[hdr_size]; 
  const size_t data_size = __hdr.GetDataSize();
  size_t extras = 0;
  // how many bytes remain to be parsed 
  size_t remainder = data_size - MIN(extras, data_size);
  
  unsigned long expanded_size = 0;
  if (__hdr.GetCompression())
  {
    expanded_size = ParseNumber(&data[extras]);
    extras += sizeof(uint32);
  }

  if (__hdr.GetEncryption())
  {
    this->_SetEncryptionID(data[extras++]);
  }

  if (__hdr.GetGrouping())
  {
    this->_SetGroupingID(data[extras++]);
  }

  data += extras;

  // expand out the data if it's compressed 
  uchar* expanded_data = NULL;
  if (__hdr.GetCompression())
  {  
    expanded_data = new uchar[expanded_size];  
        
    uncompress(expanded_data, &expanded_size, data, remainder);  
    data = expanded_data; 
    remainder = expanded_size; 
  }
  
  // set the type of frame based on the parsed header  
  this->_ClearFields(); 
  this->_InitFields(); 
  try
  {  
    ID3_TextEnc enc = ID3TE_ASCII;  // set the default encoding 
    ID3_V2Spec spec = this->GetSpec(); 
    // parse the frame's fields  
    for (ID3_Field** fi = __fields; fi != __fields + __num_fields; fi++)
    {
      if (!*fi)
      {
        // Ack!  Why is the field NULL?  Log this...
        continue;
      }
      if (remainder == 0) 
      { 
        // there's no remaining data to parse! 
        __bad_parse = true; 
        break; 
      } 
 
      if (!(*fi)->InScope(spec)) 
      { 
        // this field isn't in scope, so don't attempt to parse into it 
        // rather, give it some reasonable default value in case someone tries
        // to access it
        switch ((*fi)->GetType())
        {
          case ID3FTY_INTEGER:
            **fi = (uint32) 0;
            break;
          default:
            **fi = "";
            break;
        }
        // now continue with the rest of the fields
        continue; 
      }
      
      (*fi)->SetEncoding(enc);
      size_t frame_size = (*fi)->Parse(data, remainder); 
      
      if (0 == frame_size) 
      { 
        // nothing to parse!  ack!  parse error... 
        __bad_parse = true; 
        break; 
      } 
 
      if ((*fi)->GetID() == ID3FN_TEXTENC)  
      {
        enc = static_cast<ID3_TextEnc>((*fi)->Get());  
      }  
      
      data += frame_size; 
      remainder -= MIN(frame_size, remainder); 
    }  
  }  
  catch (...)  
  {  
    // TODO: log this!
    //cerr << "*** parsing error!" << endl;  
    // There's been an error in the parsing of the frame.  
    __bad_parse = true;  
  }  
      
  __changed = false;

  delete [] expanded_data;
 
  return MIN(hdr_size + data_size, size);  
} 
