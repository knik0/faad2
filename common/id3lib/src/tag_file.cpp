// $Id: tag_file.cpp,v 1.1 2002/01/21 08:16:22 menno Exp $

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
#include <stdio.h>

#if defined WIN32
#  include <windows.h>
static int truncate(const char *path, size_t length)
{
  int result = -1;
  HANDLE fh;
  
  fh = ::CreateFile(path,
                    GENERIC_WRITE | GENERIC_READ,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
  
  if(INVALID_HANDLE_VALUE != fh)
  {
    SetFilePointer(fh, length, NULL, FILE_BEGIN);
    SetEndOfFile(fh);
    CloseHandle(fh);
    result = 0;
  }
  
  return result;
}

// prevents a weird error I was getting compiling this under windows
#  if defined CreateFile
#    undef CreateFile
#  endif

#else
#  include <unistd.h>
#endif

#if defined HAVE_CONFIG_H
#  include <config.h>
#endif

#include "tag.h"

bool exists(const char *name)
{
  bool doesExist = false;
  FILE *in = NULL;
  
  if (NULL == name)
  {
    return false;
  }

  in = fopen(name, "rb");
  doesExist = (NULL != in);
  if (doesExist)
  {
    fclose(in);
  }
    
  return doesExist;
}



ID3_Err ID3_Tag::CreateFile()
{
  CloseFile();

  // Create a new file
  __file_handle = fopen(__file_name, "wb+");

  // Check to see if file could not be created
  if (NULL == __file_handle)
  {
    return ID3E_ReadOnly;
  }

  // Determine the size of the file
  fseek(__file_handle, 0, SEEK_END);
  __file_size = ftell(__file_handle);
  fseek(__file_handle, 0, SEEK_SET);
  
  return ID3E_NoError;
}

ID3_Err ID3_Tag::OpenFileForWriting()
{
  CloseFile();
  __file_size = 0;
  if (exists(__file_name))
  {
    // Try to open the file for reading and writing.
    __file_handle = fopen(__file_name, "rb+");
  }
  else
  {
    return ID3E_NoFile;
  }

  // Check to see if file could not be opened for writing
  if (NULL == __file_handle)
  {
    return ID3E_ReadOnly;
  }

  // Determine the size of the file
  fseek(__file_handle, 0, SEEK_END);
  __file_size = ftell(__file_handle);
  fseek(__file_handle, 0, SEEK_SET);
  
  return ID3E_NoError;
}

ID3_Err ID3_Tag::OpenFileForReading()
{
  CloseFile();
  __file_size = 0;

  __file_handle = fopen(__file_name, "rb");
  
  if (NULL == __file_handle)
  {
    return ID3E_NoFile;
  }

  // Determine the size of the file
  fseek(__file_handle, 0, SEEK_END);
  __file_size = ftell(__file_handle);
  fseek(__file_handle, 0, SEEK_SET);

  return ID3E_NoError;
}

bool ID3_Tag::CloseFile()
{
  bool bReturn = ((NULL != __file_handle) && (0 == fclose(__file_handle)));
  if (bReturn)
  {
    __file_handle = NULL;
  }
  return bReturn;
}

size_t ID3_Tag::Link(const char *fileInfo, bool parseID3v1, bool parseLyrics3)
{
  flags_t tt = ID3TT_NONE;
  if (parseID3v1)
  {
    tt |= ID3TT_ID3V1;
  }
  if (parseLyrics3)
  {
    tt |= ID3TT_LYRICS;
  }
  return this->Link(fileInfo, tt);
}

/** Attaches a file to the tag, parses the file, and adds any tag information
 ** found in the file to the tag.
 ** 
 ** Use this method if you created your ID3_Tag object without supplying a
 ** parameter to the constructor (maybe you created an array of ID3_Tag
 ** pointers).  This is the preferred method of interacting with files, since
 ** id3lib can automatically do things like parse foreign tag formats and
 ** handle padding when linked to a file.  When a tag is linked to a file,
 ** you do not need to use the <a href="#Size">Size</a>, <a
 ** href="#Render">Render</a>, or <a href="#Parse">Parse</a> methods or the
 ** <code>ID3_IsTagHeader</code> function---id3lib will take care of those
 ** details for you.  The single parameter is a pointer to a file name.
 ** 
 ** Link returns a 'luint' which is the byte position within the file that
 ** the audio starts (i.e., where the id3v2 tag ends).
 ** 
 ** \code
 **   ID3_Tag *myTag;
 **   if (myTag = new ID3_Tag)
 **   {
 **     myTag->Link("mysong.mp3");
 **     
 **     // do whatever we want with the tag
 **     // ...
 **   
 **     // setup all our rendering parameters
 **     myTag->SetUnsync(false);
 **     myTag->SetExtendedHeader(true);
 **     myTag->SetCompression(true);
 **     myTag->SetPadding(true);
 **     
 **     // write any changes to the file
 **     myTag->Update()
 **     
 **     // free the tag
 **     delete myTag;
 **   }
 ** \endcode
 ** 
 ** @see ID3_IsTagHeader
 ** @param fileInfo The filename of the file to link to.
 **/
size_t ID3_Tag::Link(const char *fileInfo, flags_t tag_types)
{
  __tags_to_parse.set(tag_types);
  
  if (NULL == fileInfo)
  {
    return 0;
  }

  // if we were attached to some other file then abort
  if (__file_handle != NULL)
  {
    // Log this
    CloseFile();
    //ID3_THROW(ID3E_TagAlreadyAttached);
  }
  
  strcpy(__file_name, fileInfo);
    
  if (ID3E_NoError != OpenFileForReading())
  {
    __starting_bytes = 0;
  }
  else
  {
    __starting_bytes = ParseFromHandle();
    
    CloseFile();
  }
  
  if (__starting_bytes > 0)
  {
    __starting_bytes += ID3_TagHeader::SIZE;
  }

  // the file size represents the file size _without_ the beginning ID3v2 tag
  // info
  __file_size -= MIN(__file_size, __starting_bytes);
  return __starting_bytes;
}

/** Renders the tag and writes it to the attached file; the type of tag
 ** rendered can be specified as a parameter.  The default is to update only
 ** the ID3v2 tag.  See the ID3_TagType enumeration for the constants that
 ** can be used.
 ** 
 ** Make sure the rendering parameters are set before calling the method.
 ** See the Link dcoumentation for an example of this method in use.
 ** 
 ** \sa ID3_TagType
 ** \sa Link
 ** \param tt The type of tag to update.
 **/
flags_t ID3_Tag::Update(flags_t ulTagFlag)
{
  OpenFileForWriting();
  if (NULL == __file_handle)
  {
    CreateFile();
  }
  flags_t tags = ID3TT_NONE;

  if ((ulTagFlag & ID3TT_ID3V2) && HasChanged())
  {
    RenderV2ToHandle();
    tags |= ID3TT_ID3V2;
  }

  if ((ulTagFlag & ID3TT_ID3V1) && 
      (!this->HasTagType(ID3TT_ID3V1) || this->HasChanged()))
  {
    RenderV1ToHandle();
    tags |= ID3TT_ID3V1;
  }
  CloseFile();
  return tags;
}

/** Strips the tag(s) from the attached file. The type of tag stripped
 ** can be specified as a parameter.  The default is to strip all tag types.
 ** 
 ** \param tt The type of tag to strip
 ** \sa ID3_TagType@see
 **/
flags_t ID3_Tag::Strip(flags_t ulTagFlag)
{
  flags_t ulTags = ID3TT_NONE;
  
  if (!(ulTagFlag & ID3TT_ID3V1) && !(ulTagFlag & ID3TT_ID3V2))
  {
    return ulTags;
  }

  // First remove the v2 tag, if requested
  if (ulTagFlag & ID3TT_ID3V2 && __starting_bytes > 0)
  {
    OpenFileForWriting();
    __file_size -= __starting_bytes;

    // We will remove the id3v2 tag in place: since it comes at the beginning
    // of the file, we'll effectively move all the data that comes after the
    // tag back n bytes, where n is the size of the id3v2 tag.  Once we've
    // copied the data, we'll truncate the file.
    //
    // To copy the data, we'll need to keep two "pointers" in the file: one
    // will mark where to read from next, the other will indicate where to 
    // write to. 
    long nNextRead, nNextWrite;
    nNextWrite = ftell(__file_handle);
    // Set the read pointer past the tag
    fseek(__file_handle, __starting_bytes, SEEK_CUR);
    nNextRead = ftell(__file_handle);
    
    uchar aucBuffer[BUFSIZ];
    
    // The nBytesRemaining variable indicates how many bytes are to be copied
    size_t nBytesToCopy = __file_size;

    // Here we reduce the nBytesToCopy by the size of any tags that appear
    // at the end of the file (e.g the id3v1 and lyrics tag).  This isn't
    // strictly necessary, since the truncation stage will remove these,
    // but this check prevents us from copying them unnecessarily.
    if ((__ending_bytes > 0) && (ulTagFlag & ID3TT_ID3V1))
    {
      nBytesToCopy -= __ending_bytes;
    }
    
    // The nBytesRemaining variable indicates how many bytes are left to be 
    // moved in the actual file.
    // The nBytesCopied variable keeps track of how many actual bytes were
    // copied (or moved) so far.
    size_t 
      nBytesRemaining = nBytesToCopy,
      nBytesCopied = 0;
    while (! feof(__file_handle))
    {
      // Move to the next read position
      fseek(__file_handle, nNextRead, SEEK_SET);
      size_t
        nBytesToRead = MIN(nBytesRemaining - nBytesCopied, BUFSIZ),
        nBytesRead   = fread(aucBuffer, 1, nBytesToRead, __file_handle);
      // Now that we've read, mark the current spot as the next spot for
      // reading
      nNextRead = ftell(__file_handle);
      
      if (nBytesRead > 0)
      {
        // Move to the next write position
        fseek(__file_handle, nNextWrite, SEEK_SET);
        size_t nBytesWritten = fwrite(aucBuffer, 1, nBytesRead, __file_handle);
        if (nBytesRead > nBytesWritten)
        {
          // TODO: log this
          //cerr << "--- attempted to write " << nBytesRead << " bytes, "
          //     << "only wrote " << nBytesWritten << endl;
        }
        // Marke the current spot as the next write position
        nNextWrite = ftell(__file_handle);
        nBytesCopied += nBytesWritten;
      }
      
      if (nBytesCopied == nBytesToCopy)
      {
        break;
      }
      if (nBytesToRead < BUFSIZ)
      {
        break;
      }
    }
    CloseFile();
  }
  
  size_t nNewFileSize = __file_size;

  if ((__ending_bytes > 0) && (ulTagFlag & ID3TT_ID3V1))
  {
    // if we're stripping the ID3v1 tag, be sure to reduce the file size by
    // those bytes
    nNewFileSize -= __ending_bytes;
    ulTags |= ID3TT_ID3V1;
  }
  
  if ((ulTagFlag & ID3TT_ID3V2) && (__starting_bytes > 0))
  {
    // If we're stripping the ID3v2 tag, there's no need to adjust the new
    // file size, since it doesn't account for the ID3v2 tag size
    ulTags |= ID3TT_ID3V2;
  }
  else
  {
    // add the original ID3v2 tag size since we don't want to delete it, and
    // the new file size represents the file size _not_ counting the ID3v2
    // tag
    nNewFileSize += __starting_bytes;
  }

  if (ulTags && (truncate(__file_name, nNewFileSize) == -1))
  {
    ID3_THROW(ID3E_NoFile);
  }

  __starting_bytes = (ulTags & ID3TT_ID3V2) ? 0 : __starting_bytes;
  __ending_bytes -= (ulTags & ID3TT_ID3V1) ? MIN(__ending_bytes, ID3_V1_LEN) : 0;
      
  __changed = __file_tags.remove(ulTags) || __changed;
  
  return ulTags;
}
