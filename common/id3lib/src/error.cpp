// $Id: error.cpp,v 1.1 2002/01/21 08:16:21 menno Exp $

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
#include "error.h"

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

static char *ID3_ErrorDescs[] =
{
  "no error",
  "out of memory",
  "no source/dest data specified",
  "improperly formatted id3 tag",
  "no buffer specified",
  "buffer is insufficient in size",
  "invalid frame id",
  "field not found",
  "unknown field type",
  "tag is already attached to a file",
  "invalid tag version",
  "file not found",
  "trying to write to a read-only file",
  "error in zlib compression library"
};
  
ID3_Error::ID3_Error(ID3_Err eCode, const char *sFileName, 
                     size_t nLine, const char *sDesc)
{
  __error = eCode;
  __line_num = nLine;
  size_t nFileNameLen = strlen(sFileName);
  size_t nDescLen = strlen(sDesc);
  
  __file_name = new char[nFileNameLen+1];
  __description     = new char[nDescLen+1];
  strncpy(__file_name, sFileName, nFileNameLen+1);
  strncpy(__description, sDesc, nDescLen+1);
}


ID3_Err ID3_Error::GetErrorID() const
{
  return __error;
}


char *ID3_Error::GetErrorType() const
{
  return ID3_ErrorDescs[__error];
}

char *ID3_Error::GetErrorDesc() const
{
  return __description;
}

char *ID3_Error::GetErrorFile() const
{
  return __file_name;
}


size_t ID3_Error::GetErrorLine() const
{
  return __line_num;
}
