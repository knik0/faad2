// $Id: tag_parse_v1.cpp,v 1.1 2002/01/21 08:16:22 menno Exp $

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
#include <string.h>
#include <memory.h>
#include "tag.h"
#include "misc_support.h"

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

void ID3_RemoveTrailingSpaces(char *buffer, luint length)
{
  for (lsint i = length - 1; i > -1 && 0x20 == buffer[i]; i--)
  {
    buffer[i] = '\0';
  }
      
  return ;
}


void ID3_Tag::ParseID3v1(void)
{
  if (NULL == __file_handle)
  {
    ID3_THROW(ID3E_NoData);
  }

  ID3V1_Tag tagID3v1;
    
  // posn ourselves at 128 bytes from the end of the file
  if (fseek(__file_handle, 0-ID3_V1_LEN, SEEK_END) != 0)
  {
    return;
    // TODO:  This is a bad error message.  Make it more descriptive
    //ID3_THROW(ID3E_NoData);
  }
    
    
  // read the next 128 bytes in;
  if (fread(tagID3v1.sID, 1, ID3_V1_LEN_ID, __file_handle) != ID3_V1_LEN_ID)
  {
    // TODO:  This is a bad error message.  Make it more descriptive
    ID3_THROW(ID3E_NoData);
  }
    
  // check to see if it was a tag
  if (memcmp(tagID3v1.sID, "TAG", ID3_V1_LEN_ID) == 0)
  {
    // guess so, let's start checking the v2 tag for frames which are the
    // equivalent of the v1 fields.  When we come across a v1 field that has
    // no current equivalent v2 frame, we create the frame, copy the data
    // from the v1 frame and attach it to the tag
      
    __file_tags.add(ID3TT_ID3V1);
    __ending_bytes += ID3_V1_LEN;

    // the TITLE field/frame
    if (fread(tagID3v1.sTitle, 1, ID3_V1_LEN_TITLE, __file_handle) != ID3_V1_LEN_TITLE)
    {
      // TODO:  This is a bad error message.  Make it more descriptive
      ID3_THROW(ID3E_NoData);
    }
    tagID3v1.sTitle[ID3_V1_LEN_TITLE] = '\0';
    ID3_RemoveTrailingSpaces(tagID3v1.sTitle,  ID3_V1_LEN_TITLE);
    ID3_AddTitle(this, tagID3v1.sTitle);
    
    // the ARTIST field/frame
    if (fread(tagID3v1.sArtist, 1, ID3_V1_LEN_ARTIST, __file_handle) != 
        ID3_V1_LEN_ARTIST)
    {
      // TODO:  This is a bad error message.  Make it more descriptive
      ID3_THROW(ID3E_NoData);
    }
    tagID3v1.sArtist[ID3_V1_LEN_ARTIST] = '\0';
    ID3_RemoveTrailingSpaces(tagID3v1.sArtist, ID3_V1_LEN_ARTIST);
    ID3_AddArtist(this, tagID3v1.sArtist);
  
    // the ALBUM field/frame
    if (fread(tagID3v1.sAlbum, 1, ID3_V1_LEN_ALBUM, __file_handle) != ID3_V1_LEN_ALBUM)
    {
      // TODO:  This is a bad error message.  Make it more descriptive
      ID3_THROW(ID3E_NoData);
    }
    tagID3v1.sAlbum[ID3_V1_LEN_ALBUM] = '\0';
    ID3_RemoveTrailingSpaces(tagID3v1.sAlbum,  ID3_V1_LEN_ALBUM);
    ID3_AddAlbum(this, tagID3v1.sAlbum);
  
    // the YEAR field/frame
    if (fread(tagID3v1.sYear, 1, ID3_V1_LEN_YEAR, __file_handle) != ID3_V1_LEN_YEAR)
    {
      // TODO:  This is a bad error message.  Make it more descriptive
      ID3_THROW(ID3E_NoData);
    }
    tagID3v1.sYear[ID3_V1_LEN_YEAR] = '\0';
    ID3_RemoveTrailingSpaces(tagID3v1.sYear,   ID3_V1_LEN_YEAR);
    ID3_AddYear(this, tagID3v1.sYear);
  
    // the COMMENT field/frame
    if (fread(tagID3v1.sComment, 1, ID3_V1_LEN_COMMENT, __file_handle) !=
        ID3_V1_LEN_COMMENT)
    {
      // TODO:  This is a bad error message.  Make it more descriptive
      ID3_THROW(ID3E_NoData);
    }
    tagID3v1.sComment[ID3_V1_LEN_COMMENT] = '\0';
    if ('\0' != tagID3v1.sComment[ID3_V1_LEN_COMMENT - 2] ||
        '\0' == tagID3v1.sComment[ID3_V1_LEN_COMMENT - 1])
    {
      ID3_RemoveTrailingSpaces(tagID3v1.sComment, ID3_V1_LEN_COMMENT);
    }
    else
    {
      // This is an id3v1.1 tag.  The last byte of the comment is the track
      // number.  
      ID3_RemoveTrailingSpaces(tagID3v1.sComment, ID3_V1_LEN_COMMENT - 1);
      ID3_AddTrack(this, tagID3v1.sComment[ID3_V1_LEN_COMMENT - 1]);
    }
    ID3_AddComment(this, tagID3v1.sComment, STR_V1_COMMENT_DESC);
      
    // the GENRE field/frame
    fread(&tagID3v1.ucGenre, 1, ID3_V1_LEN_GENRE, __file_handle);
    ID3_AddGenre(this, tagID3v1.ucGenre);
  }
    
  return ;
}
