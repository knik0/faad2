// $Id: tag_parse_lyrics3.cpp,v 1.1 2002/01/21 08:16:22 menno Exp $

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "tag.h"
#include "utils.h"
#include "misc_support.h"

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

luint ID3_CRLFtoLF(char *buffer, luint size)
{
  luint newSize = 0;
  char *dest = buffer;
  char *source = buffer;
  
  if (NULL == buffer || size == 0)
  {
    // TODO: log this
    return 0;
    // ID3_THROW(ID3E_NoData);
  }

  while (source < (buffer + size))
  {
    if (*source == 0x0D)
    {
      source++;
    }
    else
    {
      *dest++ = *source++;
    }
  }
    
  newSize = dest - buffer;
    
  return newSize;
}


luint ID3_StripTimeStamps(char *buffer, luint size)
{
  luint newSize = 0;
  char *dest = buffer;
  char *source = buffer;
  
  if ((buffer == NULL) || (size == 0))
  {
    // TODO: log this
    return 0;
    // ID3_THROW(ID3E_NoData);
  }

  while (source < (buffer + size))
  {
    if (*source == '[')
    {
      source += 7;
    }
    else
    {
      *dest++ = *source++;
    }
  }
    
  newSize = dest - buffer;
    
  return newSize;
}

luint ID3_RenderTimeStamp(uchar* buffer, luint ms, bool lf)
{
  uchar* dest = buffer;

  // put synch identifier
  *dest++ = '\0';
  
  // put timestamp
  dest += RenderNumber(dest, ms, sizeof(uint32));
  if (lf)
  {
    // put the LF
    *dest++ = 0x0A;
  }
  
  return dest - buffer;
}

luint ID3_Lyrics3ToSylt(uchar *buffer, luint size)
{
  if ((buffer == NULL) || (size == 0))
  {
    // TODO: log this
    return 0;
    //ID3_THROW(ID3E_NoData);
  }

  uchar *dest = buffer;
  uchar *source = buffer;
  luint ms;
  bool lf = false, first = true;
  
  while (source < (buffer + size))
  {
    if (0x0A == *source)
    {
      lf = true;
      source++;
    }
    else if ('[' != *source)
    {
      *dest++ = *source++;
    }
    else
    {
      // check if first timestamp
      if (first)
      {
        first = false;
      }
      else
      {
        dest += ID3_RenderTimeStamp(dest, ms, lf);
      }
      
      // timestamp found skip [
      source++;
      
      // get minutes and ms
      size_t minutes = strtol((char*)source, NULL, 10);
      
      // skip :
      source += 3;
      
      size_t seconds = strtol((char*)source, NULL, 10);
      
      // skip ]
      source += 3;
      
      // get seconds and ms
      ms = ((60 * minutes) + seconds) * 1000;
    }
  }

  dest += ID3_RenderTimeStamp(dest, ms, lf);
  
  return dest - buffer;
}

void ID3_Tag::ParseLyrics3()
{
  if (NULL == __file_handle)
  {
    // TODO: log this
    return;
    // ID3_THROW(ID3E_NoData);
  }

  uchar buffer[18];

  fseek(__file_handle, -143, SEEK_END);
  fread(buffer, 1, 18, __file_handle);

  // first check for an ID3v1 tag
  if (memcmp(&buffer[15], "TAG", 3) == 0)
  {
    // check for lyrics
    if (memcmp(&buffer[6], "LYRICSEND", 9) == 0)
    {
      // we have a Lyrics3 v1.00 tag

      // get the position of LYRICSEND string in file
      int filelen = __file_size;
      int lyrendpos = filelen - 137;

      // read the maximum Lyrics3 v1.00 tag size (5100 bytes) + some extra byte
      int bytesToRead = 5100 + 100;
      fseek(__file_handle, -(bytesToRead+143), SEEK_END);
      uchar *bufflyr;
      bufflyr = new uchar[bytesToRead];
      if (NULL == bufflyr)
      {
        ID3_THROW(ID3E_NoMemory);
      }
      fread(bufflyr, 1, bytesToRead, __file_handle);

      // search for LYRICSBEGIN
      bool      bFoundBegin = false;
      int pos = 0;
      for (; pos < bytesToRead; pos++)
      {
        if (bufflyr[pos] == 'L')
        {
          // found ?
          if (memcmp(&bufflyr[pos], "LYRICSBEGIN", 11) == 0)
          {
            // yes
            bFoundBegin = true;
            break;
          }
          // still not found
        }
      }

      if (!bFoundBegin)
      {
                                // invalid tag
        delete[] bufflyr;
        return;
      }

      delete[] bufflyr;

      // extract lyrics text
      pos += 11;
      int lyrbeginpos = filelen - ((bytesToRead+143) - pos);
      int lyrsize = lyrendpos - lyrbeginpos;

      fseek(__file_handle, lyrbeginpos, SEEK_SET);
      bufflyr = new uchar[lyrsize];
      if (NULL == bufflyr)
      {
        ID3_THROW(ID3E_NoMemory);
      }
      fread(bufflyr, 1, lyrsize, __file_handle);

      char *text;
      luint newSize;

      newSize = ID3_CRLFtoLF((char *) bufflyr, lyrsize);

      text = new char[newSize + 1];
      if (NULL == text)
      {
        ID3_THROW(ID3E_NoMemory);
      }

      text[newSize] = 0;

      memcpy(text, bufflyr, newSize);
      delete[] bufflyr;

      ID3_Frame *pLyrFrame = ID3_AddLyrics(this, text);
      if (NULL != pLyrFrame)
      {
        pLyrFrame->Field(ID3FN_LANGUAGE) = "eng";
        pLyrFrame->Field(ID3FN_DESCRIPTION) = "Converted from Lyrics3 v1.00";
      }

      delete[] text;
    }
  
    else if (memcmp(&buffer[6], "LYRICS200", 9) == 0)
    {
      // we have a Lyrics3 v2.00 tag
      luint lyricsSize;

      ID3_Frame *pLyrFrame = NULL;
      char *textInf = NULL;

      buffer[6] = 0;
      lyricsSize = atoi((char *) buffer);

      fseek(__file_handle, -18 - lyricsSize, SEEK_CUR);
      fread(buffer, 1, 11, __file_handle);

      if (memcmp(buffer, "LYRICSBEGIN", 11) == 0)
      {
        luint bytesToRead = lyricsSize - 11;
        uchar *buff2;

        __ending_bytes += lyricsSize + 9 + 6;

        buff2 = new uchar[bytesToRead];
        if (NULL == buff2)
        {
          ID3_THROW(ID3E_NoMemory);
        }

        luint posn = 0;
        bool stampsUsed = false;

        fread(buff2, 1, bytesToRead, __file_handle);

        while (posn < bytesToRead)
        {
          uchar fid[4];
          uchar sizeT[6];
          luint size;

          fid[3] = 0;
          sizeT[5] = 0;

          memcpy(fid, &buff2[posn], 3);
          memcpy(sizeT, &buff2[posn + 3], 5);
          size = atoi((char *) sizeT);

          // the IND field
          if (strcmp((char *) fid, "IND") == 0)
          {
            if (buff2[posn + 8 + 1] == '1')
            {
              stampsUsed = true;
            }
          }

          // the TITLE field
          if (strcmp((char *) fid, "ETT") == 0)
          {
            char *text;

            text = new char[size + 1];
            if (NULL == text)
            {
              ID3_THROW(ID3E_NoMemory);
            }

            text[size] = '\0';
            memcpy(text, &buff2[posn + 8], size);

            ID3_AddTitle(this, text);

            delete[] text;
          }

          // the ARTIST field
          if (strcmp((char *) fid, "EAR") == 0)
          {
            char *text;

            text = new char[size + 1];
            if (NULL == text)
            {
              ID3_THROW(ID3E_NoMemory);
            }

            text[size] = 0;
            memcpy(text, &buff2[posn + 8], size);

            ID3_AddArtist(this, text);

            delete[] text;
          }

          // the ALBUM field
          if (strcmp((char *) fid, "EAL") == 0)
          {
            char *text;

            text = new char[size + 1];
            if (NULL == text)
            {
              ID3_THROW(ID3E_NoMemory);
            }

            text[size] = 0;
            memcpy(text, &buff2[posn + 8], size);

            ID3_AddAlbum(this, text);

            delete[] text;
          }

          // the Lyrics/Music AUTHOR field
          if (strcmp((char *) fid, "AUT") == 0)
          {
            char *text;

            text = new char[size + 1];
            if (NULL == text)
            {
              ID3_THROW(ID3E_NoMemory);
            }

            text[size] = 0;
            memcpy(text, &buff2[posn + 8], size);

            ID3_AddLyricist(this, text);

            delete[] text;
          }

          // the INFORMATION field
          if (strcmp((char *) fid, "INF") == 0)
          {
            luint newSize;

            newSize = ID3_CRLFtoLF((char *) & buff2[posn + 8], size);

            textInf = new char[newSize + 1];
            if (NULL == textInf)
            {
              ID3_THROW(ID3E_NoMemory);
            }

            textInf[newSize] = 0;

            memcpy(textInf, &buff2[posn + 8], newSize);

            // if already found the lyrics text use this field as description
            if (NULL != pLyrFrame)
              pLyrFrame->Field(ID3FN_DESCRIPTION) = textInf;
          }

          // the LYRICS field
          if (strcmp((char *) fid, "LYR") == 0)
          {
            uchar *text;
            luint newSize;

            newSize = ID3_CRLFtoLF((char *) & buff2[posn + 8], size);

            if (!stampsUsed)
            {
              text = new uchar[newSize + 1];
              if (NULL == text)
              {
                ID3_THROW(ID3E_NoMemory);
              }

              text[newSize] = 0;

              memcpy(text, &buff2[posn + 8], newSize);

              pLyrFrame = ID3_AddLyrics(this, (const char*) text);
              if (pLyrFrame)
              {
                pLyrFrame->Field(ID3FN_LANGUAGE) = "eng";

                // if already found an INF field, use it as description
                if (NULL != textInf)
                {
                  pLyrFrame->Field(ID3FN_DESCRIPTION) = textInf;
                }
                else
                {
                  pLyrFrame->Field(ID3FN_DESCRIPTION) = 
                    "Converted from Lyrics3 v2.00";
                }
              }

              delete[] text;
            }

            // convert lyrics into a SYLT frame Content Descriptor
            newSize = ID3_Lyrics3ToSylt (& buff2[posn + 8], newSize);

            text = new uchar[newSize + 1];
            if (NULL == text)
            {
              ID3_THROW(ID3E_NoMemory);
            }

            text[newSize] = 0;

            memcpy(text, &buff2[posn + 8], newSize);
            
            // if already found an INF field, use it as description
            const char* description = 
              (textInf ? textInf : "Converted from Lyrics3 v2.00");
            pLyrFrame =
              ID3_AddSyncLyrics(this, "eng", description, text, newSize);
            if (pLyrFrame)
            {
              pLyrFrame->Field(ID3FN_TIMESTAMPFORMAT) = ID3TSF_MS;
              pLyrFrame->Field(ID3FN_CONTENTTYPE) = ID3CT_LYRICS;
            }

            delete[] text;
          }

          posn += size + 8;
        }

        delete [] buff2;
        if (NULL != textInf)
        {
          delete[] textInf;
        }
      }
    }
  }
}
