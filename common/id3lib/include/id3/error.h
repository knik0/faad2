// $Id: error.h,v 1.1 2002/01/21 08:16:20 menno Exp $

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

#ifndef __ID3LIB_ERROR_H__
#define __ID3LIB_ERROR_H__

#include "globals.h"

/** When id3lib encounters a nasty error, it thros an exception of type
 ** ID3_Error.  A function that calls an id3lib routine can place the call in a
 ** try block and provide an appropriate catch block.
 ** 
 ** <pre>
 ** try
 ** {
 **   // call some id3lib routine
 **   ID3_Tag myTag("mySong.mp3");
 **   ID3_Frame *myFrame = NULL;
 **   
 **   // this will generate an exception
 **   myTag << myFrame;
 ** }
 ** catch (ID3_Error err)
 ** {
 **   // handle the error
 **   ...
 ** }</pre>
 **/
class ID3_Error
{
public:
  /** Returns the ID3_Err value, which represents the ID of the error
   ** that caused the exception.
   **/
  ID3_Err GetErrorID() const;
  /** Returns the English string that defines the error type.
   ** 
   ** Each error ID has a set string error type.
   **/
  char   *GetErrorType() const;
  /** Returns a string that gives more explanation as to what caused the
   ** exception, if enabled by the code that caused the exception.
   **/
  char   *GetErrorDesc() const;
  /** Returns a pointer to a string of characters that is the name
   ** of the id3lib source file that generated the exception.
   ** 
   ** When submitting bug reports, it is useful to include the following.
   ** 
   ** <pre>
   ** cout << "Exception in file '" << err.GetErrorFile() << "'" << endl;</pre>
   **/
  char   *GetErrorFile() const;
  /** Returns the line number in the id3lib source file that threw the
   ** exception.
   ** 
   ** <pre>cout << "Line #" << err.GetErrorLine() << endl;</pre>
   **/
  size_t   GetErrorLine() const;
  
  /** Constructor
   ** 
   ** @param eID          Erroy id
   ** @param sFileName    Filename where error occurred
   ** @param nLineNum     Linenumber where error occurred
   ** @param sDescription Description of error
   **/
  ID3_Error(ID3_Err eID, const char *sFileName, size_t nLineNum, 
            const char *sDescription);
private:
  ID3_Err __error;
  size_t  __line_num;
  char   *__file_name;
  char   *__description;
};

/** Shortcut macro for throwing an error without a description
 ** 
 ** @param x The error id
 **/
#define ID3_THROW(x) throw ID3_Error(x, __FILE__, __LINE__, "")
/** Shortcut macro for throwing an error with a description
 ** 
 ** @param x The error id
 ** @param y The error description
 **/
#define ID3_THROW_DESC(x, y) throw ID3_Error(x, __FILE__, __LINE__, y)

#endif /* __ID3LIB_ERROR_H__ */
