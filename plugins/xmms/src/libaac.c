/*
**			  AAC plugin for XMMS 1.2.7
**				by ciberfred
**		------------------------------------------------
**
** need libfaad2     package from http://www.audiocoding.com
** and id3lib-3.8.x  package from http://id3lib.sourceforge.org
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "faad.h"
#include "xmms/plugin.h"
#include "xmms/util.h"
#include "xmms/configfile.h"
#include "xmms/titlestring.h"
#include "aac_utils.h"

#define AAC_DESCRIPTION	"MPEG2/4 AAC player - 1.2.7"
#define AAC_VERSION	"AAC player - 15 June 2003 (v0.4)"
#define AAC_ABOUT	"Writen from scratch by ciberfred from France\n"
#define PATH2CONFIGFILE "/.xmms/Plugins/aacConfig.txt"
#define BUFFER_SIZE	FAAD_MIN_STREAMSIZE*64

static void	aac_init(void);
static void	aac_play(char*);
static void	aac_stop(void);
static void	aac_pause(short);
static int	aac_getTime(void);
static void	aac_seek(int);
static void	aac_cleanup(void);
static void	aac_about(void);
static void	aac_configuration(void);
static void	*aac_decode(void*);

static void	aac_getSongInfo(char*);
static int	aac_isFile(char*);

extern void	readID3tag(char*);
extern GtkWidget *createDialogInfo(void);
extern void	clearWindowDatas(void);

static GtkWidget *infoBoxWindow = NULL;
extern char *title, *artist, *album, *track, *genre;


/*****************************************************************************/
/*
** struct need by xmms for Library Interface
*/

InputPlugin aac_ip =
  {
    0,		// handle
    0,		// filename
    AAC_DESCRIPTION,// description
    aac_init,	// init_func
    aac_about,	// aboutbox
    aac_configuration,		// configuration
    aac_isFile,	// ???
    0,		// scan dir
    aac_play,	// when play button
    aac_stop,	// when stop
    aac_pause,	// when pause
    aac_seek,	// when seek
    0,		// set equalizer
    aac_getTime,	// ???
    0,		// get volume
    0,		// set volume
    aac_cleanup,	// the cleanup function :)
    0,		// obsolete (???)
    0,		// send visualisation data
    0,		// set player window info
    0,		// set song title text
    0,	// get song title text to show on Playlist
    aac_getSongInfo,// file info box
    0		// pointer to outputPlugin
  };
static gboolean 	bPlaying = FALSE;
static gboolean		bOutputOpen = FALSE;
static pthread_t	decodeThread;
static gboolean		bSeek = FALSE;
static gint		seekPosition = -1; // track position
static pthread_mutex_t	mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned long	*positionTable = 0;
int			aacType;
/*****************************************************************************/

InputPlugin *get_iplugin_info(void)
{
  return (&aac_ip);
}


static void aac_init(void)
{
  ConfigFile*	cfg;

  memset(&decodeThread, 0, sizeof(pthread_t));
  cfg = xmms_cfg_open_default_file();
  xmms_cfg_read_boolean(cfg, "AAC", "seeking", &bSeek);
  xmms_cfg_free(cfg);
}

static void aac_cleanup(void)
{
  memset(&decodeThread, 0, sizeof(pthread_t));
  if(positionTable){
    free(positionTable);
  }

}

static void aac_play(char *filename)
{
  bPlaying = TRUE;
  if(pthread_create(&decodeThread, 0, aac_decode, g_strdup(filename))!=0){
    printf("Error creating pthread, can't play file\n");
  }

  return;
}

static void aac_stop(void)
{
  if (bPlaying){
    bPlaying = FALSE;
    pthread_join(decodeThread, NULL);
    memset(&decodeThread, 0, sizeof(pthread_t));
    aac_ip.output->close_audio();
    clearWindowDatas();
  }
}

static void aac_pause(short paused)
{
  if(bOutputOpen){
    aac_ip.output->pause(paused);
  }
}

static int aac_getTime(void)
{
  if (!bPlaying){
    return (-1);
  }
  else{
    return (aac_ip.output->output_time());
  }
}

static void aac_seek(int time)
{
  seekPosition=time;
  while(bPlaying && seekPosition!=-1) xmms_usleep(10000);
}

static void aac_getSongInfo(char *filename)
{
  infoBoxWindow = createDialogInfo();
  gtk_widget_show(infoBoxWindow);
}

static void *aac_decode(void *args)
{
  char 			*filename = args;
  char			*xmmstitle=NULL;
  FILE 			*file = NULL;
  faacDecHandle		decoder = 0;
  unsigned char		*buffer = 0;
  unsigned long		bufferconsumed = 0;
  unsigned long		samplerate = 0;
  unsigned long		lenght=0;
  char			channels;
  unsigned long		buffervalid = 0;
  TitleInput		*input;
  char 			*temp = g_strdup(filename);
  char			*ext  = strrchr(temp, '.');

  pthread_mutex_lock(&mutex);
  seekPosition=-1;
  clearWindowDatas();
  if((file = fopen(filename, "rb")) == 0){
    printf("can't find file %s\n", filename);
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
  }
  if(bSeek){
    checkADTSForSeeking(file, &positionTable, &lenght);
    if((aacType = getAacInfo(file)) ==-1){
      g_print("erreur getAAC\n");
      fclose(file);
      if(positionTable){
	free(positionTable); positionTable=0;
      }
      pthread_mutex_unlock(&mutex);
      pthread_exit(NULL);
    }
  }
  if((decoder = faacDecOpen()) == NULL){
    printf("Open Decoder Error\n");
    fclose(file);
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
  }
  if((buffer = g_malloc(BUFFER_SIZE)) == NULL){
    printf("error g_malloc\n");
    fclose(file);
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
  }

  if((buffervalid = fread(buffer, 1, BUFFER_SIZE, file))==0){
    printf("Error file NULL\n");
    g_free(buffer);
    fclose(file);
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
  }
  XMMS_NEW_TITLEINPUT(input);
  input->file_name = g_basename(temp);
  input->file_ext = ext ? ext+1 : NULL;
  input->file_path = temp;
  if(!strncmp(buffer, "ID3", 3)){
    int size = 0;

    readID3tag(filename);
    if(title)
      input->track_name = g_strdup(title);
    if(artist)
      input->performer = g_strdup(artist);
    if(genre)
      input->genre = g_strdup(genre);
    if(track)
      input->track_number = atoi(track);
    fseek(file, 0, SEEK_SET);
/*
** hum .. horrible hack taken from the winamp plugin to jump
** the tag, is there any id3 function to do this ???? hum... seems not :(
*/
    size = (buffer[6]<<21) | (buffer[7]<<14) | (buffer[8]<<7) | buffer[9];
    size+=10;
    fread(buffer, 1, size, file);
    if(bSeek){
      checkADTSForSeeking(file, &positionTable, &lenght);
      if((aacType = getAacInfo(file)) ==-1){
	printf("erreur getAAC\n");
	g_free(buffer); buffer=0;
	faacDecClose(decoder);
	fclose(file);
	aac_ip.output->close_audio();
	if(positionTable){
	  free(positionTable); positionTable=0;
	}
	pthread_mutex_unlock(&mutex);
	pthread_exit(NULL);
      }
      printf("AAC-%s Type\n", aacType?"MPEG2":"MPEG4");
    }
    buffervalid = fread(buffer, 1, BUFFER_SIZE, file);
  }
  xmmstitle = xmms_get_titlestring(xmms_get_gentitle_format(), input);
  if(xmmstitle == NULL)
    xmmstitle = g_strdup(input->file_name);
  g_free(temp);
  g_free(input->performer);
  g_free(input->album_name);
  g_free(input->track_name);
  g_free(input->genre);
  g_free(input);

  bufferconsumed = faacDecInit(decoder, buffer, &samplerate, &channels);
  if((bOutputOpen = aac_ip.output->open_audio(FMT_S16_NE, samplerate, channels)) == FALSE){
    printf("Output Error\n");
    g_free(buffer); buffer=0;
    faacDecClose(decoder);
    fclose(file);
    aac_ip.output->close_audio();
    if(positionTable){
      free(positionTable); positionTable=0;
    }
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
  }
  if(bSeek){
    aac_ip.set_info(xmmstitle, lenght*1000, -1, samplerate, channels);
  }else{
    aac_ip.set_info(xmmstitle, -1, -1, samplerate, channels);
  }
  aac_ip.output->flush(0);  
  while(bPlaying && buffervalid > 0){
    faacDecFrameInfo	finfo;
    unsigned long		samplesdecoded;
    char			*sample_buffer = NULL;

    if(bSeek && seekPosition!=-1){
      fseek(file, positionTable[seekPosition], SEEK_SET);
      bufferconsumed=0;
      buffervalid = fread(buffer, 1, BUFFER_SIZE, file);
      aac_ip.output->flush(seekPosition*1000);
      seekPosition=-1;
    }
    if(bufferconsumed > 0){
      memmove(buffer, &buffer[bufferconsumed], buffervalid-bufferconsumed);
      buffervalid -= bufferconsumed;
      buffervalid += fread(&buffer[buffervalid], 1, BUFFER_SIZE-buffervalid, file);
      bufferconsumed = 0;
    }
    sample_buffer = faacDecDecode(decoder, &finfo, buffer);
    if(finfo.error){
      buffervalid = 0;
      printf("FAAD2 Error %s\n", faacDecGetErrorMessage(finfo.error));
      printf("---Use Psystrip.exe on the file to avoid the ADTS error---\n");
    }
    bufferconsumed += finfo.bytesconsumed;
    samplesdecoded = finfo.samples;
    if((samplesdecoded<=0) && !sample_buffer){
      printf("error\n");
    }
    while(bPlaying && aac_ip.output->buffer_free() < (samplesdecoded<<1)){
      xmms_usleep(10000);
    }
    aac_ip.add_vis_pcm(aac_ip.output->written_time(), FMT_S16_LE, channels, samplesdecoded<<1, sample_buffer);
    aac_ip.output->write_audio(sample_buffer, samplesdecoded<<1);
  }
  while(bPlaying && aac_ip.output->buffer_playing()){
    xmms_usleep(10000);
  }
  aac_ip.output->buffer_free();
  aac_ip.output->close_audio();
  bPlaying = FALSE;
  bOutputOpen = FALSE;
  g_free(buffer);
  faacDecClose(decoder);
  g_free(xmmstitle);
  fclose(file);
  seekPosition = -1;
  if(positionTable){
    free(positionTable); positionTable=0;
  }
  pthread_mutex_unlock(&mutex);
  pthread_exit(NULL);
}

static int aac_isFile(char *filename)
{
  char *extention = strrchr(filename, '.');
  if (extention && !strcasecmp(extention, ".aac")){
    return (1);
  }
  return(0);
}

static void aac_about(void)
{
  static GtkWidget	*aboutbox;

  if(aboutbox!=NULL)
    return;

  aboutbox = xmms_show_message(
		 "About MPEG2/4-AAC plugin",
		 "decoding engine : FAAD2 team\n"
		 "Plugin coder : ciberfred",
 	         "Ok", FALSE, NULL, NULL);
  gtk_signal_connect(GTK_OBJECT(aboutbox), "destroy",
		     GTK_SIGNAL_FUNC(gtk_widget_destroyed),
		     &aboutbox);
}


static  GtkWidget *checkbutton;
static  GtkWidget *window;
static void aac_configuration_save(GtkWidget *widget, gpointer data)
{
  ConfigFile	*cfg;
  bSeek = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton));
  cfg = xmms_cfg_open_default_file();
  xmms_cfg_write_boolean(cfg, "AAC", "seeking", bSeek);
  xmms_cfg_free(cfg);
  gtk_widget_destroy(window);
}

static void	aac_configuration(void)
{
  GtkWidget *vbox, *hbox;
  GtkWidget *NotaBene;
  GtkWidget *button2;

  window = gtk_window_new(GTK_WINDOW_DIALOG);
  gtk_signal_connect(GTK_OBJECT(window), "destroy",
		     GTK_SIGNAL_FUNC(gtk_widget_destroyed),
		     &window);
  gtk_window_set_title(GTK_WINDOW(window), "AAC Plugin Configuration");
  gtk_widget_set_usize(window, 220, 200);

  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  NotaBene = gtk_text_new(NULL, NULL);
  GTK_WIDGET_UNSET_FLAGS (NotaBene, GTK_CAN_FOCUS);
  gtk_text_insert(GTK_TEXT(NotaBene), NULL, NULL, NULL,
                   "Remember that unable seeking"
		   " is not suitable for playing"
		   " file from network.\n"
		   "Seeking must read first all aac file before playing.",-1);
  gtk_box_pack_start(GTK_BOX(vbox), NotaBene, FALSE, FALSE, 0);

  checkbutton = gtk_check_button_new_with_label("Unable Seeking");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), bSeek);
  gtk_box_pack_start(GTK_BOX(vbox), checkbutton, FALSE, FALSE, 0);

  hbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  button2 = gtk_button_new_with_label ("Save");
  gtk_signal_connect_object(GTK_OBJECT(button2), "clicked",
			    GTK_SIGNAL_FUNC(aac_configuration_save),
			    GTK_OBJECT(window));
  gtk_box_pack_start(GTK_BOX(hbox), button2, FALSE, FALSE, 0);
  button2 = gtk_button_new_with_label ("Close");
  gtk_signal_connect_object(GTK_OBJECT(button2), "clicked",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(window));
  gtk_box_pack_start(GTK_BOX(hbox), button2, FALSE, FALSE, 0);
  gtk_widget_show_all(window);
}

