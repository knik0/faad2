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

#include  "config.h"

#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#if (defined (WIN32) || defined (_WIN32))
	#pragma pack(push,1)
#endif

#define	SF_BUFFER_LEN			(4096)
#define	SF_FILENAME_LEN			(256)
#define	SF_HEADER_LEN			(2048)
#define	SF_TEXT_LEN				(1024)

#define		BITWIDTH2BYTES(x)	(((x) + 7) / 8)

#define		PEAK_CHANNEL_COUNT	16

/*	For some reason SIGNED_SIZEOF returns an unsigned  value which causes
**	a warning when that value is added or subtracted from a signed
**	value. Use SIGNED_SIZEOF instead.
*/
#define		SIGNED_SIZEOF(x)	((int) sizeof (x))

#if (defined (WIN32) || defined (_WIN32))
#	define	SF_MAX_COUNT		0x7FFFFFFFFFFFFFFFI64
#elif (SIZEOF_OFF_T == 4)
#	define	SF_MAX_COUNT		0x7FFFFFFF
#elif (SIZEOF_OFF_T == 8)
#	define	SF_MAX_COUNT		0x7FFFFFFFFFFFFFFFLL
#else
#	error "sizeof (long) != 4 && sizeof (long) != 8"
#endif

#define	SF_MAX_UINT				0xFFFFFFFF
#define	SF_MAX_INT				0x7FFFFFFF

#if (defined (WIN32) || defined (_WIN32))
	#define	SIZEOF_SF_COUNT_T	8
#else
	#define	SIZEOF_SF_COUNT_T	SIZEOF_OFF_T
#endif

enum
{	/* PEAK chunk location. */
	SF_PEAK_START		= 42,
	SF_PEAK_END			= 43,

	/* PEAK chunk location. */
	SF_SCALE_MAX		= 52,
	SF_SCALE_MIN		= 53,

	/* Char type for 8 bit files. */
	SF_CHARS_SIGNED		= 200,
	SF_CHARS_UNSIGNED	= 201
} ; 

#define		SFM_MASK 	(SFM_READ | SFM_WRITE | SFM_RDWR)
#define		SFM_UNMASK 	(~SFM_MASK)

/*---------------------------------------------------------------------------------------
**	PEAK_CHUNK - This chunk type is common to both AIFF and WAVE files although their 
**	endian encodings are different. 
*/

typedef struct 
{	float        value ;    	/* signed value of peak */ 
	unsigned int position ; 	/* the sample frame for the peak */ 
} PEAK_POS ; 

typedef struct 
{	unsigned int  version ;						/* version of the PEAK chunk */ 
	unsigned int  timestamp ;					/* secs since 1/1/1970  */ 
	PEAK_POS      peak [PEAK_CHANNEL_COUNT] ;	/* the peak info */ 
} PEAK_CHUNK ; 

/*=======================================================================================
**	SF_PRIVATE stuct - a pointer to this struct is passed back to the caller of the
**	sf_open_XXXX functions. The caller however has no knowledge of the struct's
**	contents. 
*/

typedef struct sf_private_tag
{	/* Force the compiler to double align the start of buffer. */
	double			buffer		[SF_BUFFER_LEN/sizeof(double)] ;
	char			filename	[SF_FILENAME_LEN] ;

	/* logbuffer and logindex should only be changed within the logging functions 
	** of common.c
	*/
	char			logbuffer	[SF_BUFFER_LEN] ;
	unsigned char	header		[SF_HEADER_LEN] ; /* Must be unsigned */

	/* For storing text from header. */
	char			headertext	[SF_TEXT_LEN] ;
	
	/* Guard value. If this changes the buffers above have overflowed. */ 
	int				Magick ;
	
	/* Index variables for maintaining logbuffer and header above. */
	unsigned int	logindex ;
	unsigned int	headindex, headcurrent ;
	int				has_text ;
	
	int 			filedes ;
	int				error ;
	
	int				mode ;			/* Open mode : SFM_READ, SFM_WRITE or SFM_RDWR. */
	int				endian ;		/* File endianness : SF_ENDIAN_LITTLE or SF_ENDIAN_BIG. */
	int				chars ;			/* Chars are SF_CHARS_SIGNED or SF_CHARS_UNSIGNED. */
	int				float_endswap ;	/* Need to endswap float32s? */
	
	SF_INFO			sf ; 	

	int				has_peak ;		/* Has a PEAK chunk (AIFF and WAVE) been read? */
	int				peak_loc ;		/* Write a PEAK chunk at the start or end of the file? */
	PEAK_CHUNK		peak ;			
	
	sf_count_t		dataoffset ;	/* Offset in number of bytes from beginning of file. */
	sf_count_t		datalength ;	/* Length in bytes of the audio data. */
	sf_count_t		dataend ;		/* Offset to file tailer. */

	int				blockwidth ;	/* Size in bytes of one set of interleaved samples. */
	int				bytewidth ;		/* Size in bytes of one sample (one channel). */

	sf_count_t		filelength ;

	int				last_op ;		/* Last operation; either SFM_READ or SFM_WRITE */ 	
	sf_count_t		read_current ;
	sf_count_t		write_current ;

	void			*fdata ;		/*	This is a pointer to dynamically allocated file format 
									**	specific data. 
									*/
	int				norm_double ;
	int				norm_float ;
	
	/* A set of file specific function pointers */

	sf_count_t		(*read_short)	(struct sf_private_tag*, short *ptr, sf_count_t len) ;
	sf_count_t		(*read_int)		(struct sf_private_tag*, int *ptr, sf_count_t len) ;
	sf_count_t		(*read_float)	(struct sf_private_tag*, float *ptr, sf_count_t len) ;
	sf_count_t		(*read_double)	(struct sf_private_tag*, double *ptr, sf_count_t len) ;

	sf_count_t		(*write_short)	(struct sf_private_tag*, short *ptr, sf_count_t len) ;
	sf_count_t		(*write_int)	(struct sf_private_tag*, int *ptr, sf_count_t len) ;
	sf_count_t		(*write_float)	(struct sf_private_tag*, float *ptr, sf_count_t len) ;
	sf_count_t		(*write_double)	(struct sf_private_tag*, double *ptr, sf_count_t len) ;

	sf_count_t		(*new_seek) 	(struct sf_private_tag*, int mode, sf_count_t samples_from_start) ;
	int				(*write_header)	(struct sf_private_tag*) ;
	int				(*command)		(struct sf_private_tag*, int command, void *data, int datasize) ;
	int				(*close)		(struct sf_private_tag*) ;

} SF_PRIVATE ;

enum
{	SFE_NO_ERROR	= 0,

	SFE_BAD_FILE,
	SFE_BAD_FILE_READ,
	SFE_OPEN_FAILED,
	SFE_BAD_OPEN_FORMAT,
	SFE_BAD_SNDFILE_PTR,
	SFE_BAD_SF_INFO_PTR,
	SFE_BAD_SF_INCOMPLETE,
	SFE_BAD_FILE_PTR,
	SFE_BAD_INT_PTR,
	SFE_MALLOC_FAILED, 
	SFE_UNIMPLEMENTED,
	SFE_BAD_READ_ALIGN,
	SFE_BAD_WRITE_ALIGN,
	SFE_UNKNOWN_FORMAT,
	SFE_NOT_READMODE,
	SFE_NOT_WRITEMODE,
	SFE_BAD_MODE_RW,
	SFE_BAD_SF_INFO,
	SFE_SHORT_READ,
	SFE_SHORT_WRITE,
	SFE_INTERNAL,
	SFE_LOG_OVERRUN,
	SFE_BAD_CONTROL_CMD,
	SFE_BAD_ENDIAN,
	SFE_CHANNEL_COUNT,
	
	SFE_BAD_SEEK, 
	SFE_NOT_SEEKABLE,
	SFE_AMBIGUOUS_SEEK,
	SFE_WRONG_SEEK,
	SFE_SEEK_FAILED,
	
	SFE_BAD_OPEN_MODE,
	SFE_OPEN_PIPE_RDWR,
	SFE_RDWR_POSITION,

	SFE_WAV_NO_RIFF,
	SFE_WAV_NO_WAVE,
	SFE_WAV_NO_FMT,
	SFE_WAV_FMT_SHORT,
	SFE_WAV_FMT_TOO_BIG,
	SFE_WAV_BAD_FACT,
	SFE_WAV_BAD_PEAK,
	SFE_WAV_PEAK_B4_FMT,
	SFE_WAV_BAD_FORMAT,
	SFE_WAV_BAD_BLOCKALIGN,
	SFE_WAV_NO_DATA,
	SFE_WAV_ADPCM_NOT4BIT,
	SFE_WAV_ADPCM_CHANNELS,
	SFE_WAV_GSM610_FORMAT,
	SFE_WAV_UNKNOWN_CHUNK,

	SFE_AIFF_NO_FORM,
	SFE_AIFF_AIFF_NO_FORM,
	SFE_AIFF_COMM_NO_FORM,
	SFE_AIFF_SSND_NO_COMM,
	SFE_AIFF_UNKNOWN_CHUNK,
	SFE_AIFF_COMM_CHUNK_SIZE,
	SFE_AIFF_BAD_COMM_CHUNK,
	SFE_AIFF_PEAK_B4_COMM,
	SFE_AIFF_BAD_PEAK,
	SFE_AIFF_NO_SSND,
	SFE_AIFF_NO_DATA,

	SFE_AU_UNKNOWN_FORMAT,
	SFE_AU_NO_DOTSND,
	
	SFE_RAW_READ_BAD_SPEC,
	SFE_RAW_BAD_BITWIDTH,
	
	SFE_PAF_NO_MARKER,
	SFE_PAF_VERSION,
	SFE_PAF_UNKNOWN_FORMAT,
	SFE_PAF_SHORT_HEADER,
	
	SFE_SVX_NO_FORM, 
	SFE_SVX_NO_BODY,
	SFE_SVX_NO_DATA,
	SFE_SVX_BAD_COMP, 	

	SFE_NIST_BAD_HEADER,
	SFE_NIST_BAD_ENCODING,

	SFE_SMTD_NO_SEKD, 
	SFE_SMTD_NO_SAMR, 

	SFE_VOC_NO_CREATIVE, 
	SFE_VOC_BAD_FORMAT, 
	SFE_VOC_BAD_VERSION, 
	SFE_VOC_BAD_MARKER, 
	SFE_VOC_BAD_SECTIONS, 
	SFE_VOC_MULTI_SAMPLERATE, 
	SFE_VOC_MULTI_SECTION, 
	SFE_VOC_MULTI_PARAM, 
	SFE_VOC_SECTION_COUNT, 

	SFE_IRCAM_NO_MARKER,
	SFE_IRCAM_BAD_CHANNELS,
	SFE_IRCAM_UNKNOWN_FORMAT,

	SFE_W64_64_BIT,
	SFE_W64_NO_RIFF,
	SFE_W64_NO_WAVE,
	SFE_W64_NO_FMT,
	SFE_W64_NO_DATA,
	SFE_W64_FMT_SHORT,
	SFE_W64_FMT_TOO_BIG,
	SFE_W64_ADPCM_NOT4BIT,
	SFE_W64_ADPCM_CHANNELS,
	SFE_W64_GSM610_FORMAT,

	SFE_DWVW_BAD_BITWIDTH,
	
	SFE_MAX_ERROR			/* This must be last in list. */
} ;

int subformat_to_bytewidth (int format) ;
int s_bitwidth_to_subformat (int bits) ;
int u_bitwidth_to_subformat (int bits) ;

/*  Functions for reading and writing floats and doubles on processors
**	with non-IEEE floats/doubles.
*/
float	float32_read  (unsigned char *cptr) ;
void	float32_write (float in, unsigned char *out) ;

double	double64_read  (unsigned char *cptr) ;
void	double64_write (double in, unsigned char *out) ;

/* Endian swapping routines implemented in src/common.h. */

void	endswap_short_array	(short *ptr, sf_count_t len) ;
void	endswap_int_array 	(int *ptr, sf_count_t len) ;
/* Always 8 byte values whether sizeof (long) == 8 or not. */
void	endswap_long_array  (long *ptr, sf_count_t len) ; 

/* Functions for writing to the internal logging buffer. */

void	psf_log_printf		(SF_PRIVATE *psf, char *format, ...) ;
void	psf_log_SF_INFO 	(SF_PRIVATE *psf) ;

/* Functions used when writing file headers. */

int		psf_binheader_writef	(SF_PRIVATE *psf, char *format, ...) ;
void	psf_asciiheader_printf	(SF_PRIVATE *psf, char *format, ...) ;

/* Functions used when reading file headers. */

int		psf_binheader_readf	(SF_PRIVATE *psf, char const *format, ...) ;

/* Functions used in the write function for updating the peak chunk. */

void	peak_update_short	(SF_PRIVATE *psf, short *ptr, size_t items) ;
void	peak_update_int		(SF_PRIVATE *psf, int *ptr, size_t items) ;
void	peak_update_double	(SF_PRIVATE *psf, double *ptr, size_t items) ;

/* Functions defined in command.c. */

int		psf_get_format_simple_count	(void) ;
int		psf_get_format_simple		(SF_FORMAT_INFO *data) ;

int		psf_get_format_major_count	(void) ;
int		psf_get_format_major		(SF_FORMAT_INFO *data) ;

int		psf_get_format_subtype_count	(void) ;
int		psf_get_format_subtype		(SF_FORMAT_INFO *data) ;

double	psf_calc_signal_max 		(SF_PRIVATE *psf) ;

/* Default seek function. Use for PCM and float encoded data. */
sf_count_t  psf_default_seek (SF_PRIVATE *psf, int mode, sf_count_t samples_from_start) ;

/*------------------------------------------------------------------------------------ 
**	File I/O functions which will allow access to large files (> 2 Gig) on
**	some 32 bit OSes. Implementation in file_io.c.
*/ 

int psf_open (const char *pathname, int flags) ;

sf_count_t psf_fseek (int fd, sf_count_t offset, int whence) ;
sf_count_t psf_fread (void *ptr, sf_count_t bytes, sf_count_t count, int fd) ;
sf_count_t psf_fwrite (void *ptr, sf_count_t bytes, sf_count_t count, int fd) ;
sf_count_t psf_fgets (char *buffer, sf_count_t bufsize, int fd) ;
sf_count_t psf_ftell (int fd) ;
sf_count_t psf_get_filelen (int fd) ;

void psf_fclearerr (int fd) ;

int psf_ferror (int fd) ;
int psf_fclose (int fd) ;

/*------------------------------------------------------------------------------------ 
** Functions for reading and writing different file formats.
*/

int		aiff_open	(SF_PRIVATE *psf) ;
int		au_open		(SF_PRIVATE *psf) ;
int		au_nh_open	(SF_PRIVATE *psf) ;	/* Headerless version of AU. */
int		ircam_open	(SF_PRIVATE *psf) ;
int		nist_open	(SF_PRIVATE *psf) ;
int		paf_open	(SF_PRIVATE *psf) ;
int		raw_open	(SF_PRIVATE *psf) ;
int		svx_open	(SF_PRIVATE *psf) ;
int		voc_open	(SF_PRIVATE *psf) ;
int		w64_open	(SF_PRIVATE *psf) ;
int		wav_open	(SF_PRIVATE *psf) ;

/* In progress. Do not currently work. */

int		rx2_open	(SF_PRIVATE *psf) ;
int		sd2_open	(SF_PRIVATE *psf) ;
int		smpltd_open	(SF_PRIVATE *psf) ;
int		txw_open	(SF_PRIVATE *psf) ;

/*------------------------------------------------------------------------------------ 
**	Init functions for a number of common data encodings. 
*/

int 	pcm_init		(SF_PRIVATE *psf) ;
int 	ulaw_init		(SF_PRIVATE *psf) ;
int 	alaw_init		(SF_PRIVATE *psf) ;
int 	float32_init	(SF_PRIVATE *psf) ;
int 	double64_init	(SF_PRIVATE *psf) ;
int 	dwvw_init		(SF_PRIVATE *psf, int bitwidth) ;
int		gsm610_init 	(SF_PRIVATE *psf) ;

int		wav_w64_ima_init (SF_PRIVATE *psf, int blockalign, int samplesperblock) ;
int		wav_w64_msadpcm_init (SF_PRIVATE *psf, int blockalign, int samplesperblock) ;

int		aiff_ima_init (SF_PRIVATE *psf, int blockalign, int samplesperblock) ;

/*------------------------------------------------------------------------------------ 
** Here's how we fix systems which don't snprintf / vsnprintf.
** Systems without these functions should use the 
*/

#if (defined (WIN32) || defined (_WIN32))
#define	LSF_SNPRINTF	_snprintf
#elif		(HAVE_SNPRINTF && ! FORCE_MISSING_SNPRINTF)
#define	LSF_SNPRINTF	snprintf
#else
int missing_snprintf (char *str, size_t n, char const *fmt, ...) ;
#define	LSF_SNPRINTF	missing_snprintf
#endif

#if (defined (WIN32) || defined (_WIN32))
#define	LSF_VSNPRINTF	_vsnprintf
#elif		(HAVE_VSNPRINTF && ! FORCE_MISSING_SNPRINTF)
#define	LSF_VSNPRINTF	vsnprintf
#else
int missing_vsnprintf (char *str, size_t n, const char *fmt, ...) ;
#define	LSF_VSNPRINTF	missing_vsnprintf
#endif

#ifdef _WIN32
	#pragma pack(pop,1)
#endif

#endif /* COMMON_H_INCLUDED */

