/*
** function to read id3tag from aac files
*/

#include <id3/tag.h>
#include <id3/utils.h>
#include <id3/misc_support.h>
#include <id3/readers.h>
#include <stdio.h>

// this is to show the cpp functions to the C interface...
#ifdef __cplusplus
extern "C" {
#endif

void	readID3tag(char*);
void	clearWindowDatas(void);
extern char	*title;
extern char	*artist;
extern char	*album;
extern char	*year;
extern char	*track;
extern char	*genre;
extern char	*comment;
extern char	*composer;
extern char	*url;
extern char	*originalArtist;
extern char	*encodedby;

#ifdef __cplusplus
}
#endif


void readID3tag(char *filename)
{
  ID3_Tag tag;

  tag.Link(filename, ID3TT_ALL);
  ID3_Tag::Iterator	*iter = tag.CreateIterator();
  ID3_Frame		*frame = NULL;

  while((frame = iter->GetNext()) != NULL){
    ID3_FrameID FrameID = frame->GetID();
    switch (FrameID)
      {
      case ID3FID_TITLE:
	{
	  title = ID3_GetString(frame, ID3FN_TEXT);
	  break;
	}
      case ID3FID_LEADARTIST:
	{
	  artist = ID3_GetString(frame, ID3FN_TEXT);
	  break;
	}
      case ID3FID_ALBUM:
	{
	  album = ID3_GetString(frame, ID3FN_TEXT);
	  break;
	}
      case ID3FID_YEAR:
	{
	  year = ID3_GetString(frame, ID3FN_TEXT);
	  break;
	}
      case ID3FID_TRACKNUM:
	{
	  track = ID3_GetString(frame, ID3FN_TEXT);
	  break;
	}
      case ID3FID_CONTENTTYPE:
	{
	  genre = ID3_GetString(frame, ID3FN_TEXT);
	  break;
	}
      case ID3FID_COMMENT:
	{
	  comment = ID3_GetString(frame, ID3FN_TEXT);
	  break;
	}
      case ID3FID_COMPOSER:
	{
	  composer = ID3_GetString(frame, ID3FN_TEXT);
	  break;
	}
      case ID3FID_WWWARTIST:
	{
	  url = ID3_GetString(frame, ID3FN_URL);
		break;
	}
      case ID3FID_ORIGARTIST:
	{
	  originalArtist = ID3_GetString(frame, ID3FN_TEXT);
	  break;
		}
      case ID3FID_ENCODEDBY:
	{
	  encodedby = ID3_GetString(frame, ID3FN_TEXT);
	  break;
	}
      default:
	break;
	}
  }
  delete iter;
  return;
}

void clearWindowDatas(void)
{
  if(title)
    if(strcmp(title,"")!=0){
      delete [] title;
    }
  if(artist)
    if(strcmp(artist,"")!=0){
      delete [] artist;
    }
  if(album)
    if(strcmp(album,"")!=0){
      delete [] album;
    }
  if(year)
  if(strcmp(year,"")!=0){
    delete [] year;
  }
  if(track)
    if(strcmp(track,"")!=0){
      delete [] track;
    }
  if(genre)
    if(strcmp(genre,"")!=0){
      delete [] genre;
 }
  if(comment)
    if(strcmp(comment,"")!=0){
      delete [] comment;
    }
  if(composer)
  if(strcmp(composer,"")!=0){
    delete [] composer;
 }
  if(url)
    if(strcmp(url,"")!=0){
      delete [] url;
    }
  if(originalArtist)
    if(strcmp(originalArtist,"")!=0){
      delete [] originalArtist;
 }
  if(encodedby)
  if(strcmp(encodedby,"")!=0){
    delete [] encodedby;
  }
  title=artist=album=year=track=genre=comment=composer=url=originalArtist=encodedby=0;
}
