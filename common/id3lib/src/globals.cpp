// $Id: globals.cpp,v 1.1 2002/01/21 08:16:22 menno Exp $

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

#include "globals.h"

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if 0
  const char * const ID3LIB_NAME           = __ID3LIB_NAME;
  const char * const ID3LIB_DATE           = __ID3LIB_DATE;
  const char * const ID3LIB_VERSION        = __ID3LIB_VERSION;
  const char * const ID3LIB_FULL_NAME      = __ID3LIB_FULLNAME;
  const int          ID3LIB_MAJOR_VERSION  = __ID3LIB_MAJOR_VERSION;
  const int          ID3LIB_MINOR_VERSION  = __ID3LIB_MINOR_VERSION;
  const int          ID3LIB_PATCH_VERSION  = __ID3LIB_PATCH_VERSION;
  const int          ID3LIB_INTERFACE_AGE  = __ID3LIB_INTERFACE_AGE;
  const int          ID3LIB_BINARY_AGE     = __ID3LIB_BINARY_AGE;
#endif

#ifdef __cplusplus
}
#endif
