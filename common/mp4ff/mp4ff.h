#ifndef MP4FF_H
#define MP4FF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>


#include "private.h"
#include "funcprotos.h"



/* This is the reference for all your library entry points. */


/* =========================== public interface ========================= // */

/* return 1 if the file is a quicktime file */
int mp4ff_check_sig(mp4_callback_t *path);

/* call this first to open the file and create all the objects */
mp4ff_t* mp4ff_open(mp4_callback_t *callbacks, int rd, int wr, int append);

/* make the quicktime file streamable */
int mp4ff_make_streamable(mp4_callback_t *in_path, mp4_callback_t *out_path);

/* Set various options in the file. */
int mp4ff_set_time_scale(mp4ff_t *file, int time_scale);
int mp4ff_set_copyright(mp4ff_t *file, char *string);
int mp4ff_set_name(mp4ff_t *file, char *string);
int mp4ff_set_info(mp4ff_t *file, char *string);
int mp4ff_get_time_scale(mp4ff_t *file);
char* mp4ff_get_copyright(mp4ff_t *file);
char* mp4ff_get_name(mp4ff_t *file);
char* mp4ff_get_info(mp4ff_t *file);

/* Read all the information about the file. */
/* Requires a MOOV atom be present in the file. */
/* If no MOOV atom exists return 1 else return 0. */
int mp4ff_read_info(mp4ff_t *file);

/* set up tracks in a new file after opening and before writing */
/* returns the number of quicktime tracks allocated */
/* audio is stored two channels per quicktime track */
int mp4ff_set_audio(mp4ff_t *file, int channels, long sample_rate, int bits, int sample_size, int time_scale, int sample_duration, char *compressor);

/* Samplerate can be set after file is created */
int mp4ff_set_framerate(mp4ff_t *file, float framerate);

/* video is stored one layer per quicktime track */
int mp4ff_set_video(mp4ff_t *file, int tracks, int frame_w, int frame_h, float frame_rate, int time_scale, char *compressor);

/* routines for setting various video parameters */

/* Set the depth of the track. */
int mp4ff_set_depth(mp4ff_t *file, int depth, int track);

/* close the file and delete all the objects */
int mp4ff_write(mp4ff_t *file);
int mp4ff_destroy(mp4ff_t *file);
int mp4ff_close(mp4ff_t *file);

/* get length information */
/* channel numbers start on 1 for audio and video */
long mp4ff_audio_length(mp4ff_t *file, int track);
long mp4ff_video_length(mp4ff_t *file, int track);

/* get position information */
long mp4ff_audio_position(mp4ff_t *file, int track);
long mp4ff_video_position(mp4ff_t *file, int track);

/* get file information */
int mp4ff_video_tracks(mp4ff_t *file);
int mp4ff_audio_tracks(mp4ff_t *file);

int mp4ff_has_audio(mp4ff_t *file);
long mp4ff_audio_sample_rate(mp4ff_t *file, int track);
int mp4ff_audio_bits(mp4ff_t *file, int track);
int mp4ff_track_channels(mp4ff_t *file, int track);
int mp4ff_audio_time_scale(mp4ff_t *file, int track);
int mp4ff_audio_sample_duration(mp4ff_t *file, int track);
char* mp4ff_audio_compressor(mp4ff_t *file, int track);

int mp4ff_has_video(mp4ff_t *file);
int mp4ff_video_width(mp4ff_t *file, int track);
int mp4ff_video_height(mp4ff_t *file, int track);
int mp4ff_video_depth(mp4ff_t *file, int track);
float mp4ff_video_frame_rate(mp4ff_t *file, int track);
char* mp4ff_video_compressor(mp4ff_t *file, int track);
int mp4ff_video_time_scale(mp4ff_t *file, int track);

int mp4ff_video_frame_time(mp4ff_t *file, int track, long frame,
    long *start_time, int *duration);

int mp4ff_get_iod_audio_profile_level(mp4ff_t *file);
int mp4ff_set_iod_audio_profile_level(mp4ff_t *file, int id);
int mp4ff_get_iod_video_profile_level(mp4ff_t *file);
int mp4ff_set_iod_video_profile_level(mp4ff_t *file, int id);

int mp4ff_get_mp4_video_decoder_config(mp4ff_t *file, int track, unsigned char** ppBuf, int* pBufSize);
int mp4ff_set_mp4_video_decoder_config(mp4ff_t *file, int track, unsigned char* pBuf, int bufSize);
int mp4ff_get_mp4_audio_decoder_config(mp4ff_t *file, int track, unsigned char** ppBuf, int* pBufSize);
int mp4ff_set_mp4_audio_decoder_config(mp4ff_t *file, int track, unsigned char* pBuf, int bufSize);

/* number of bytes of raw data in this frame */
long mp4ff_frame_size(mp4ff_t *file, long frame, int track);
long mp4ff_audio_frame_size(mp4ff_t *file, long frame, int track);

/* get the quicktime track and channel that the audio channel belongs to */
/* channels and tracks start on 0 */
int mp4ff_channel_location(mp4ff_t *file, int *mp4ff_track, int *mp4ff_channel, int channel);

/* file positioning */
int mp4ff_seek_end(mp4ff_t *file);
int mp4ff_seek_start(mp4ff_t *file);

/* set position of file descriptor relative to a track */
int mp4ff_set_audio_position(mp4ff_t *file, long sample, int track);
int mp4ff_set_video_position(mp4ff_t *file, long frame, int track);

/* ========================== Access to raw data follows. */
/* write data for one quicktime track */
/* the user must handle conversion to the channels in this track */
int mp4ff_write_audio(mp4ff_t *file, char *audio_buffer, long samples, int track);
int mp4ff_write_audio_frame(mp4ff_t *file, unsigned char *audio_buffer, long bytes, int track);

int mp4ff_write_video_frame(mp4ff_t *file, unsigned char *video_buffer, long bytes, int track, unsigned char isKeyFrame, long duration, long renderingOffset);

/* for writing a frame using a library that needs a file descriptor */
int mp4ff_write_frame_init(mp4ff_t *file, int track); /* call before fwrite */
mp4_callback_t* mp4ff_get_fd(mp4ff_t *file);     /* return a file descriptor */
int mp4ff_write_frame_end(mp4ff_t *file, int track); /* call after fwrite */

/* For reading and writing audio to a file descriptor. */
int mp4ff_write_audio_end(mp4ff_t *file, int track, long samples); /* call after fwrite */

/* Read an entire chunk. */
/* read the number of bytes starting at the byte_start in the specified chunk */
/* You must provide enough space to store the chunk. */
int mp4ff_read_chunk(mp4ff_t *file, char *output, int track, long chunk, long byte_start, long byte_len);

/* read raw data */
long mp4ff_read_audio(mp4ff_t *file, char *audio_buffer, long samples, int track);
long mp4ff_read_audio_frame(mp4ff_t *file, unsigned char *audio_buffer, int maxBytes, int track);
long mp4ff_read_frame(mp4ff_t *file, unsigned char *video_buffer, int track);
long mp4ff_get_sample_duration(mp4ff_t *file, long frame, int track);

/* for reading frame using a library that needs a file descriptor */
/* Frame caching doesn't work here. */
int mp4ff_read_frame_init(mp4ff_t *file, int track);
int mp4ff_read_frame_end(mp4ff_t *file, int track);

/* Dump the file structures for the currently opened file. */
int mp4ff_dump(mp4ff_t *file);

/* Test the 32 bit overflow */
int mp4ff_test_position(mp4ff_t *file);

#ifdef __cplusplus
}
#endif

#endif
