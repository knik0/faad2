/*
** a mp4 AAC audio player for XMMS by ciberfred
**
*/

#include "mp4.h"
#include <pthread.h>
#include <glib.h>
#include <gtk/gtk.h>


#include "faad.h"

#include "xmms/plugin.h"
#include "xmms/util.h"
#include "xmms/configfile.h"
#include "xmms/titlestring.h"

#include "libmp4_utils.h"

#define MP4_DESCRIPTION	"MP4 audio player - 1.2.7"
#define MP4_VERSION	"mp4 audio player (v0.3) - 1 Aout 2003"
#define MP4_ABOUT	"Written from scratch by ciberfred"

static void	mp4_init(void);
static void	mp4_about(void);
static void	mp4_play(char *);
static void	mp4_stop(void);
static void	mp4_pause(short);
static void	mp4_seek(int);
static int	mp4_getTime(void);
static void	mp4_cleanup(void);
static void	mp4_getSongInfo(char *);
static int	mp4_isFile(char *);

static void	*mp4Decode(void *args);

InputPlugin mp4_ip =
  {
    0,	// handle
    0,	// filename
    MP4_DESCRIPTION,
    mp4_init,
    mp4_about,
    0,	// configuration
    mp4_isFile,
    0,	//scandir
    mp4_play,
    mp4_stop,
    mp4_pause,
    mp4_seek,
    0,	// set equalizer
    mp4_getTime,
    0,	// get volume
    0,
    mp4_cleanup,
    0,	// obsolete
    0,	// send visualisation data
    0,	// set player window info
    0,	// set song title text
    0,	// get song title text
    mp4_getSongInfo, // info box
    0,	// to output plugin
  };


static gboolean		bPlaying = FALSE;
static pthread_t	decodeThread;
static pthread_mutex_t	mutex = PTHREAD_MUTEX_INITIALIZER;
static int		seekPosition = -1;

InputPlugin *get_iplugin_info(void)
{
  return(&mp4_ip);
}

static void mp4_init(void)
{
  memset(&decodeThread, 0, sizeof(pthread_t));
  return;
}

static void mp4_play(char *filename)
{
  bPlaying = TRUE;
  pthread_create(&decodeThread, 0, mp4Decode, g_strdup(filename));
  return;
}

static void mp4_stop(void)
{
  if(bPlaying){
    bPlaying = FALSE;
    pthread_join(decodeThread, NULL);
    memset(&decodeThread, 0, sizeof(pthread_t));
    mp4_ip.output->close_audio();
  }
}

static void *mp4Decode(void *args)
{
  unsigned long msDuration;
  unsigned char	*buffer = NULL;
  int		bufferSize = 0;
  int		lastframe = 0;
  faacDecHandle decoder;
  MP4SampleId	numSamples;
  MP4SampleId	sampleID=1;
  MP4FileHandle mp4file;
  MP4Duration	duration;
  int		mp4track;
  unsigned long	samplerate;
  unsigned char channels;
  unsigned int	avgBitrate;


  pthread_mutex_lock(&mutex);
  seekPosition = -1;
  bPlaying = TRUE;
  if(!(mp4file = MP4Read(args, 0))){
    printf("can't open file: %s\n", args);
    free(args);
    bPlaying = FALSE;
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
  }
  if((mp4track = getAACTrack(mp4file)) < 0){
// check here for others Audio format.....

    g_print("Unsupported Audio track type\n");
    free(args);
    MP4Close(mp4file);
    bPlaying = FALSE;
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
  }
  decoder = faacDecOpen();
  MP4GetTrackESConfiguration(mp4file, mp4track, &buffer, &bufferSize);
  if(!buffer){
    free(args);
    faacDecClose(decoder);
    MP4Close(mp4file);
    bPlaying = FALSE;
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
  }
  if(faacDecInit2(decoder, buffer, bufferSize, &samplerate, &channels)<0){
    free(args);
    faacDecClose(decoder);
    MP4Close(mp4file);
    bPlaying = FALSE;
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
  }
  free(buffer);
  if(channels == 0){
    g_print("Number of Channels not supported\n");
    free(args);
    faacDecClose(decoder);
    MP4Close(mp4file);
    bPlaying = FALSE;
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
  }
  mp4_ip.output->open_audio(FMT_S16_NE, samplerate, channels);
  mp4_ip.output->flush(0);
  g_print("MP4 flush\n");
  duration = MP4GetTrackDuration(mp4file, mp4track);
  msDuration = MP4ConvertFromTrackDuration(mp4file, mp4track, duration,
					   MP4_MSECS_TIME_SCALE);
  numSamples = MP4GetTrackNumberOfSamples(mp4file, mp4track);
  //mp4_ip.set_info(args, msDuration, -1, samplerate/1000, channels);

  while(bPlaying){
    void		*sampleBuffer;
    faacDecFrameInfo	frameInfo;    
    int			rc;

    if(seekPosition!=-1){
      duration = MP4ConvertToTrackDuration(mp4file, mp4track,seekPosition*1000,
					   MP4_MSECS_TIME_SCALE);
      sampleID = MP4GetSampleIdFromTime(mp4file, mp4track, duration, 0);
      mp4_ip.output->flush(seekPosition*1000);
      seekPosition = -1;
    }
    buffer=NULL;
    bufferSize=0;
    if(sampleID > numSamples){
      mp4_ip.output->close_audio();
      free(args);
      faacDecClose(decoder);
      MP4Close(mp4file);
      bPlaying = FALSE;
      pthread_mutex_unlock(&mutex);
      pthread_exit(NULL);
    }
    rc = MP4ReadSample(mp4file, mp4track, sampleID++, &buffer, &bufferSize,
		       NULL, NULL, NULL, NULL);
    //g_print("%d/%d\n", sampleID-1, numSamples);
    if((rc==0) || (buffer== NULL)){
      printf("read error\n");
      sampleBuffer = NULL;
      sampleID=0;
      mp4_ip.output->buffer_free();
      mp4_ip.output->close_audio();
      free(args);
      faacDecClose(decoder);
      MP4Close(mp4file);
      bPlaying = FALSE;
      pthread_mutex_unlock(&mutex);
      pthread_exit(NULL);
    }
    else{
      sampleBuffer = faacDecDecode(decoder, &frameInfo, buffer, bufferSize);
      if(frameInfo.error > 0){
	printf("FAAD2 error : %s\n", faacDecGetErrorMessage(frameInfo.error));
	mp4_ip.output->close_audio();
	free(args);
	faacDecClose(decoder);
	MP4Close(mp4file);
	bPlaying = FALSE;
	pthread_mutex_unlock(&mutex);
	pthread_exit(NULL);
      }
      if(buffer){
	free(buffer); buffer=0;
      }
      while(bPlaying && mp4_ip.output->buffer_free()<frameInfo.samples<<1)
	xmms_usleep(30000);
    }
    //mp4_ip.add_vis_pcm(mp4_ip.output->written_time(), FMT_S16_NE, channels, frameInfo.samples<<1, sampleBuffer);
    mp4_ip.output->write_audio(sampleBuffer, frameInfo.samples<<1);
  }
  while(bPlaying && mp4_ip.output->buffer_free()){
    xmms_usleep(10000);
  }
  mp4_ip.output->close_audio();
  free(args);
  faacDecClose(decoder);
  MP4Close(mp4file);
  bPlaying = FALSE;
  pthread_mutex_unlock(&mutex);
  pthread_exit(NULL);
}

static int	mp4_isFile(char *filename)
{
  char *extention = strrchr(filename, '.');
  if (extention &&
      !strcasecmp(extention, ".mp4") ||
      !strcasecmp(extention, ".m4a")
      ){
    return (1);
  }
  return(0);
}

static void	mp4_about(void)
{
  static GtkWidget *aboutbox;

  if(aboutbox!=NULL)
    return;

  aboutbox = xmms_show_message(
			       "About MP4 AAC player plugin",
			       "decoding engine : FAAD2 (30 July 2003)\n"
			       "plugin version : 0.3 (ciberfred)/static",
			       "Ok", FALSE, NULL, NULL);
  gtk_signal_connect(GTK_OBJECT(aboutbox), "destroy",
                     GTK_SIGNAL_FUNC(gtk_widget_destroyed),
                     &aboutbox);
}

static void	mp4_pause(short flag)
{
  mp4_ip.output->pause(flag);
}

static void	mp4_seek(int time)
{
  seekPosition = time;
  while(bPlaying && seekPosition!=-1) xmms_usleep(10000);
}

static int	mp4_getTime(void)
{
  if (!bPlaying){
    return (-1);
  }
  else{
    return (mp4_ip.output->output_time());
  }
}

static void	mp4_cleanup(void)
{
}

static void	mp4_getSongInfo(char *filename)
{
  getMP4info(filename);
}
