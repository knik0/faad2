/* aac-XMMS.c
 *
 * XMMS input plugin that decodes AAC files.
 *
 * Copyright (C) 2001 Albert Fong
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */


/*
 * This plugin depends on XMMS 1.2.5+, faad2-26012002, id3lib-3.8.0pre2
 * zlib, and at least glib-1.2.
 *
 * XMMS:	www.xmms.org
 * FAAC/FAAD:	www.audiocoding.com
 * id3lib:	id3lib.sourceforge.net
 *
 * Planned updates:
 *
 * 	Configuration panel in XMMS.
 *
 * 	File info panel in XMMS.
 *
 * 	Allow the buffer size to be customized, or buffer the entire 
 * 	file before decoding.
 *
 * 	Choose to enable or disable seeking.  Currently, the plugin needs
 * 	to build a position table first before seeking is possible.  This
 * 	requires an initial pass through the file before decoding.  This
 * 	is not desirable if the file is streamed across the network or from
 * 	slow storage media.
 *
 * 	Choose to enable or disable play time calculation in the playlist.
 * 	Again, this requires an initial pass to calculate the play time.
 *
 * 	VBR calculation.
 *
 * 	Handle unicode ID3 tags properly.
 * 
 *  A more efficient method of handling streaming files, seeking, and play
 *  time calculation is desirable.
 */

/* Update:  February 7, 2002
 *
 * 	Updated to remove id3lib modifications, and general clean up for
 * 	release.
 */

/* Update:  January 26, 2002
 *
 *	Updated to work with January 26 source release for FAAC/FAAD, to
 *	use libfaad2. (www.audiocoding.com, faac.sourceforge.net)
 */

/* Set to 0 if you want to disable seeking.  Useful when streaming files
 * off of a network drive, so you don't have to download the entire file
 * to construct the seek_table before playing
 */
#define USE_SEEKING	1

#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "faad.h"
#include "xmms/plugin.h"
#include "xmms/titlestring.h"
#include "id3.h"
#include <glib.h>

#define BUFFER_SIZE	1024*64

static void aac_init(void);
static int  aac_is_our_file(char *filename);
static void aac_play_file(char *filename);
static void aac_stop(void);
static void aac_pause(short paused);
static void aac_seek(int time);
static int  aac_get_time(void);
static void aac_cleanup(void);
static void aac_get_song_info(char *filename, char **title, int *length);

static void *decode_loop(void *arg);
static gchar *construct_title(char *filename, ID3Tag *id3tag);

/* Constructs a table of file positions for each second of output */
static void get_AAC_info(FILE *infile, unsigned long *bitrate, unsigned long **seek_table, unsigned long *length);

gboolean playing = FALSE;
gboolean output_opened = FALSE;
gint seek_pos = -1;
pthread_t decode_thread;

InputPlugin aac_ip =
{
	NULL,				/* handle */
	NULL,				/* filename */
	NULL,				/* description */
	aac_init,
	NULL,				/* about box */
	NULL,				/* configure plugin */
	aac_is_our_file,
	NULL,				/* scan dir */
	aac_play_file,
	aac_stop,
	aac_pause,
#if USE_SEEKING
	aac_seek,
#else
	NULL,
#endif
	NULL,				/* set equalizer */
	aac_get_time,
	NULL,				/* get volume */
	NULL,				/* set volume */
	aac_cleanup,
	NULL,				/* obsolete */
	NULL,				/* send visualization data */
	NULL,				/* set player window info */
	NULL,				/* set song title text */
	aac_get_song_info,
	NULL,				/* file info box */
	NULL				/* pointer to OutputPlugin */
};

InputPlugin *get_iplugin_info(void)
{
	aac_ip.description = g_strdup_printf("MPEG-2 AAC Player");
	return &aac_ip;
}

static void aac_init(void)
{
}

static int aac_is_our_file(char *filename)
{
	/* extension check (.AAC) */
	gchar *ext = NULL;
	ext = strrchr(filename, '.');
	if (ext && !strcasecmp(ext, ".aac"))
		return 1;
	return 0;
}

/* Quick hack to avoid patching id3lib - will be removed whenever id3lib
 * gets a working C API
 */
int _ID3_IsTagHeader(const unsigned char data[ID3_TAGHEADERSIZE])
{
	size_t size = 0;
	if (data[0] == 0x49 && data[1] == 0x44 && data[2] == 0x33 &&
	    data[3] < 0xff && data[4] < 0xff && data[5] < 0x80)
	{
		size = (data[9] & 0x7f) |
		       ((data[8] & 0x7f) << 7) |
		       ((data[7] & 0x7f) << 14) |
		       ((data[6] & 0x7f) << 21);
		return size;
	}
	else
	{
		return -1;
	}
}

#define DECODE_ERROR() \
	do { \
		printf("decode error at file: %s line: %d\n", __FILE__, __LINE__); \
		if (output_opened) \
		{ \
			aac_ip.output->flush(0); \
			aac_ip.output->close_audio(); \
			output_opened = FALSE; \
		} \
		if (decoder) faacDecClose(decoder); \
		if (infile) fclose(infile); \
		if (id3tag) ID3Tag_Delete(id3tag); \
		g_free(seek_table); \
		g_free(id3buffer); \
		g_free(buffer); \
		g_free(sample_buffer); \
		playing = FALSE; \
		pthread_exit(NULL); \
	} while (0);

static void *aac_decode_loop(void *arg)
{
	ID3Tag *id3tag = NULL;
	FILE *infile = NULL;
	gchar *filename = arg;
	int tagsize = 0;

	unsigned long buffervalid = 0;
	unsigned long bufferconsumed = 0;

	unsigned long bytesdecoded;
	unsigned long samplesdecoded;

	unsigned long samplerate;
	unsigned long channels;

	unsigned long bitrate = -1;

	int result = 0;

	unsigned char id3header[ID3_TAGHEADERSIZE];
	unsigned char *id3buffer = NULL;
	unsigned char *buffer = NULL;
	short *sample_buffer = NULL;
	unsigned long *seek_table = NULL;
	unsigned long seek_table_length = 0;

	faacDecHandle decoder = NULL;
	faacDecConfigurationPtr config = NULL;
	faacDecFrameInfo finfo;

	buffer = g_malloc0(BUFFER_SIZE);

	infile = fopen(filename, "rb");
	if (!infile) DECODE_ERROR();
	if (fread(id3header, 1, ID3_TAGHEADERSIZE, infile) != ID3_TAGHEADERSIZE)
		DECODE_ERROR();
	if ((tagsize = _ID3_IsTagHeader(id3header)) != -1)
	{
		id3buffer = g_malloc0(tagsize);
		if (fread(id3buffer, 1, tagsize, infile) != tagsize)
			DECODE_ERROR();
		id3tag = ID3Tag_New();
		ID3Tag_Parse(id3tag, id3header, id3buffer);
	}
	else
		fseek(infile, 0, SEEK_SET);

#if USE_SEEKING
	get_AAC_info(infile, &bitrate, &seek_table, &seek_table_length);
#endif
	
	decoder = faacDecOpen();
	if (!decoder) DECODE_ERROR();
	
	config = faacDecGetCurrentConfiguration(decoder);
	if (!config) DECODE_ERROR();

	buffervalid = fread(buffer, 1, BUFFER_SIZE, infile);
	bufferconsumed = faacDecInit(decoder, buffer, &samplerate, &channels);

	output_opened = aac_ip.output->open_audio(FMT_S16_LE, samplerate, channels);
	if (!output_opened)
		DECODE_ERROR();

#if USE_SEEKING
	aac_ip.set_info(construct_title(filename, id3tag), (seek_table_length-1)*1000, bitrate, samplerate, channels);
#else
	aac_ip.set_info(construct_title(filename, id3tag), -1, bitrate, samplerate, channels);
#endif
	
	while (playing && buffervalid > 0)
	{
		/* handle seeking */
		if (seek_pos != -1)
		{
			if (seek_pos > seek_table_length) seek_pos = seek_table_length;
			if (seek_pos < 0) seek_pos = 0;

			if (fseek(infile, seek_table[seek_pos], SEEK_SET) == -1)
				DECODE_ERROR();
			bufferconsumed = 0;
			buffervalid = fread(buffer, 1, BUFFER_SIZE, infile);

			aac_ip.output->flush(seek_pos*1000);
			seek_pos = -1;
		}
		
		if (bufferconsumed > 0)
		{
			memmove(buffer, &buffer[bufferconsumed], buffervalid-bufferconsumed);
			buffervalid -= bufferconsumed;
			buffervalid += fread(&buffer[buffervalid], 1, BUFFER_SIZE-buffervalid, infile);
		}
		sample_buffer = faacDecDecode(decoder, &finfo, buffer);
		bytesdecoded = finfo.bytesconsumed;
		samplesdecoded = finfo.samples;
		result = finfo.error;

		if (sample_buffer == NULL)
		{
			buffervalid = 0;
			printf("aac-XMMS: %s\n", faacDecGetErrorMessage(result));
		}

		bufferconsumed = bytesdecoded;

		if (sample_buffer && (samplesdecoded > 0))
		{
			while (playing && aac_ip.output->buffer_free() < (samplesdecoded << 1))
			{
				if (seek_pos != -1)
					break;
				else
					xmms_usleep(10000);
			}

			if (seek_pos == -1)
			{
				aac_ip.add_vis_pcm(aac_ip.output->written_time(), FMT_S16_LE, channels, samplesdecoded << 1, sample_buffer);
				aac_ip.output->write_audio(sample_buffer, samplesdecoded << 1);
			}
		}
	}

	if (output_opened)
	{
		while (playing && aac_ip.output->buffer_playing()) {
			xmms_usleep(10000);
		}
		aac_ip.output->flush(0);
		aac_ip.output->close_audio();
		output_opened = FALSE;
	} 

	if (decoder) faacDecClose(decoder); 
	if (infile) fclose(infile); 
	if (id3tag) ID3Tag_Delete(id3tag);

	g_free(seek_table);
	g_free(id3buffer); 
	g_free(buffer); 

	playing = FALSE;
	seek_pos = -1;
	
	pthread_exit(NULL); 
}

static void aac_play_file(char *filename)
{
	playing = TRUE;
	pthread_create(&decode_thread, NULL, aac_decode_loop, g_strdup(filename));
}

static void aac_stop(void)
{
	playing = FALSE;
	pthread_join(decode_thread, NULL);
}

static void aac_pause(short paused)
{
	if (output_opened)
		aac_ip.output->pause(paused);
}

static void aac_seek(int time)
{
	seek_pos = time;
	while (playing && seek_pos != -1) xmms_usleep(10000);
}

static int aac_get_time(void)
{
	if (!playing)
		return -1;
	return aac_ip.output->output_time();
}

/*
 * Construct a title string based on the given ID3 tag, or from the filename
 * if the tag is unavailable.  This does not take into account different 
 * encodings other than ASCII.
 *
 */
static gchar *construct_title(char *filename, ID3Tag *id3tag)
{
	TitleInput *input = NULL;
	ID3Frame *id3frame = NULL;
	ID3Field *id3field = NULL;
	gchar *ext = NULL;
	gchar *title = NULL;
	gchar *tmp = NULL;

	XMMS_NEW_TITLEINPUT(input);

	ext = strrchr(filename, '.');
	if (ext) ext++;
	
	input->file_name = g_basename(filename);
	input->file_ext = ext;
	input->file_path = filename;

	if (id3tag)
	{
		/* performer */	
		id3frame = ID3Tag_FindFrameWithID(id3tag, ID3FID_LEADARTIST);
		if (id3frame)
		{
			id3field = ID3Frame_GetField(id3frame, ID3FN_TEXT);
			if (id3field)
			{
				input->performer = g_malloc0((ID3Field_Size(id3field)+1));
				ID3Field_GetASCII(id3field, input->performer, ID3Field_Size(id3field));
				id3field = NULL;
			}
			id3frame = NULL;
		}
	
		/* album name */
		id3frame = ID3Tag_FindFrameWithID(id3tag, ID3FID_ALBUM);
		if (id3frame)
		{
			id3field = ID3Frame_GetField(id3frame, ID3FN_TEXT);
			if (id3field)
			{
				input->album_name = g_malloc0(ID3Field_Size(id3field)+1);
				ID3Field_GetASCII(id3field, input->album_name, ID3Field_Size(id3field));
				id3field = NULL;
			}
			id3frame = NULL;
		}

		/* track name */
		id3frame = ID3Tag_FindFrameWithID(id3tag, ID3FID_TITLE);
		if (id3frame)
		{
			id3field = ID3Frame_GetField(id3frame, ID3FN_TEXT);
			if (id3field)
			{
				input->track_name = g_malloc0((ID3Field_Size(id3field)+1));
				ID3Field_GetASCII(id3field, input->track_name, ID3Field_Size(id3field));
				id3field = NULL;
			}			
			id3frame = NULL;
		}

		/* track number */
		id3frame = ID3Tag_FindFrameWithID(id3tag, ID3FID_TRACKNUM);
		if (id3frame)
		{
			id3field = ID3Frame_GetField(id3frame, ID3FN_TEXT);
			if (id3field)
			{
				/* The track number MAY be extended with a '/'
				 * followed by a numeric string, eg. "4/9".
				 * In this case, take only the first number
				 */
				tmp = g_malloc0(ID3Field_Size(id3field)+1);
				ID3Field_GetASCII(id3field, tmp, ID3Field_Size(id3field));
				input->track_number = atoi(tmp);
				g_free(tmp);
				id3field = NULL;
			}
			id3frame = NULL;
		}

		/* year */
		id3frame = ID3Tag_FindFrameWithID(id3tag, ID3FID_YEAR);
		if (id3frame)
		{
			id3field = ID3Frame_GetField(id3frame, ID3FN_TEXT);
			if (id3field)
			{
				tmp = g_malloc0(ID3Field_Size(id3field)+1);
				ID3Field_GetASCII(id3field, tmp, ID3Field_Size(id3field));
				input->year = atoi(tmp);
				g_free(tmp);
				id3field = NULL;
			}
			id3frame = NULL;
		}

		/* date */
		id3frame = ID3Tag_FindFrameWithID(id3tag, ID3FID_DATE);
		if (id3frame)
		{
			id3field = ID3Frame_GetField(id3frame, ID3FN_TEXT);
			if (id3field)
			{
				input->date = g_malloc0(ID3Field_Size(id3field)+1);
				ID3Field_GetASCII(id3field, input->date, ID3Field_Size(id3field));
				id3field = NULL;
			}
			id3frame = NULL;
		}

		/* genre */
		/* XXX Genre handling is known to be broken.
		 *     It does not take into account id3v1 genre listings
		 */
		id3frame = ID3Tag_FindFrameWithID(id3tag, ID3FID_CONTENTTYPE);
		if (id3frame)
		{
			id3field = ID3Frame_GetField(id3frame, ID3FN_TEXT);
			if (id3field)
			{
				input->genre = g_malloc0(ID3Field_Size(id3field)+1);
				ID3Field_GetASCII(id3field, input->genre, ID3Field_Size(id3field));
				id3field = NULL;
			}
			id3frame = NULL;
		}

		/* comment */
		id3frame = ID3Tag_FindFrameWithID(id3tag, ID3FID_COMMENT);
		if (id3frame)
		{
			id3field = ID3Frame_GetField(id3frame, ID3FN_TEXT);
			if (id3field)
			{
				input->comment = g_malloc0(ID3Field_Size(id3field)+1);
				ID3Field_GetASCII(id3field, input->comment, ID3Field_Size(id3field));
				id3field = NULL;
			}
			id3frame = NULL;
		}
	}

	title = xmms_get_titlestring(xmms_get_gentitle_format(), input);

	g_free(input->performer);
	g_free(input->album_name);
	g_free(input->track_name);
	g_free(input->date);
	g_free(input->genre);
	g_free(input->comment);
	g_free(input);

	return title;
}

static void aac_get_song_info(char *filename, char **title, int *length)
{
	ID3Tag *id3tag = NULL;
	ID3Frame *id3frame = NULL;
	ID3Field *id3field = NULL;
	unsigned char header[ID3_TAGHEADERSIZE];
	unsigned char *buffer = NULL;
	unsigned long *seek_table = NULL;
	unsigned long seconds, bitrate;
	FILE *infile = NULL;
	int buffersize;

	(*length) = -1;
	
	infile = fopen(filename, "rb");
	if (!infile)
		return;

	memset(header, 0, ID3_TAGHEADERSIZE);
	fread(header, 1, ID3_TAGHEADERSIZE, infile);

	/* check for a valid ID3 header */
	if ((buffersize = _ID3_IsTagHeader(header)) != -1)
	{
		buffer = g_malloc0(buffersize);
		if (fread(buffer, 1, buffersize, infile) != buffersize)
		{
			g_free(buffer);
			fclose(infile);
			return;
		}
		id3tag = ID3Tag_New();
		ID3Tag_Parse(id3tag, header, buffer);

		id3frame = ID3Tag_FindFrameWithID(id3tag, ID3FID_SONGLEN);
		if (!id3frame)
		{
			id3field = ID3Frame_GetField(id3frame, ID3FN_TEXT);
			if (id3field)
			{
				buffer = g_malloc0(ID3Field_Size(id3field)+1);
				ID3Field_GetASCII(id3field, buffer, ID3Field_Size(id3field));
				(*length) = atoi(buffer);
				g_free(buffer);
			}

		}
	
	}

	(*title) = construct_title(filename, id3tag);

#if USE_SEEKING
	get_AAC_info(infile, &bitrate, &seek_table, &seconds);
	(*length) = (seconds-1)*1000;
#endif

	if (id3tag) ID3Tag_Delete(id3tag);
	if (infile) fclose(infile);

	g_free(seek_table);
	g_free(buffer);
}

static void aac_cleanup(void)
{
	if (aac_ip.description)
		g_free(aac_ip.description);
}

#define ADTS_HEADER_SIZE 8
#define SEEK_TABLE_CHUNK 60

static int sample_rates[] = { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000 };

/* Modeled after the FAAD winamp plugin */

static void get_AAC_info(FILE *infile, unsigned long *bitrate, unsigned long **seek_table, unsigned long *seek_table_length)
{
	unsigned long orig_pos, pos;
	unsigned char header[ADTS_HEADER_SIZE];
	unsigned int framecount, framelength, t_framelength = 0, framesinsec = 0;

	unsigned long *tmp_seek_table = NULL;
	unsigned long *tmp_realloc = NULL;
	unsigned long tmp_seek_table_length;
	unsigned long seconds = 0;

	float frames_per_sec;

	int id;
	int sample_rate_index;

	orig_pos = ftell(infile);

	(*bitrate) = -1;
	(*seek_table) = NULL;	
	(*seek_table_length) = -1;

	tmp_seek_table = g_malloc0(SEEK_TABLE_CHUNK * sizeof(unsigned long));
	tmp_seek_table_length = SEEK_TABLE_CHUNK;

	for (framecount = 0;; framecount++, framesinsec++)
	{
		pos = ftell(infile);
		if (fread(header, 1, ADTS_HEADER_SIZE, infile) != ADTS_HEADER_SIZE)
			break;

		/* check syncword */
		if (!((header[0] == 0xff) && ((header[1] & 0xf6) == 0xf0)))
			break;

		if (!framecount)
		{
			/* read the fixed ADTS header only once */
			id = header[1] & 0x08;
			sample_rate_index = (header[2] & 0x3c) >> 2;
			frames_per_sec = sample_rates[sample_rate_index] / 1024.0f;
		}

		if (id == 0)
		{
			/* MPEG-4 */
			framelength = ((unsigned int)header[4] << 5) | ((unsigned int)header[5] >> 3);
		}
		else
		{
			/* MPEG-2 */
			framelength = (((unsigned int)header[3] & 0x3) << 11) | ((unsigned int)header[4] << 3) | (header[5] >> 5);
		}
		t_framelength += framelength;

		if (framesinsec == 43)
		{
			framesinsec = 0;
		}

		if (framesinsec == 0)
		{
			if (seconds == tmp_seek_table_length)
			{
				tmp_seek_table = realloc(tmp_seek_table, (seconds + SEEK_TABLE_CHUNK)*sizeof(unsigned long));
				tmp_seek_table_length = seconds + SEEK_TABLE_CHUNK;
			}
			tmp_seek_table[seconds] = pos;
			seconds++;
		}

		if (fseek(infile, framelength - ADTS_HEADER_SIZE, SEEK_CUR) == -1)
			break;
	}

	if (seconds)
	{
		(*seek_table) = g_malloc0(seconds*sizeof(unsigned long));
		memcpy((*seek_table), tmp_seek_table, seconds*sizeof(unsigned long));
		(*seek_table_length) = seconds;

// Bitrate calculation here is unreliable, and variable, so setting it here
// doesn't help us any

//		(*bitrate) = (unsigned long)(((t_framelength / framecount)*(sample_rates[sample_rate_index]/1024.0))+0.5)*8;
	}

	g_free(tmp_seek_table);
	fseek(infile, orig_pos, SEEK_SET);
}

