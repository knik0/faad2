/*
** Copyright (C) 1999-2002 Erik de Castro Lopo <erikd@zip.com.au>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 2.1 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
** 
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/* sndfile.h -- system-wide definitions */

#ifndef SNDFILE_H
#define SNDFILE_H

/* This is the version 1.0.X header file. */
#define	SNDFILE_1

#include <stdio.h>
#include <stdlib.h>

/* For the Metrowerks CodeWarrior Pro Compiler (mainly MacOS) */

#if	(defined (__MWERKS__))
#include	<unix.h>
#else
#include	<sys/types.h>
#endif

#ifdef _WIN32
	#pragma pack(push,1)
#endif

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/* The following file types can be read and written.
** A file type would consist of a major type (ie SF_FORMAT_WAV) bitwise
** ORed with a minor type (ie SF_FORMAT_PCM). SF_FORMAT_TYPEMASK and
** SF_FORMAT_SUBMASK can be used to separate the major and minor file
** types.
*/

enum
{	SF_FORMAT_WAV			= 0x010000,		/* Microsoft WAV format (little endian). */
	SF_FORMAT_AIFF			= 0x020000,		/* Apple/SGI AIFF format (big endian). */
	SF_FORMAT_AU			= 0x030000,		/* Sun/NeXT AU format (big endian). */
	SF_FORMAT_RAW			= 0x040000,		/* RAW PCM data. */
	SF_FORMAT_PAF			= 0x050000,		/* Ensoniq PARIS file format. */
	SF_FORMAT_SVX			= 0x060000,		/* Amiga IFF / SVX8 / SV16 format. */
	SF_FORMAT_NIST			= 0x070000,		/* Sphere NIST format. */
	SF_FORMAT_VOC			= 0x080000,		/* VOC files. */
	SF_FORMAT_REX			= 0x090000,		/* Propellorheads Rex/Rcy */
	SF_FORMAT_IRCAM			= 0x0A0000,		/* Berkeley/IRCAM/CARL */
	SF_FORMAT_W64			= 0x0B0000,		/* Sonic Foundry's 64 bit RIFF/WAV */
	SF_FORMAT_SD2			= 0x0C0000,		/* Sound Designer 2 */
	SF_FORMAT_REX2			= 0x0D0000,		/* Propellorheads Rex2 */
	SF_FORMAT_TXW			= 0x0E0000,		/* Yamaha TX16 sampler file */
	SF_FORMAT_KRZ			= 0x0F0000,		/* Kurzweil sampler file */
	SF_FORMAT_OCT			= 0x100000,		/* Save as a GNU Octave data file */
	SF_FORMAT_SMPLTD		= 0x110000,		/* Sekd Samplitude. */
	SF_FORMAT_WMA			= 0x120000,		/* Windows Media Audio. */
	SF_FORMAT_SHN			= 0x130000,		/* Shorten. */
	
	/* Subtypes from here on. */
	
	SF_FORMAT_PCM_S8		= 0x0001,		/* Signed 8 bit data */
	SF_FORMAT_PCM_16		= 0x0002,		/* Signed 16 bit data */
	SF_FORMAT_PCM_24		= 0x0003,		/* Signed 24 bit data */
	SF_FORMAT_PCM_32		= 0x0004,		/* Signed 32 bit data */

	SF_FORMAT_PCM_U8		= 0x0005,		/* Unsigned 8 bit data (WAV and RAW only) */
	
	SF_FORMAT_FLOAT			= 0x0006,		/* 32 bit float data */
	SF_FORMAT_DOUBLE		= 0x0007,		/* 64 bit float data */
	
	SF_FORMAT_ULAW			= 0x0008,		/* U-Law encoded. */
	SF_FORMAT_ALAW			= 0x0009,		/* A-Law encoded. */
	SF_FORMAT_IMA_ADPCM		= 0x000A,		/* IMA ADPCM. */
	SF_FORMAT_MS_ADPCM		= 0x000B,		/* Microsoft ADPCM. */

	SF_FORMAT_GSM610		= 0x000C,		/* GSM 6.10 encoding. */

	SF_FORMAT_G721_32		= 0x000D,		/* 32kbs G721 ADPCM encoding. */
	SF_FORMAT_G723_24		= 0x000E,		/* 24kbs G723 ADPCM encoding. */

	SF_FORMAT_DWVW_12		= 0x000F, 		/* 12 bit Delta Width Variable Word encoding. */
	SF_FORMAT_DWVW_16		= 0x0010, 		/* 16 bit Delta Width Variable Word encoding. */
	SF_FORMAT_DWVW_24		= 0x0011, 		/* 24 bit Delta Width Variable Word encoding. */
	SF_FORMAT_DWVW_N		= 0x0012, 		/* N bit Delta Width Variable Word encoding. */

	SF_FORMAT_SVX_FIB		= 0x0020, 		/* SVX Fibonacci Delta encoding. */
	SF_FORMAT_SVX_EXP		= 0x0021, 		/* SVX Exponential Delta encoding. */

	/* Endian-ness options. */
	
	SF_ENDIAN_FILE			= 0x00000000,	/* Default file endian-ness. */
	SF_ENDIAN_LITTLE		= 0x10000000,	/* Force little endian-ness. */
	SF_ENDIAN_BIG			= 0x20000000,	/* Force big endian-ness. */
	SF_ENDIAN_CPU			= 0x30000000,	/* Force CPU endian-ness. */

	SF_FORMAT_SUBMASK		= 0x0000FFFF,		
	SF_FORMAT_TYPEMASK		= 0x0FFF0000,
	SF_FORMAT_ENDMASK		= 0x30000000
} ;

/* The following are the valid command numbers for the sf_command()
** interface. 
*/

enum
{	SFC_GET_LIB_VERSION	= 0x1000,
	SFC_GET_LOG_INFO,

	SFC_GET_NORM_DOUBLE,
	SFC_GET_NORM_FLOAT,
	SFC_SET_NORM_DOUBLE,
	SFC_SET_NORM_FLOAT,
	
	SFC_GET_SIMPLE_FORMAT_COUNT,
	SFC_GET_SIMPLE_FORMAT,

	SFC_GET_FORMAT_MAJOR_COUNT,
	SFC_GET_FORMAT_MAJOR,
	SFC_GET_FORMAT_SUBTYPE_COUNT,
	SFC_GET_FORMAT_SUBTYPE,
	
	SFC_CALC_SIGNAL_MAX
} ;

enum
{	/* True and false */
	SF_FALSE		= 0,
	SF_TRUE			= 1,

	/* Modes for opening files. */
	SFM_READ	= 0x10, 
	SFM_WRITE	= 0x20,
	SFM_RDWR	= 0x30
} ;

/* A SNDFILE* pointer can be passed around much like stdio.h's FILE* pointer. */

typedef	void		SNDFILE ;

#if (defined (WIN32) || defined (_WIN32))
	typedef __int64		sf_count_t ;
#else
	typedef off_t		sf_count_t ;
#endif

/* A pointer to a SF_INFO structure is passed to sf_open_read () and filled in. 
** On write, the SF_INFO structure is filled in by the user and passed into  
** sf_open_write ().
*/

struct SF_INFO
{	sf_count_t	samples ;		/* -1 if unknown. */
	int			samplerate ;
	int			channels ;
	int			format ;
	int			sections ;
	int			seekable ;
} ;

typedef	struct SF_INFO SF_INFO ;

/* The SF_FORMAT_INFO struct is used to retrieve information about the sound 
** file formats libsndfile supports using the sf_command () interface.
**
** Using this interface will allow applications to support new file formats 
** and encoding types when libsndfile is upgraded, without requiring 
** re-compilation of the application.
** 
** Please consult the libsndfile documentation (particularly the information
** on the sf_command () interface) for examples of its use.
*/

typedef struct
{	int			format ;
	const char  *name ;
	const char  *extension ;
} SF_FORMAT_INFO ;

/* Open the specified file for read, write or both. On error, this will 
** return a NULL pointer. To find the error number, pass a NULL SNDFILE 
** to sf_perror () or sf_error_str ().
*/

SNDFILE* 	sf_open		(const char *path, int mode, SF_INFO *sfinfo) ;

/* sf_error () returns TRUE if an error has been recorded for the given SNDFILE. */

int		sf_error		(SNDFILE *sndfile) ;

/* sf_perror () prints the current error string to stderr. */

int		sf_perror		(SNDFILE *sndfile) ;

/* sf_error_str () returns the current error message to the caller in the 
** string buffer provided. 
*/

int		sf_error_str	(SNDFILE *sndfile, char* str, size_t len) ;

/* sf_error_number () allows the retrieval of the error string for each internal
** error number. This is provided mainly for testing purposes and most users of
** this library will not need to use it.
** 
*/

int		sf_error_number	(int errnum, char *str, size_t maxlen) ;

/* Return TRUE if fields of the SF_INFO struct are a valid combination of values. */

int		sf_command	(SNDFILE *sndfile, int command, void *data, int datasize) ;

/* Return TRUE if fields of the SF_INFO struct are a valid combination of values. */

int		sf_format_check	(const SF_INFO *info) ;

/* Seek within the waveform data chunk of the SNDFILE. sf_seek () uses 
** the same values for whence (SEEK_SET, SEEK_CUR and SEEK_END) as
** stdio.h function fseek ().
** An offset of zero with whence set to SEEK_SET will position the 
** read / write pointer to the first data sample.
** On success sf_seek returns the current position in (multi-channel) 
** samples from the start of the file.
** The sf_read_seek() and sf_write_seek functions only work on files 
** openned in read/write mode allowing the current read and write 
** positions to be manipulated separately.
** On error all of these functions return -1.
*/

sf_count_t	sf_seek 		(SNDFILE *sndfile, sf_count_t frames, int whence) ;

/* Functions for reading/writing the waveform data of a sound file.
*/

sf_count_t	sf_read_raw		(SNDFILE *sndfile, void *ptr, sf_count_t bytes) ;
sf_count_t	sf_write_raw 	(SNDFILE *sndfile, void *ptr, sf_count_t bytes) ;

/* Functions for reading and writing the data chunk in terms of frames. 
** The number of items actually read/written = frames * number of channels.
**     sf_xxxx_raw		read/writes the raw data bytes from/to the file
**     sf_xxxx_short	passes data in the native short format
**     sf_xxxx_int		passes data in the native int format
**     sf_xxxx_float	passes data in the native float format
**     sf_xxxx_double	passes data in the native double format
** All of these read/write function return number of frames read/written.
*/

sf_count_t	sf_readf_short	(SNDFILE *sndfile, short *ptr, sf_count_t frames) ;
sf_count_t	sf_writef_short	(SNDFILE *sndfile, short *ptr, sf_count_t frames) ;

sf_count_t	sf_readf_int	(SNDFILE *sndfile, int *ptr, sf_count_t frames) ;
sf_count_t	sf_writef_int 	(SNDFILE *sndfile, int *ptr, sf_count_t frames) ;

sf_count_t	sf_readf_float	(SNDFILE *sndfile, float *ptr, sf_count_t frames) ;
sf_count_t	sf_writef_float	(SNDFILE *sndfile, float *ptr, sf_count_t frames) ;

sf_count_t	sf_readf_double	(SNDFILE *sndfile, double *ptr, sf_count_t frames) ;
sf_count_t	sf_writef_double(SNDFILE *sndfile, double *ptr, sf_count_t frames) ;

/* Functions for reading and writing the data chunk in terms of items. 
** Otherwise similar to above.
** All of these read/write function return number of items read/written.
*/

sf_count_t	sf_read_short	(SNDFILE *sndfile, short *ptr, sf_count_t items) ;
sf_count_t	sf_write_short	(SNDFILE *sndfile, short *ptr, sf_count_t items) ;

sf_count_t	sf_read_int		(SNDFILE *sndfile, int *ptr, sf_count_t items) ;
sf_count_t	sf_write_int 	(SNDFILE *sndfile, int *ptr, sf_count_t items) ;

sf_count_t	sf_read_float	(SNDFILE *sndfile, float *ptr, sf_count_t items) ;
sf_count_t	sf_write_float	(SNDFILE *sndfile, float *ptr, sf_count_t items) ;

sf_count_t	sf_read_double	(SNDFILE *sndfile, double *ptr, sf_count_t items) ;
sf_count_t	sf_write_double	(SNDFILE *sndfile, double *ptr, sf_count_t items) ;

/* Close the SNDFILE. Returns 0 on success, or an error number. */

int		sf_close		(SNDFILE *sndfile) ;

#ifdef __cplusplus
}		/* extern "C" */
#endif	/* __cplusplus */

#ifdef _WIN32
	#pragma pack(pop,1)
#endif

#endif	/* SNDFILE_H */
