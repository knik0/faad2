// $Id: field_string_unicode.cpp,v 1.1 2002/01/21 08:16:22 menno Exp $

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
#include <stdlib.h>
#include "field.h"
#include "utils.h"

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

// this function is another way of using Set()

ID3_Field& ID3_Field::operator= (const unicode_t *string)
{
  Set(string);
  
  return *this;
}


// this is Set()

void ID3_Field::Set(const unicode_t *string)
{
  size_t nBytes = (0 == __length) ? ucslen(string) : __length;
  
  // we can simply increment the nBytes count here because we just pilfer
  // the NULL which is present in the string which was passed to us
  if (__flags & ID3FF_CSTR)
  {
    nBytes++;
  }
    
  // doubling the nBytes because Unicode is twice the size of ASCII
  nBytes *= sizeof(unicode_t);
  
  Set((uchar *) string, nBytes);
  
  this->SetEncoding(ID3TE_UNICODE);
  __type = ID3FTY_TEXTSTRING;
  __changed = true;
  
  return ;
}


void ID3_Field::Add(const unicode_t *string)
{
  if (NULL == __data)
  {
    Set(string);
  }
  else
  {
    unicode_t *uBuffer = (unicode_t *) __data;

    // +1 is for the NULL at the end and the other +1 is for the list divider
    size_t newLen = ucslen(string) + ucslen(uBuffer) + 1 + 1;
    
    unicode_t *temp = new unicode_t[newLen];
    if (NULL == temp)
    {
      ID3_THROW(ID3E_NoMemory);
    }

    ucscpy(temp, uBuffer);

    // I use the value 1 as a divider because then I can change it to either a
    // '/' or a NULL at render time.  This allows easy use of these functions
    // for text lists or in the IPLS frame
    temp[ucslen(uBuffer)] = L'\001';
    ucscpy(&temp[ucslen(uBuffer) + 1], string);
    temp[newLen - 1] = NULL_UNICODE;
      
    Set(temp);
      
    delete [] temp;
  }
  
  return ;
}


// this is Get()

size_t ID3_Field::Get(unicode_t *buffer, size_t maxChars, index_t itemNum) const
{
  size_t charsUsed = 0;
  
  // check to see if there is a string in the frame to copy before we even try
  if (NULL != __data)
  {
    lsint nullOffset = 0;
    
    if (__flags & ID3FF_CSTR)
    {
      nullOffset = -1;
    }
      
    // first we must find which element is being sought to make sure it exists
    // before we try to get it
    if (itemNum <= GetNumTextItems() && itemNum > 0)
    {
      unicode_t *source = (unicode_t *) __data;
      size_t posn = 0;
      size_t sourceLen = 0;
      index_t curItemNum = 1;
      
      // now we find that element and set the souvre pointer
      while (curItemNum < itemNum)
      {
        while (*source != L'\001' && *source != L'\0' && posn <
               ((__size / sizeof(unicode_t)) + nullOffset))
        {
          source++, posn++;
        }
          
        source++;
        curItemNum++;
      }
      
      // now that we are positioned at the first character of the string we
      // want, find the end of it
      while (source[sourceLen] != L'\001' && source[sourceLen] != L'\0' &&
             posn <((__size / sizeof(unicode_t) + nullOffset)))
      {
        sourceLen++, posn++;
      }
        
      if (NULL == buffer)
      {
        ID3_THROW(ID3E_NoBuffer);
      }

      size_t actualChars = MIN(maxChars, sourceLen);
        
      ucsncpy(buffer, source, actualChars);
      if (actualChars < maxChars)
      {
        buffer[actualChars] = L'\0';
      }
      charsUsed = actualChars;
    }
  }
  
  return charsUsed;
}


size_t ID3_Field::GetNumTextItems() const
{
  size_t numItems = 0;
  
  if (NULL != __data)
  {
    index_t posn = 0;
    
    numItems++;
    
    while (posn < __size)
    {
      if (__data[posn++] == L'\001')
      {
        numItems++;
      }
    }
  }
  
  return numItems;
}


size_t 
ID3_Field::ParseUnicodeString(const uchar *buffer, size_t nSize)
{
  size_t nBytes = 0;
  unicode_t *temp = NULL;
  if (__length > 0)
  {
    nBytes = __length;
  }
  else
  {
    if (__flags & ID3FF_CSTR)
    {
      while (nBytes < nSize &&
             !(buffer[nBytes] == 0 && buffer[nBytes + 1] == 0))
      {
        nBytes += sizeof(unicode_t);
      }
    }
    else
    {
      nBytes = nSize;
    }
  }
  
  if (nBytes > 0)
  {
    // Sanity check our indices and sizes before we start copying memory
    if (nBytes > nSize)
    {
      ID3_THROW_DESC(ID3E_BadData, "field information invalid");
    }

    temp = new unicode_t[(nBytes / sizeof(unicode_t)) + 1];
    if (NULL == temp)
    {
      ID3_THROW(ID3E_NoMemory);
    }

    size_t loc = 0;

    memcpy(temp, buffer, nBytes);
    temp[nBytes / sizeof(unicode_t)] = NULL_UNICODE;
      
    // if there is a BOM, skip past it and check to see if we need to swap
    // the byte order around
    if (temp[0] == 0xFEFF || temp[0] == 0xFFFE)
    {
      loc++;
        
      // if we need to swap the byte order
      /* TODO: Determine if this the correct check to make sure bytes should
         be swapped.  For example, the example tag 230-unicode.tag (found in 
         the distrubitution) has two unicode sections, each that begin with
         the FEFF magic number.  Each unicode character is, as usual, two
         bytes.  The first byte is the ascii equivalent; the second is null.
         Is this the "correct" encoding?  When a little-endian parses each of
         those characters, the bytes are swapped, so they essentially end up
         as the ascii equivalent automatically.  The FEFF magic number is also
         swapped, so the number is evaluated as FFFE.  The original code below
         forced byteswapping if the value of the first unicode character was
         not equal to 0xFEFF.  This doesn't work for a little-endian machine,
         though, since, as the rest of the code now stands, swapping the bytes
         will not create a correct parse.  Therefore, the code swaps bytes
         only when the value is equal to FEFF.
      */
      if (temp[0] == 0xFEFF)
      {
        for (index_t i = loc; i < ucslen(temp); i++)
        {
          uchar
            u1 = ((uchar *)(&temp[i]))[0],
            u2 = ((uchar *)(&temp[i]))[1];
          temp[i] = (u1 << 8) | u2;
        }
      }
    }
      
    Set(&temp[loc]);
      
    delete [] temp;
  }
  
  if (__flags & ID3FF_CSTR)
  {
    nBytes += sizeof(unicode_t);
  }
    
  __changed = false;
  
  return nBytes;
}


size_t ID3_Field::RenderUnicodeString(uchar *buffer) const
{
  size_t nBytes = 0;
  
  nBytes = BinSize();
  
  if (NULL != __data && __size && nBytes)
  {
    // we render at sizeof(unicode_t) bytes into the buffer because we make
    // room for the Unicode BOM
    memcpy(&buffer[sizeof(unicode_t)], (uchar *) __data, 
           nBytes - sizeof(unicode_t));
    
    unicode_t *ourString = (unicode_t *) &buffer[sizeof(unicode_t)];
    // now we convert the internal dividers to what they are supposed to be
    for (index_t i = sizeof(unicode_t); i < this->Size(); i++)
    {
      if (ourString[i] == 0x01)
      {
        unicode_t sub = L'/';
        
        if (__flags & ID3FF_LIST)
        {
          sub = L'\0';
        }
        
        ourString[i] = sub;
      }
    }
  }
  
  if (nBytes)
  {
    // render the BOM
    unicode_t *BOM = (unicode_t *) buffer;
    BOM[0] = 0xFFFE;
  }
  
  if (nBytes == sizeof(unicode_t) && (__flags & ID3FF_CSTR))
  {
    for (size_t i = 0; i < sizeof(unicode_t); i++)
    {
      buffer[i] = 0;
    }
  }
    
  __changed = false;
  
  return nBytes;
}
