// $Id: field.cpp,v 1.1 2002/01/21 08:16:21 menno Exp $

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
#include "field.h"

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

// This is used for unimplemented frames so that their data is preserved when
// parsing and rendering
static ID3_FieldDef ID3FD_Unimplemented[] =
{
  {
    ID3FN_DATA,                         // FIELD NAME
    ID3FTY_BINARY,                      // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  }
};

const ID3_FieldDef* ID3_FieldDef::DEFAULT = ID3FD_Unimplemented;

static ID3_FieldDef ID3FD_URL[] =
{ 
  {
    ID3FN_URL,                          // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};
  
static ID3_FieldDef ID3FD_UserURL[] =
{
  {
    ID3FN_TEXTENC,                      // FIELD NAME        
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DESCRIPTION,                  // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR | ID3FF_ENCODABLE,       // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_URL,                          // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};
  
static ID3_FieldDef ID3FD_Text[] =
{
  {
    ID3FN_TEXTENC,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_TEXT,                         // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_ENCODABLE,                    // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};
  
  
static ID3_FieldDef ID3FD_UserText[] =
{
  {
    ID3FN_TEXTENC,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DESCRIPTION,                  // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR | ID3FF_ENCODABLE,       // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_TEXT,                         // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_ENCODABLE,                    // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};
  
  
static ID3_FieldDef ID3FD_GeneralText[] =
{
  {
    ID3FN_TEXTENC,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_LANGUAGE,                     // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    3,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DESCRIPTION,                  // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR | ID3FF_ENCODABLE,       // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_TEXT,                         // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_ENCODABLE,                    // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};

static ID3_FieldDef ID3FD_TermsOfUse[] =
{
  {
    ID3FN_TEXTENC,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_3_0,                          // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_LANGUAGE,                     // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    3,                                  // FIXED LEN
    ID3V2_3_0,                          // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_TEXT,                         // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_3_0,                          // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_ENCODABLE,                    // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};

static ID3_FieldDef ID3FD_LinkedInfo[] =
{
  {
    ID3FN_ID,                           // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    3,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_2_1,                          // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_ID,                           // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    4,                                  // FIXED LEN
    ID3V2_3_0,                          // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_URL,                          // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_TEXT,                         // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};

static ID3_FieldDef ID3FD_Picture[] =
{
  {
    ID3FN_TEXTENC,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_IMAGEFORMAT,                  // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    3,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_2_1,                          // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_MIMETYPE,                     // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_3_0,                          // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_PICTURETYPE,                  // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DESCRIPTION,                  // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR | ID3FF_ENCODABLE,       // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DATA,                         // FIELD NAME
    ID3FTY_BINARY,                      // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};
  
static ID3_FieldDef ID3FD_GEO[] =
{
  {
    ID3FN_TEXTENC,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_MIMETYPE,                     // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_FILENAME,                     // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR | ID3FF_ENCODABLE,       // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DESCRIPTION,                  // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR | ID3FF_ENCODABLE,       // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DATA,                         // FIELD NAME
    ID3FTY_BINARY,                      // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};
  
static ID3_FieldDef ID3FD_UFI[] =
{
  {
    ID3FN_OWNER,                        // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DATA,                         // FIELD NAME
    ID3FTY_BINARY,                      // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};
  
static ID3_FieldDef ID3FD_PlayCounter[] =
{
  {
    ID3FN_COUNTER,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    4,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};
  
static ID3_FieldDef ID3FD_Popularimeter[] =
{
  {
    ID3FN_EMAIL,                        // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_RATING,                       // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_COUNTER,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    4,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};

static ID3_FieldDef ID3FD_Private[] =
{
  {
    ID3FN_OWNER,                        // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_3_0,                          // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DATA,                         // FIELD NAME
    ID3FTY_BINARY,                      // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_3_0,                          // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};
  
  
static ID3_FieldDef ID3FD_Registration[] =
{
  {
    ID3FN_OWNER,                        // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_3_0,                          // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_ID,                           // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_3_0,                          // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DATA,                         // FIELD NAME
    ID3FTY_BINARY,                      // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_3_0,                          // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};
  
static ID3_FieldDef ID3FD_InvolvedPeople[] =
{
  {
    ID3FN_TEXTENC,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_TEXT,                         // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_TEXTLIST,                     // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};

static ID3_FieldDef ID3FD_CDM[] =
{
  {
    ID3FN_DATA,                         // FIELD NAME
    ID3FTY_BINARY,                      // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_2_1,                          // INITIAL SPEC
    ID3V2_2_1,                          // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  }
};

static ID3_FieldDef ID3FD_SyncLyrics[] = 
{
  {
    ID3FN_TEXTENC,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_LANGUAGE,                     // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    3,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_TIMESTAMPFORMAT,              // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_CONTENTTYPE,                  // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DESCRIPTION,                  // FIELD NAME
    ID3FTY_TEXTSTRING,                  // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_CSTR | ID3FF_ENCODABLE,       // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_DATA,                         // FIELD NAME
    ID3FTY_BINARY,                      // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};


/*
 * Currently unused
 */
#if defined __UNDEFINED__
static ID3_FieldDef ID3FD_Volume[] =
{
  {
    ID3FN_VOLUMEADJ,                    // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_NUMBITS,                      // FIELD NAME
    ID3FTY_INTEGER,                     // FIELD TYPE
    1,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_NONE,                         // FLAGS
    ID3FN_NOFIELD                       // LINKED FIELD
  },
  {
    ID3FN_VOLCHGRIGHT,                  // FIELD NAME
    ID3FTY_BITFIELD,                    // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_ADJUSTEDBY,                   // FLAGS
    ID3FN_NUMBITS                       // LINKED FIELD
  },
  {
    ID3FN_VOLCHGLEFT,                   // FIELD NAME
    ID3FTY_BITFIELD,                    // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_ADJUSTEDBY,                   // FLAGS
    ID3FN_NUMBITS                       // LINKED FIELD
  },
  {
    ID3FN_PEAKVOLRIGHT,                 // FIELD NAME
    ID3FTY_BITFIELD,                    // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_ADJUSTEDBY,                   // FLAGS
    ID3FN_NUMBITS                       // LINKED FIELD
  },
  {
    ID3FN_PEAKVOLLEFT,                  // FIELD NAME
    ID3FTY_BITFIELD,                    // FIELD TYPE
    0,                                  // FIXED LEN
    ID3V2_EARLIEST,                     // INITIAL SPEC
    ID3V2_LATEST,                       // ENDING SPEC
    ID3FF_ADJUSTEDBY,                   // FLAGS
    ID3FN_NUMBITS                       // LINKED FIELD
  },
  { ID3FN_NOFIELD }
};
#endif /* __UNEFINED__ */

// **** Currently Implemented Frames
// APIC  PIC  ID3FID_PICTURE           Attached picture
// COMM  COM  ID3FID_COMMENT           Comments
// ENCR       ID3FID_CRYPTOREG         Encryption method registration
// GEOB  GEO  ID3FID_GENERALOBJECT     General encapsulated object
// GRID       ID3FID_GROUPINGREG       Group identification registration
// IPLS  IPL  ID3FID_INVOLVEDPEOPLE    Involved people list
// LINK  LNK  ID3FID_LINKEDINFO        Linked information
// PCNT  CNT  ID3FID_PLAYCOUNTER       Play counter
// POPM  POP  ID3FID_POPULARIMETER     Popularimeter
// PRIV       ID3FID_PRIVATE           Private frame
// SYLT  SLT  ID3FID_SYNCEDLYRICS      Synchronized lyric/text
// TALB  TAL  ID3FID_ALBUM             Album/Movie/Show title
// TBPM  TBP  ID3FID_BPM               BPM (beats per minute)
// TCOM  TCM  ID3FID_COMPOSER          Composer
// TCON  TCO  ID3FID_CONTENTTYPE       Content type
// TCOP  TCR  ID3FID_COPYRIGHT         Copyright message
// TDAT  TDA  ID3FID_DATE              Date
// TDLY  TDY  ID3FID_PLAYLISTDELAY     Playlist delay
// TENC  TEN  ID3FID_ENCODEDBY         Encoded by
// TEXT  TXT  ID3FID_LYRICIST          Lyricist/Text writer
// TFLT  TFT  ID3FID_FILETYPE          File type
// TIME  TKE  ID3FID_TIME              Time
// TIT1  TIM  ID3FID_CONTENTGROUP      Content group description
// TIT2  TT1  ID3FID_TITLE             Title/songname/content description
// TIT3  TT2  ID3FID_SUBTITLE          Subtitle/Description refinement
// TKEY  TT3  ID3FID_INITIALKEY        Initial key
// TLAN  TLA  ID3FID_LANGUAGE          Language(s)
// TLEN  TLE  ID3FID_SONGLEN           Length
// TMED  TMT  ID3FID_MEDIATYPE         Media type
// TOAL  TOT  ID3FID_ORIGALBUM         Original album/movie/show title
// TOFN  TOF  ID3FID_ORIGFILENAME      Original filename
// TOLY  TOL  ID3FID_ORIGLYRICIST      Original lyricist(s)/text writer(s)
// TOPE  TOA  ID3FID_ORIGARTIST        Original artist(s)/performer(s)
// TORY  TOR  ID3FID_ORIGYEAR          Original release year
// TOWN       ID3FID_FILEOWNER         File owner/licensee
// TPE1  TP1  ID3FID_LEADARTIST        Lead performer(s)/Soloist(s)
// TPE2  TP2  ID3FID_BAND              Band/orchestra/accompaniment
// TPE3  TP3  ID3FID_CONDUCTOR         Conductor/performer refinement
// TPE4  TP4  ID3FID_MIXARTIST         Interpreted, remixed, or otherwise modified
// TPOS  TPA  ID3FID_PARTINSET         Part of a set
// TPUB  TPB  ID3FID_PUBLISHER         Publisher
// TRCK  TRK  ID3FID_TRACKNUM          Track number/Position in set
// TRDA  TRD  ID3FID_RECORDINGDATES    Recording dates
// TRSN  TRN  ID3FID_NETRADIOSTATION   Internet radio station name
// TRSO  TRO  ID3FID_NETRADIOOWNER     Internet radio station owner
// TSIZ  TSI  ID3FID_SIZE              Size
// TSRC  TRC  ID3FID_ISRC              ISRC (international standard recording code)
// TSSE  TSS  ID3FID_ENCODERSETTINGS   Software/Hardware and encoding settings
// TXXX  TXX  ID3FID_USERTEXT          User defined text information
// TYER  TYE  ID3FID_YEAR              Year
// UFID  UFI  ID3FID_UNIQUEFILEID      Unique file identifier
// USER       ID3FID_TERMSOFUSE        Terms of use
// USLT  ULT  ID3FID_UNSYNCEDLYRICS    Unsynchronized lyric/text transcription
// WCOM  WCM  ID3FID_WWWCOMMERCIALINFO Commercial information
// WCOP  WCM  ID3FID_WWWCOPYRIGHT      Copyright/Legal infromation
// WOAF  WCP  ID3FID_WWWAUDIOFILE      Official audio file webpage
// WOAR  WAF  ID3FID_WWWARTIST         Official artist/performer webpage
// WOAS  WAR  ID3FID_WWWAUDIOSOURCE    Official audio source webpage
// WORS  WAS  ID3FID_WWWRADIOPAGE      Official internet radio station homepage
// WPAY  WRA  ID3FID_WWWPAYMENT        Payment
// WPUB  WPY  ID3FID_WWWPUBLISHER      Official publisher webpage
// WXXX  WXX  ID3FID_WWWUSER           User defined URL link
//       CDM  ID3FID_METACOMPRESSION   Compressed data meta frame

// **** Currently unimplemented frames
// AENC  CRA  ID3FID_AUDIOCRYPTO       Audio encryption
// COMR       ID3FID_COMMERCIAL        Commercial frame
// EQUA  EQU  ID3FID_EQUALIZATION      Equalization
// ETCO  ETC  ID3FID_EVENTTIMING       Event timing codes
// MCDI  MCI  ID3FID_CDID              Music CD identifier
// MLLT  MLL  ID3FID_MPEGLOOKUP        MPEG location lookup table
// OWNE       ID3FID_OWNERSHIP         Ownership frame
// POSS       ID3FID_POSITIONSYNC      Position synchronisation frame
// RBUF  BUF  ID3FID_BUFFERSIZE        Recommended buffer size
// RVAD  RVA  ID3FID_VOLUMEADJ         Relative volume adjustment
// RVRB  REV  ID3FID_REVERB            Reverb
// SYTC  STC  ID3FID_SYNCEDTEMPO       Synchronized tempo codes
//       CRM  ID3FID_METACRYPTO        Encrypted meta frame
static  ID3_FrameDef ID3_FrameDefs[] =
{
  //                          short  long   tag    file
  // frame id                 id     id     discrd discrd field defs           description
  {ID3FID_AUDIOCRYPTO,       "CRA", "AENC", false, false, ID3FD_Unimplemented, "Audio encryption"},
  {ID3FID_PICTURE,           "PIC", "APIC", false, false, ID3FD_Picture,       "Attached picture"},
  {ID3FID_COMMENT,           "COM", "COMM", false, false, ID3FD_GeneralText,   "Comments"},
  {ID3FID_COMMERCIAL,        ""   , "COMR", false, false, ID3FD_Unimplemented, "Commercial"},
  {ID3FID_CRYPTOREG,         ""   , "ENCR", false, false, ID3FD_Registration,  "Encryption method registration"},
  {ID3FID_EQUALIZATION,      "EQU", "EQUA", false, true,  ID3FD_Unimplemented, "Equalization"},
  {ID3FID_EVENTTIMING,       "ETC", "ETCO", false, true,  ID3FD_Unimplemented, "Event timing codes"},
  {ID3FID_GENERALOBJECT,     "GEO", "GEOB", false, false, ID3FD_GEO,           "General encapsulated object"},
  {ID3FID_GROUPINGREG,       ""   , "GRID", false, false, ID3FD_Registration,  "Group identification registration"},
  {ID3FID_INVOLVEDPEOPLE,    "IPL", "IPLS", false, false, ID3FD_InvolvedPeople,"Involved people list"},
  {ID3FID_LINKEDINFO,        "LNK", "LINK", false, false, ID3FD_LinkedInfo,    "Linked information"},
  {ID3FID_CDID,              "MCI", "MCDI", false, false, ID3FD_Unimplemented, "Music CD identifier"},
  {ID3FID_MPEGLOOKUP,        "MLL", "MLLT", false, true,  ID3FD_Unimplemented, "MPEG location lookup table"},
  {ID3FID_OWNERSHIP,         ""   , "OWNE", false, false, ID3FD_Unimplemented, "Ownership frame"},
  {ID3FID_PLAYCOUNTER,       "CNT", "PCNT", false, false, ID3FD_PlayCounter,   "Play counter"},
  {ID3FID_POPULARIMETER,     "POP", "POPM", false, false, ID3FD_Popularimeter, "Popularimeter"},
  {ID3FID_POSITIONSYNC,      ""   , "POSS", false, true,  ID3FD_Unimplemented, "Position synchronisation frame"},
  {ID3FID_PRIVATE,           ""   , "PRIV", false, false, ID3FD_Private,       "Private frame"},
  {ID3FID_BUFFERSIZE,        "BUF", "RBUF", false, false, ID3FD_Unimplemented, "Recommended buffer size"},
  {ID3FID_VOLUMEADJ,         "RVA", "RVAD", false, true,  ID3FD_Unimplemented, "Relative volume adjustment"},
  {ID3FID_REVERB,            "REV", "RVRB", false, false, ID3FD_Unimplemented, "Reverb"},
  {ID3FID_SYNCEDLYRICS,      "SLT", "SYLT", false, false, ID3FD_SyncLyrics,	"Synchronized lyric/text"},
  {ID3FID_SYNCEDTEMPO,       "STC", "SYTC", false, true,  ID3FD_Unimplemented, "Synchronized tempo codes"},
  {ID3FID_ALBUM,             "TAL", "TALB", false, false, ID3FD_Text,          "Album/Movie/Show title"},
  {ID3FID_BPM,               "TBP", "TBPM", false, false, ID3FD_Text,          "BPM (beats per minute)"},
  {ID3FID_COMPOSER,          "TCM", "TCOM", false, false, ID3FD_Text,          "Composer"},
  {ID3FID_CONTENTTYPE,       "TCO", "TCON", false, false, ID3FD_Text,          "Content type"},
  {ID3FID_COPYRIGHT,         "TCR", "TCOP", false, false, ID3FD_Text,          "Copyright message"},
  {ID3FID_DATE,              "TDA", "TDAT", false, false, ID3FD_Text,          "Date"},
  {ID3FID_PLAYLISTDELAY,     "TDY", "TDLY", false, false, ID3FD_Text,          "Playlist delay"},
  {ID3FID_ENCODEDBY,         "TEN", "TENC", false, true,  ID3FD_Text,          "Encoded by"},
  {ID3FID_LYRICIST,          "TXT", "TEXT", false, false, ID3FD_Text,          "Lyricist/Text writer"},
  {ID3FID_FILETYPE,          "TFT", "TFLT", false, false, ID3FD_Text,          "File type"},
  {ID3FID_INITIALKEY,        "TKE", "TKEY", false, false, ID3FD_Text,          "Initial key"},
  {ID3FID_TIME,              "TIM", "TIME", false, false, ID3FD_Text,          "Time"},
  {ID3FID_CONTENTGROUP,      "TT1", "TIT1", false, false, ID3FD_Text,          "Content group description"},
  {ID3FID_TITLE,             "TT2", "TIT2", false, false, ID3FD_Text,          "Title/songname/content description"},
  {ID3FID_SUBTITLE,          "TT3", "TIT3", false, false, ID3FD_Text,          "Subtitle/Description refinement"},
  {ID3FID_LANGUAGE,          "TLA", "TLAN", false, false, ID3FD_Text,          "Language(s)"},
  {ID3FID_SONGLEN,           "TLE", "TLEN", false, true,  ID3FD_Text,          "Length"},
  {ID3FID_MEDIATYPE,         "TMT", "TMED", false, false, ID3FD_Text,          "Media type"},
  {ID3FID_ORIGALBUM,         "TOT", "TOAL", false, false, ID3FD_Text,          "Original album/movie/show title"},
  {ID3FID_ORIGFILENAME,      "TOF", "TOFN", false, false, ID3FD_Text,          "Original filename"},
  {ID3FID_ORIGLYRICIST,      "TOL", "TOLY", false, false, ID3FD_Text,          "Original lyricist(s)/text writer(s)"},
  {ID3FID_ORIGARTIST,        "TOA", "TOPE", false, false, ID3FD_Text,          "Original artist(s)/performer(s)"},
  {ID3FID_ORIGYEAR,          "TOR", "TORY", false, false, ID3FD_Text,          "Original release year"},
  {ID3FID_FILEOWNER,         ""   , "TOWN", false, false, ID3FD_Text,          "File owner/licensee"},
  {ID3FID_LEADARTIST,        "TP1", "TPE1", false, false, ID3FD_Text,          "Lead performer(s)/Soloist(s)"},
  {ID3FID_BAND,              "TP2", "TPE2", false, false, ID3FD_Text,          "Band/orchestra/accompaniment"},
  {ID3FID_CONDUCTOR,         "TP3", "TPE3", false, false, ID3FD_Text,          "Conductor/performer refinement"},
  {ID3FID_MIXARTIST,         "TP4", "TPE4", false, false, ID3FD_Text,          "Interpreted, remixed, or otherwise modified by"},
  {ID3FID_PARTINSET,         "TPA", "TPOS", false, false, ID3FD_Text,          "Part of a set"},
  {ID3FID_PUBLISHER,         "TPB", "TPUB", false, false, ID3FD_Text,          "Publisher"},
  {ID3FID_TRACKNUM,          "TRK", "TRCK", false, false, ID3FD_Text,          "Track number/Position in set"},
  {ID3FID_RECORDINGDATES,    "TRD", "TRDA", false, false, ID3FD_Text,          "Recording dates"},
  {ID3FID_NETRADIOSTATION,   "TRN", "TRSN", false, false, ID3FD_Text,          "Internet radio station name"},
  {ID3FID_NETRADIOOWNER,     "TRO", "TRSO", false, false, ID3FD_Text,          "Internet radio station owner"},
  {ID3FID_SIZE,              "TSI", "TSIZ", false, true,  ID3FD_Text,          "Size"},
  {ID3FID_ISRC,              "TRC", "TSRC", false, false, ID3FD_Text,          "ISRC (international standard recording code)"},
  {ID3FID_ENCODERSETTINGS,   "TSS", "TSSE", false, false, ID3FD_Text,          "Software/Hardware and settings used for encoding"},
  {ID3FID_USERTEXT,          "TXX", "TXXX", false, false, ID3FD_UserText,      "User defined text information"},
  {ID3FID_YEAR,              "TYE", "TYER", false, false, ID3FD_Text,          "Year"},
  {ID3FID_UNIQUEFILEID,      "UFI", "UFID", false, false, ID3FD_UFI,           "Unique file identifier"},
  {ID3FID_TERMSOFUSE,        ""   , "USER", false, false, ID3FD_TermsOfUse,    "Terms of use"},
  {ID3FID_UNSYNCEDLYRICS,    "ULT", "USLT", false, false, ID3FD_GeneralText,   "Unsynchronized lyric/text transcription"},
  {ID3FID_WWWCOMMERCIALINFO, "WCM", "WCOM", false, false, ID3FD_URL,           "Commercial information"},
  {ID3FID_WWWCOPYRIGHT,      "WCP", "WCOP", false, false, ID3FD_URL,           "Copyright/Legal infromation"},
  {ID3FID_WWWAUDIOFILE,      "WAF", "WOAF", false, false, ID3FD_URL,           "Official audio file webpage"},
  {ID3FID_WWWARTIST,         "WAR", "WOAR", false, false, ID3FD_URL,           "Official artist/performer webpage"},
  {ID3FID_WWWAUDIOSOURCE,    "WAS", "WOAS", false, false, ID3FD_URL,           "Official audio source webpage"},
  {ID3FID_WWWRADIOPAGE,      "WRA", "WORS", false, false, ID3FD_URL,           "Official internet radio station homepage"},
  {ID3FID_WWWPAYMENT,        "WPY", "WPAY", false, false, ID3FD_URL,           "Payment"},
  {ID3FID_WWWPUBLISHER,      "WPB", "WPUB", false, false, ID3FD_URL,           "Official publisher webpage"},
  {ID3FID_WWWUSER,           "WXX", "WXXX", false, false, ID3FD_UserURL,       "User defined URL link"},
  {ID3FID_METACRYPTO,        "CRM", ""    , false, false, ID3FD_Unimplemented, "Encrypted meta frame"},
  {ID3FID_METACOMPRESSION,   "CDM", ""    , false, false, ID3FD_CDM,           "Compressed data meta frame"},
  {ID3FID_NOFRAME}
};
  
ID3_Field::ID3_Field()
  : __id(ID3FN_NOFIELD),
    __type(ID3FTY_INTEGER),
    __length(0),
    __spec_begin(ID3V2_EARLIEST),
    __spec_end(ID3V2_LATEST),
    __flags(0),
    __changed(false),
    __data(NULL),
    __size(0),
    __enc(ID3TE_NONE)
{
  Clear();
}

ID3_Field::~ID3_Field()
{
  this->Clear();
}

void
ID3_Field::Clear()
{
  if (__data != NULL && __size > 0 && __type != ID3FTY_INTEGER)
  {
    delete[] __data;
  }
    
  __type       = ID3FTY_INTEGER;
  __data       = NULL;
  __size       = sizeof (uint32);
  __changed    = true;
  __enc        = ID3TE_NONE;
  
  return ;
}

bool
ID3_Field::HasChanged()
{
  return __changed;
}

size_t ID3_Field::Size() const
{
  return BinSize(false);
}

size_t ID3_Field::BinSize(bool withExtras) const
{
  size_t bytes   = 0;

  bytes = __size;
    
  // check to see if we are within the legal limit for this field 0 means
  // arbitrary length field
  if (__length > 0)
  {
    bytes = __length;
  }
  else if (withExtras)
  {
    if (NULL == __data && __size > 0)
    {
      bytes = (__flags & ID3FF_CSTR) ? sizeof(unicode_t) : 0;
    }
      
    // if we are a Unicode string, add 2 bytes for the BOM (but only if there
    // is a string to render - regardless of NULL)
    if (ID3TE_UNICODE == this->GetEncoding() && __data != NULL && __size > 0)
    {
      bytes += sizeof(unicode_t);
    }
        
    // if we are an ASCII string, divide by sizeof(unicode_t) because
    // internally we store the string as Unicode, so the ASCII version will
    // only be half as long
    if (__type == ID3FTY_TEXTSTRING && this->GetEncoding() != ID3TE_UNICODE)
    {
      bytes /= sizeof(unicode_t);
    }
  }
  else if (__type == ID3FTY_TEXTSTRING)
  {
    // because it seems that the application called us via ID3_Field::Size()
    // we are going to return the number of characters, not bytes.  since we
    // store every string internally as unicode, we will divide the 'bytes'
    // variable by the size of a unicode character (should be two bytes)
    // because Unicode strings have twice as many bytes as they do characters
    bytes /= sizeof(unicode_t);
  }
  
  return bytes;
}

size_t ID3_Field::Parse(const uchar *buffer, size_t buffSize)
{
  size_t bytesUsed       = 0;
  
  switch (this->GetType())
  {
    case ID3FTY_INTEGER:
      bytesUsed = ParseInteger(buffer, buffSize);
      break;
        
    case ID3FTY_BINARY:
      bytesUsed = ParseBinary(buffer, buffSize);
      break;
        
    case ID3FTY_TEXTSTRING:
      if (this->GetEncoding() == ID3TE_UNICODE)
      {
        bytesUsed = ParseUnicodeString(buffer, buffSize);
      }
      else
      {
        bytesUsed = ParseASCIIString(buffer, buffSize);
      }
      break;

    default:
      ID3_THROW(ID3E_UnknownFieldType);
      break;
  }
  
  return bytesUsed;
}

ID3_FrameDef *
ID3_FindFrameDef(const ID3_FrameID id)
{
  ID3_FrameDef  *info   = NULL;

  for (index_t cur = 0; ID3_FrameDefs[cur].eID != ID3FID_NOFRAME; cur++)
  {
    if (ID3_FrameDefs[cur].eID == id)
    {
      info = &ID3_FrameDefs[cur];
      break;
    }
  }
    
  return info;
}

ID3_FrameID
ID3_FindFrameID(const char *id)
{
  ID3_FrameID fid = ID3FID_NOFRAME;
  
  for (index_t cur = 0; ID3_FrameDefs[cur].eID != ID3FID_NOFRAME; cur++)
  {
    if (((strcmp(ID3_FrameDefs[cur].sShortTextID, id) == 0) &&
         strlen(id) == 3) ||
        ((strcmp(ID3_FrameDefs[cur].sLongTextID,  id) == 0) &&
         strlen(id) == 4))
    {
      fid = ID3_FrameDefs[cur].eID;
      break;
    }
  }
  
  return fid;
}

size_t ID3_Field::Render(uchar *buffer) const
{
  size_t bytesUsed = 0;
  
  switch (this->GetType()) 
  {
    case ID3FTY_INTEGER:
      bytesUsed = RenderInteger(buffer);
      break;
        
    case ID3FTY_BINARY:
      bytesUsed = RenderBinary(buffer);
      break;
        
    case ID3FTY_TEXTSTRING:
      if (this->GetEncoding() == ID3TE_UNICODE)
      {
        bytesUsed = RenderUnicodeString(buffer);
      }
      else
      {
        bytesUsed = RenderASCIIString(buffer);
      }
      break;
        
    default:
      ID3_THROW (ID3E_UnknownFieldType);
      break;
  }
    
  return bytesUsed;
}

ID3_Field &
ID3_Field::operator=( const ID3_Field &rhs )
{
  if (this != &rhs)
  {
    switch (rhs.GetType())
    {
      case ID3FTY_INTEGER:
      {
        *this = rhs.Get();
        break;
      }
      case ID3FTY_TEXTSTRING:
      case ID3FTY_BINARY:
      {
        this->Set(rhs.__data, rhs.__size);
        break;
      }
      default:
      {
        break;
      }
    }
  }
  __type = rhs.__type;
  return *this;
}

bool ID3_Field::SetEncoding(ID3_TextEnc enc)
{
  bool changed = this->IsEncodable() && (enc != this->GetEncoding()) &&
    (ID3TE_NONE < enc && enc < ID3TE_NUMENCODINGS);
  if (changed)
  {
    __enc = enc;
    __changed = true;
  }
  return changed;
}
