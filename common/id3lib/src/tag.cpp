// $Id: tag.cpp,v 1.1 2002/01/21 08:16:22 menno Exp $

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

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

#if defined HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "tag.h"
#include "uint28.h"
#include <string.h>

#ifdef  MAXPATHLEN
#  define ID3_PATH_LENGTH   (MAXPATHLEN + 1)
#elif   defined (PATH_MAX)
#  define ID3_PATH_LENGTH   (PATH_MAX + 1)
#else   /* !MAXPATHLEN */
#  define ID3_PATH_LENGTH   (2048 + 1)
#endif  /* !MAXPATHLEN && !PATH_MAX */

/** \class ID3_Tag
 ** \brief The representative class of an id3 tag.
 ** 
 ** This is the 'container' class for everything else.  It is through an
 ** ID3_Tag that most of the productive stuff happens.  
 ** Let's look at what's
 ** required to start using ID3v2 tags.
 ** 
 ** \code
 **   #include <id3/tag.h>
 ** \endcode
 ** 
 ** This simple \c #include does it all.  In order to read an
 ** existing tag, do the following:
 **
 ** \code
 **   ID3_Tag myTag;
 **   myTag.Link("something.mp3");
 ** \endcode
 ** 
 ** That is all there is to it.  Now all you have to do is use the 
 ** Find() method to locate the frames you are interested in
 ** is the following:
 ** 
 ** \code
 **   ID3_Frame *myFrame;
 **   if (myTag.Find(ID3FID_TITLE) == myFrame)
 **   {
 **     char title[1024];
 **     myFrame->Field(ID3FN_TEXT).Get(title, 1024);
 **     cout << "Title: " << title << endl;
 **   }
 ** \endcode
 ** 
 ** This code snippet locates the ID3FID_TITLE frame and copies the contents of
 ** the text field into a buffer and displays the buffer.  Not difficult, eh?
 **
 ** When using the ID3_Tag::Link() method, you automatically gain access to any
 ** ID3v1/1.1, ID3v2, and Lyrics3 v2.0 tags present in the file.  The class
 ** will automaticaly parse and convert any of these foreign tag formats into
 ** ID3v2 tags.  Also, id3lib will correctly parse any correctly formatted
 ** 'CDM' frames from the unreleased ID3v2 2.01 draft specification.
 **
 ** \author Dirk Mahoney
 ** \version $Id: tag.cpp,v 1.1 2002/01/21 08:16:22 menno Exp $
 ** \sa ID3_Frame
 ** \sa ID3_Field
 ** \sa ID3_Err
 **/

/** Analyses a buffer to determine if we have a valid ID3v2 tag header.
 ** If so, return the total number of bytes (including the header) to
 ** read so we get all of the tag
 **/
size_t ID3_Tag::IsV2Tag(const uchar* const data)
{
  lsint tagSize = 0;
  
  if (strncmp(ID3_TagHeader::ID, (char *)data, ID3_TagHeader::ID_SIZE) == 0 &&
      data[ID3_TagHeader::MAJOR_OFFSET]    <  0xFF &&
      data[ID3_TagHeader::MINOR_OFFSET]    <  0xFF &&
      data[ID3_TagHeader::SIZE_OFFSET + 0] <  0x80 &&
      data[ID3_TagHeader::SIZE_OFFSET + 1] <  0x80 &&
      data[ID3_TagHeader::SIZE_OFFSET + 2] <  0x80 &&
      data[ID3_TagHeader::SIZE_OFFSET + 3] <  0x80)
  {
    uint28 data_size(&data[ID3_TagHeader::SIZE_OFFSET]);
    tagSize = data_size.to_uint32() + ID3_TagHeader::SIZE;
  }
  
  return tagSize;
}

int32 ID3_IsTagHeader(const uchar data[ID3_TAGHEADERSIZE])
{
  size_t size = ID3_Tag::IsV2Tag(data);
  
  if (!size)
  {
    return -1;
  }
  
  return size - ID3_TagHeader::SIZE;
}

void ID3_RemoveFromList(ID3_Elem *which, ID3_Elem **list)
{
  ID3_Elem *cur = *list;

  if (cur == which)
  {
    *list = which->pNext;
    delete which;
    which = NULL;
  }
  else
  {
    while (cur)
    {
      if (cur->pNext == which)
      {
        cur->pNext = which->pNext;
        delete which;
        which = NULL;
        break;
      }
      else
      {
        cur = cur->pNext;
      }
    }
  }
}


void ID3_ClearList(ID3_Elem *list)
{
  ID3_Elem *next = NULL;
  for (ID3_Elem *cur = list; cur; cur = next)
  {
    next = cur->pNext;
    delete cur;
  }
}

/** Copies a frame to the tag.  The frame parameter can thus safely be deleted
 ** or allowed to go out of scope.
 ** 
 ** Operator<< supports the addition of a pointer to a frame object, or
 ** the frame object itself.
 **
 ** \code
 **   ID3_Frame *pFrame, frame;
 **   p_frame = &frame;
 **   myTag << pFrame;
 **   myTag << frame;
 ** \endcode
 **
 ** Both these methods copy the given frame to the tag---the tag creates its
 ** own copy of the frame.
 ** 
 ** \name operator<<
 ** \param frame The frame to be added to the tag.
 **/
ID3_Tag& operator<<(ID3_Tag& tag, const ID3_Frame& frame)
{
  tag.AddFrame(frame);
  
  return tag;
}


ID3_Tag& operator<<(ID3_Tag& tag, const ID3_Frame *frame)
{
  tag.AddFrame(frame);
  
  return tag;
}


/** Default constructor; it can accept an optional filename as a parameter.
 **
 ** If this file exists, it will be opened and all id3lib-supported tags will
 ** be parsed and converted to id3v2 if necessary.  After the conversion, the
 ** file will remain unchanged, and will continue to do so until you use the
 ** Update() method on the tag (if you choose to Update() at all).
 **
 ** \param name The filename of the mp3 file to link to
 **/
ID3_Tag::ID3_Tag(const char *name)
  : __frames(NULL),
    __file_name(new char[ID3_PATH_LENGTH]),
    __file_handle(NULL)
    
{
  this->Clear();
  if (name)
  {
    this->Link(name);
  }
}

/** Standard copy constructor.
 **
 ** \param tag What is copied into this tag
 **/
ID3_Tag::ID3_Tag(const ID3_Tag &tag)
  : __frames(NULL),
    __file_name(new char[ID3_PATH_LENGTH]),
    __file_handle(NULL)
{
  *this = tag;
}

ID3_Tag::~ID3_Tag()
{
  this->Clear();
  
  delete [] __file_name;
}

/** Clears the object and disassociates it from any files.
 **
 ** It frees any resources for which the object is responsible, and the
 ** object is now free to be used again for any new or existing tag.
 **/
void ID3_Tag::Clear()
{
  this->CloseFile();
  
  if (__frames)
  {
    ID3_ClearList(__frames);
    __frames = NULL;
  }
  __num_frames = 0;
  __cursor = NULL;
  __is_padded = true;
  
  __hdr.Clear();
  __hdr.SetSpec(ID3V2_LATEST);
  
  __file_size = 0;
  __starting_bytes = 0;
  __ending_bytes = 0;
  __is_file_writable = false;

  __tags_to_parse.clear();
  __file_tags.clear();

  __changed = true;
}


void ID3_Tag::AddFrame(const ID3_Frame& frame)
{
  this->AddFrame(&frame);
}

/** Attaches a frame to the tag; the tag doesn't take responsibility for
 ** releasing the frame's memory when tag goes out of scope.
 ** 
 ** Optionally, operator<< can also be used to attach a frame to a tag.  To
 ** use, simply supply its parameter a pointer to the ID3_Frame object you wish
 ** to attach.
 ** 
 ** \code
 **   ID3_Frame myFrame;
 **   myTag.AddFrame(&myFrame);
 ** \endcode 
 ** 
 ** As stated, this method attaches the frames to the tag---the tag does
 ** not create its own copy of the frame.  Frames created by an application
 ** must exist until the frame is removed or the tag is finished with it.
 ** 
 ** \param pFrame A pointer to the frame that is being added to the tag.
 ** \sa ID3_Frame
 **/
void ID3_Tag::AddFrame(const ID3_Frame *frame)
{
  if (frame)
  {
    ID3_Frame* new_frame = new ID3_Frame(*frame);
    this->AttachFrame(new_frame);
  }
}

/** Attaches a frame to the tag; the tag takes responsibility for
 ** releasing the frame's memory when tag goes out of scope.
 ** 
 ** This method accepts responsibility for the attached frame's memory, and
 ** will delete the frame and its contents when the tag goes out of scope or is
 ** deleted.  Therefore, be sure the frame isn't "Attached" to other tags.
 ** 
 ** \code
 **   ID3_Frame *frame = new ID3_Frame;
 **   myTag.AttachFrame(frame);
 ** \endcode
 ** 
 ** \param frame A pointer to the frame that is being added to the tag.
 **/
void ID3_Tag::AttachFrame(ID3_Frame *frame)
{
  ID3_Elem *elem;
  
  if (NULL == frame)
  {
    ID3_THROW(ID3E_NoData);
  }

  elem = new ID3_Elem;
  if (NULL == elem)
  {
    ID3_THROW(ID3E_NoMemory);
  }

  elem->pNext = __frames;
  elem->pFrame = frame;
  
  __frames = elem;
  __num_frames++;
  __cursor = NULL;
  
  __changed = true;
}


/** Copies an array of frames to the tag.
 ** 
 ** This method copies each frame in an array to the tag.  As in 
 ** AddFrame, the tag adds a copy of the frame, and it assumes responsiblity
 ** for freeing the frames' memory when the tag goes out of scope.
 ** 
 ** \code
 **   ID3_Frame myFrames[10];
 **   myTag.AddFrames(myFrames, 10);
 ** \endcode
 ** 
 ** \sa ID3_Frame
 ** \sa ID3_Frame#AddFrame
 ** \param pNewFrames A pointer to an array of frames to be added to the tag.
 ** \param nFrames The number of frames in the array pNewFrames.
 **/
void ID3_Tag::AddFrames(const ID3_Frame *frames, size_t numFrames)
{
  for (index_t i = numFrames - 1; i >= 0; i--)
  {
    AddFrame(frames[i]);
  }
}


/** Removes a frame from the tag.
 ** 
 ** If you already own the frame object in question, then you should already
 ** have a pointer to the frame you want to delete.  If not, or if you wish to
 ** delete a pre-existing frame (from a tag you have parsed, for example), the
 ** use one of the Find methods to obtain a frame pointer to pass to this
 ** method.
 ** 
 ** \code
 **   ID3_Frame *someFrame;
 **   if (someFrame = myTag.Find(ID3FID_TITLE))
 **   {
 **     myTag.RemoveFrame(someFrame);
 **   }
 ** \endcode
 **   
 ** \sa ID3_Tag#Find
 ** \param pOldFrame A pointer to the frame that is to be removed from the
 **                  tag
 **/
ID3_Frame* ID3_Tag::RemoveFrame(const ID3_Frame *frame)
{
  ID3_Frame *the_frame = NULL;
  ID3_Elem *elem = Find(frame);
  if (NULL != elem)
  {
    the_frame = elem->pFrame;
    //assert(the_frame == frame);
    elem->pFrame = NULL;
    ID3_RemoveFromList(elem, &__frames);
    --__num_frames;
  }
    
  return the_frame;
}


/** Indicates whether the tag has been altered since the last parse, render,
 ** or update.
 **
 ** If you have a tag linked to a file, you do not need this method since the
 ** Update() method will check for changes before writing the tag.
 ** 
 ** This method is primarily intended as a status indicator for applications
 ** and for applications that use the Parse() and Render() methods.
 **
 ** Setting a field, changed the ID of an attached frame, setting or grouping
 ** or encryption IDs, and clearing a frame or field all constitute a change
 ** to the tag, as do calls to the SetUnsync(), SetExtendedHeader(), and
 ** SetPadding() methods.
 ** 
 ** \code
 **   if (myTag.HasChanged())
 **   {
 **     // render and output the tag
 **   }
 ** \endcode
 ** 
 ** \return Whether or not the tag has been altered.
 **/
bool ID3_Tag::HasChanged() const
{
  bool changed = __changed;
  
  if (! changed)
  {
    ID3_Elem *cur = __frames;
    
    while (cur)
    {
      if (cur->pFrame)
      {
        changed = cur->pFrame->HasChanged();
      }
        
      if (changed)
      {
        break;
      }
      else
      {
        cur = cur->pNext;
      }
    }
  }
  
  return changed;
}

bool ID3_Tag::SetSpec(ID3_V2Spec spec)
{
  bool changed = __hdr.SetSpec(spec);
  __changed = __changed || changed;
  return changed;
}

ID3_V2Spec ID3_Tag::GetSpec() const
{
  return __hdr.GetSpec();
}

/** Turns unsynchronization on or off, dependant on the value of the boolean
 ** parameter.
 ** 
 ** If you call this method with 'false' as the parameter, the
 ** binary tag will not be unsync'ed, regardless of whether the tag should
 ** be.  This option is useful when the file is only going to be used by
 ** ID3v2-compliant software.  See the id3v2 standard document for futher
 ** details on unsync.
 **
 ** Be default, tags are created without unsync.
 ** 
 ** \code
 **   myTag.SetUnsync(false);
 ** \endcode
 ** 
 ** \param bSync Whether the tag should be unsynchronized
 **/
bool ID3_Tag::SetUnsync(bool b)
{
  bool changed = __hdr.SetUnsync(b);
  __changed = changed || __changed;
  return changed;
}


/** Turns extended header rendering on or off, dependant on the value of the
 ** boolean parameter.
 ** 
 ** This option is currently ignored as id3lib doesn't yet create extended
 ** headers.  This option only applies when rendering tags for id3v2 versions
 ** that support extended headers.
 **
 ** By default, id3lib will generate extended headers for all tags in which
 ** extended headers are supported.
 ** 
 ** \code
 **   myTag.SetExtendedHeader(true);
 ** \endcode
 ** 
 ** \param bExt Whether to render an extended header
 **/
bool ID3_Tag::SetExtendedHeader(bool ext)
{
  bool changed = __hdr.SetExtended(ext);
  __changed = changed || __changed;
  return changed;
}

/** Turns padding on or off, dependant on the value of the boolean
 ** parameter.
 ** 
 ** When using id3v2 tags in association with files, id3lib can optionally
 ** add padding to the tags to ensure minmal file write times when updating
 ** the tag in the future.
 ** 
 ** When the padding option is switched on, id3lib automatically creates
 ** padding according to the 'ID3v2 Programming Guidelines'.  Specifically,
 ** enough padding will be added to round out the entire file (song plus
 ** tag) to an even multiple of 2K.  Padding will only be created when the
 ** tag is attached to a file and that file is not empty (aside from a
 ** pre-existing tag).
 ** 
 ** id3lib's addition to the guidelines for padding, is that if frames are
 ** removed from a pre-existing tag (or the tag simply shrinks because of
 ** other reasons), the new tag will continue to stay the same size as the
 ** old tag (with padding making the difference of course) until such time as
 ** the padding is greater than 4K.  When this happens, the padding will be
 ** reduced and the new tag will be smaller than the old.
 ** 
 ** By default, padding is switched on.
 ** 
 ** \code
 **   myTag.SetPadding(false);
 ** \endcode
 ** 
 ** \param bPad Whether or not render the tag with padding.
 **/
bool ID3_Tag::SetPadding(bool pad)
{
  bool changed = (__is_padded != pad);
  __changed = changed || __changed;
  if (changed)
  {
    __is_padded = pad;
  }
  
  return changed;
}


ID3_Tag &
ID3_Tag::operator=( const ID3_Tag &rTag )
{
  if (this != &rTag)
  {
    Clear();
    size_t nFrames = rTag.NumFrames();
    for (size_t nIndex = 0; nIndex < nFrames; nIndex++)
    {
      ID3_Frame *frame = new ID3_Frame;
      // Copy the frames in reverse order so that they appear in the same order
      // as the original tag when rendered.
      *frame = *(rTag[nFrames - nIndex - 1]);
      AttachFrame(frame);
    }
  }
  return *this;
}
