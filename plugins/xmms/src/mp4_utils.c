/*
 * some functions for MP4 files
*/

#include "mp4ff.h"
#include "faad.h"

#include <stdio.h>
const char *mp4AudioNames[]=
  {
    "MPEG-1 Audio Layers 1,2 or 3",
    "MPEG-2 low biterate (MPEG-1 extension) - MP3",
    "MPEG-2 AAC Main Profile",
    "MPEG-2 AAC Low Complexity profile",
    "MPEG-2 AAC SSR profile",
    "MPEG-4 audio (MPEG-4 AAC)",
    0
  };

/* MPEG-4 Audio types from 14496-3 Table 1.5.1 (from mp4.h)*/
const char *mpeg4AudioNames[]=
  {
    "!!!!MPEG-4 Audio track Invalid !!!!!!!",
    "MPEG-4 AAC Main profile",
    "MPEG-4 AAC Low Complexity profile",
    "MPEG-4 AAC SSR profile",
    "MPEG-4 AAC Long Term Prediction profile",
    "MPEG-4 AAC Scalable",
    "MPEG-4 CELP",
    "MPEG-4 HVXC",
    "MPEG-4 Text To Speech",
    "MPEG-4 Main Synthetic profile",
    "MPEG-4 Wavetable Synthesis profile",
    "MPEG-4 MIDI Profile",
    "MPEG-4 Algorithmic Synthesis and Audio FX profile"
  };

/*
 * find AAC track
*/

int getAACTrack(mp4ff_t *infile)
{
  int i, rc;
  int numTracks = mp4ff_total_tracks(infile);

  printf("total-tracks: %d\n", numTracks);
  for(i=0; i<numTracks; i++){
    unsigned char*	buff = 0;
    int			buff_size = 0;
    mp4AudioSpecificConfig mp4ASC;

    printf("testing-track: %d\n", i);
    mp4ff_get_decoder_config(infile, i, &buff, &buff_size);
    if(buff){
      rc = NeAACDecAudioSpecificConfig(buff, buff_size, &mp4ASC);
      free(buff);
      if(rc < 0)
	continue;
      return(i);
    }
  }
  return(-1);
}


void getMP4info(char* file)
{
  /*
  MP4FileHandle	mp4file;
  MP4Duration	trackDuration;
  int numTracks;
  int i=0;

  if(!(mp4file = MP4Read(file,0)))
    return;
  //MP4Dump(mp4file, 0, 0);
  numTracks = MP4GetNumberOfTracks(mp4file, NULL, 0);
  g_print("there are %d track(s)\n", numTracks);
  for(i=0;i<numTracks;i++){
    MP4TrackId trackID = MP4FindTrackId(mp4file, i, NULL, 0);
    const char *trackType = MP4GetTrackType(mp4file, trackID);
    printf("Track %d, %s", trackID, trackType);
    if(!strcmp(trackType, MP4_AUDIO_TRACK_TYPE)){//we found audio track !
      int j=0;
      u_int8_t audiotype = MP4GetTrackAudioType(mp4file, trackID);
      while(mp4AudioTypes[j]){ // what kind of audio is ?
	if(mp4AudioTypes[j] == audiotype){
	  if(mp4AudioTypes[j] == MP4_MPEG4_AUDIO_TYPE){
	    audiotype = MP4GetTrackAudioMpeg4Type(mp4file, trackID);
	    g_print(" %s", mpeg4AudioNames[audiotype]);
	  }
	  else{
	    printf(" %s", mp4AudioNames[j]);
	  }
	  g_print(" duration :%d",
		 MP4ConvertFromTrackDuration(mp4file, trackID,
					     MP4GetTrackDuration(mp4file,
								 trackID),
					     MP4_MSECS_TIME_SCALE));
	}
	j++;
      }
    }
    printf("\n");
  }
  MP4Close(mp4file);
  */
}
