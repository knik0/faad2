/*
**			  AAC plugin for XMMS 1.2.7
**				by ciberfred
**		------------------------------------------------
** The version of the plugin match the version of XMMS
** for identifie different version use the date :)
**
**			version 1.2.7 (23 august 2002)
**
**
**	       need faad2 package from http://www.audiocoding.com
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

#define AAC_DESCRIPTION	"MPEG2/4 AAC player - 1.2.7"
#define AAC_VERSION	"AAC player - 23 Agust 2002"
#define AAC_ABOUT	"Writen from scratch by ciberfred from France"
#define BUFFER_SIZE	FAAD_MIN_STREAMSIZE*64

static void aac_init(void);
static void aac_play(char*);
static void aac_stop(void);
static void aac_pause(short);
static int  aac_getTime(void);
static void aac_seek(int);
static void aac_cleanup(void);
static void aac_about(void);
static void *aac_decode(void*);

static void aac_getSongInfo(char*);
static int  aac_isFile(char*);

extern void readID3tag(char*);
extern GtkWidget *createDialogInfo(void);
static GtkWidget *infoBoxWindow = NULL;
extern char *title, *artist, *album, *track, *genre;
extern void clearWindowDatas(void);
/******************************************************************************/
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
	0,		// configuration
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
static gboolean	bOutputOpen = FALSE;
static pthread_t	decodeThread;
static gint		seek_pos = -1; // the track position
static pthread_mutex_t	mutex = PTHREAD_MUTEX_INITIALIZER;

/******************************************************************************/

InputPlugin	*get_iplugin_info(void)
{
	return (&aac_ip);
}

static void aac_init(void)
{
	memset(&decodeThread, 0, sizeof(pthread_t));
}

static void aac_cleanup(void)
{
}

static void aac_play(char *filename)
{
	printf("play\n");
	bPlaying = TRUE;
	pthread_create(&decodeThread, 0, aac_decode, g_strdup(filename));
	return;
}

static void aac_stop(void)
{
	printf("stop\n");
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
	printf("pause\n");
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
	printf("seek\n");
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
	char			channels;
	unsigned long		buffervalid = 0;
	TitleInput		*input;
	char 			*temp = g_strdup(filename);
	char			*ext  = strrchr(temp, '.');

	printf("decoding...\n");
	pthread_mutex_lock(&mutex);
	clearWindowDatas();
	if ((file = fopen(filename, "rb")) == 0){
		printf("can't find file %s\n", filename);
		pthread_mutex_unlock(&mutex);
		pthread_exit(NULL);
	}
	if ((decoder = faacDecOpen()) == NULL){
		printf("Open Decoder Error\n");
		fclose(file);
		pthread_mutex_unlock(&mutex);
		pthread_exit(NULL);
	}
	if ((buffer = g_malloc(BUFFER_SIZE)) == NULL)
		printf("error g_malloc\n");
	buffervalid = fread(buffer, 1, BUFFER_SIZE, file);
// check for ID3 tag
		XMMS_NEW_TITLEINPUT(input);
		input->file_name = g_basename(temp);
		input->file_ext = ext ? ext+1 : NULL;
		input->file_path = temp;
	if (!strncmp(buffer, "ID3", 3)){
		int size = 0;

		printf("Song with a ID3Tagv2\n");
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
//	printf("song with %d channels at %d Hz\n", channels, samplerate);
	if((bOutputOpen = aac_ip.output->open_audio(FMT_S16_NE, samplerate, channels)) == FALSE){
		printf("Output Error\n");
		g_free(buffer);
		buffer=0;
		faacDecClose(decoder);
		fclose(file);
		aac_ip.output->close_audio();
		pthread_mutex_unlock(&mutex);
		pthread_exit(NULL);
	}
	aac_ip.set_info(xmmstitle, -1, -1, samplerate, channels);
	aac_ip.output->flush(0);

	while(bPlaying && buffervalid > 0){
		faacDecFrameInfo	finfo;
		unsigned long		samplesdecoded;
		char			*sample_buffer = NULL;

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
//	aac_ip.output->flush(0);
	aac_ip.output->buffer_free();
	aac_ip.output->close_audio();
	bPlaying = FALSE;
	bOutputOpen = FALSE;
	g_free(buffer);
	faacDecClose(decoder);
	g_free(xmmstitle);
	fclose(file);
	printf("...ended\n");
	seek_pos = -1;
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
	GtkWidget *dialog, *button, *label, *label2;

	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), "About FAAD2 plugin");
	label = gtk_label_new(AAC_ABOUT);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, TRUE, TRUE, 0);
	gtk_widget_show(label);
	label2 = gtk_label_new(AAC_VERSION);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label2, TRUE, TRUE, 0);
	gtk_widget_show(label2);
	button = gtk_button_new_with_label("close");
	gtk_signal_connect_object(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, FALSE, FALSE, 0);
	gtk_widget_show(button);
	gtk_widget_show(dialog);
}
