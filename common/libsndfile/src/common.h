/*
** Copyright (C) 1999-2001 Erik de Castro Lopo <erikd@zip.com.au>
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

#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#ifdef _WIN32
	#pragma pack(push,1)
#endif


#define	SF_BUFFER_LEN			(4096)
#define	SF_FILENAME_LEN			(256)
#define	SF_HEADER_LEN			(2048)
#define	SF_TEXT_LEN				(1024)

#define		BITWIDTH2BYTES(x)	(((x) + 7) / 8)

#define		PEAK_CHANNEL_COUNT	16

enum
{	SF_MODE_READ		= 11, 
	SF_MODE_WRITE		= 12,
	SF_MODE_RW			= 13,	/* Unlikely that RW will ever be implemented. */ 
	
	/* PEAK chunk location. */
	SF_PEAK_START		= 42,
	SF_PEAK_END			= 43,

	/* Two endian enums. */
	SF_ENDIAN_LITTLE	= 100, 
	SF_ENDIAN_BIG		= 101,
	
	/* Char type for 8 bit files. */
	SF_CHARS_SIGNED		= 200,
	SF_CHARS_UNSIGNED	= 201
} ; 

/*	Processor floating point capabilities. float32_get_capabilities () in
**	src/float32.c returns one of the latter three values.
*/
enum
{	FLOAT_UNKNOWN		= 0x00,
	FLOAT_CAN_RW_LE		= 0x23,
	FLOAT_CAN_RW_BE		= 0x34,
	FLOAT_BROKEN_LE		= 0x35,
	FLOAT_BROKEN_BE		= 0x36
} ;

enum
{	SF_FALSE = 0, 
	SF_TRUE = 1
} ; 

/* Command values for sf_command (). These are obtained using the Python
** script sf_command.py in the top level directory of the libsndfile sources.
*/
enum
{	SFC_LIB_VERSION	= 0x1048C,
	SFC_READ_TEXT	= 0x054F0,
	SFC_WRITE_TEXT	= 0x0B990,
	SFC_NORM_FLOAT	= 0x0914A,
	SFC_NORM_DOUBLE	= 0x1226D,
	SFC_SCALE_MODE	= 0x0A259,
	SFC_ADD_PEAK	= 0x96F53
} ;

/*	Function pointer typedefs. */

typedef	int	(*func_seek) 	(void*, long, int) ;

typedef	int	(*func_short)	(void*, short *ptr, unsigned int len) ;
typedef	int	(*func_int)		(void*, int *ptr, unsigned int len) ;
typedef	int	(*func_float)	(void*, float *ptr, unsigned int len) ;
typedef	int	(*func_double)	(void*, double *ptr, unsigned int len, int normalize) ;

typedef	int	(*func_wr_hdr)	(void*) ;
typedef	int	(*func_command)	(void*, int command, void *data, int datasize) ;

typedef	int	(*func_close)	(void*) ;

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

typedef struct
{	/* Force the compiler to double align the start of buffer. */
	double			buffer		[SF_BUFFER_LEN/sizeof(double)] ;
	char			filename	[SF_FILENAME_LEN] ;

	/* logbuffer and logindex should only be changed within the logging functions 
	** of common.c
	*/
	char			logbuffer	[SF_BUFFER_LEN] ;
	unsigned char	header		[SF_HEADER_LEN] ;

	/* For storing text from header. */
	char			headertext	[SF_TEXT_LEN] ;
	
	/* Guard value. If this changes the buffers above have overflowed. */ 
	int				Magick ;
	
	/* Index variables for maintaining logbuffer and header above. */
	unsigned int	logindex ;
	unsigned int	headindex, headcurrent ;
	int				has_text ;
	
	FILE 			*file ;
	int				error ;
	
	int				mode ;			/* Open mode : SF_MODE_READ or SF_MODE_WRITE. */
	int				endian ;		/* File endianness : SF_ENDIAN_LITTLE or SF_ENDIAN_BIG. */
	int				chars ;			/* Chars are SF_CHARS_SIGNED or SF_CHARS_UNSIGNED. */
	int				fl32_endswap ;	/* Need to endswap float32s? */
	
	SF_INFO			sf ; 	

	int				has_peak ;		/* Has a PEAK chunk (AIFF and WAVE) been read? */
	int				peak_loc ;		/* Write a PEAK chunk at the start or end of the file? */
	PEAK_CHUNK		peak ;			
	
	long			dataoffset ;	/* Offset in number of bytes from beginning of file. */
	long			datalength ;	/* Length in bytes of the audio data. */
	long			tailstart ;		/* Offset to file tailer. */
	unsigned int	blockwidth ;	/* Size in bytes of one set of interleaved samples. */
	unsigned int	bytewidth ;		/* Size in bytes of one sample (one channel). */

	long			filelength ;
	long			current ;

	void			*fdata ;
	
	int				scale_mode ;
	int				norm_double ;
	int				norm_float ;

	func_seek		seek_func ;

	func_short		read_short ;
	func_int		read_int ;
	func_float		read_float ;
	func_double		read_double ;

	func_short		write_short ;
	func_int		write_int ;
	func_float		write_float ;
	func_double		write_double ;

	func_wr_hdr		write_header ;
	func_command	command ;
	func_close		close ;

} SF_PRIVATE ;

enum
{	SFE_NO_ERROR	= 0,

	SFE_BAD_FILE,
	SFE_OPEN_FAILED,
	SFE_BAD_OPEN_FORMAT,
	SFE_BAD_SNDFILE_PTR,
	SFE_BAD_SF_INFO_PTR,
	SFE_BAD_INT_FD,
	SFE_BAD_INT_PTR,
	SFE_MALLOC_FAILED, 
	SFE_BAD_SEEK, 
	SFE_NOT_SEEKABLE,
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
	SFE_BAD_CONTROL_CMD,
	
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
	SFE_VOC_BAD_VERSION, 
	SFE_VOC_BAD_MARKER, 

	SFE_IRCAM_NO_MARKER,
	SFE_IRCAM_BAD_CHANNELS,
	SFE_IRCAM_UNKNOWN_FORMAT,

	SFE_MAX_ERROR			/* This must be last in list. */
} ;

/* Get the float32 capability of the processor at run time.
**	Implemented in src/float32.c.
*/
int		float32_get_capability (int endianness) ;
float	float32_read  (unsigned char *cptr) ;
void	float32_write (float in, unsigned char *out) ;


/* Endian swapping routines implemented in src/common.h. */

void	endswap_short_array	(short *ptr, int len) ;
void	endswap_int_array 	(int *ptr, int len) ;

/* Functions for writing to the internal logging buffer. */

void	psf_log_printf		(SF_PRIVATE *psf, char *format, ...) ;
void	psf_log_SF_INFO 		(SF_PRIVATE *psf) ;

/* Functions used when writing file headers. */

int		psf_binheader_writef	(SF_PRIVATE *psf, char *format, ...) ;
void	psf_asciiheader_printf	(SF_PRIVATE *psf, char *format, ...) ;

/* Functions used when reading file headers. */

int		psf_binheader_readf	(SF_PRIVATE *psf, char const *format, ...) ;

/* Functions used in the write function for updating the peak chunk. */

void	peak_update_short	(SF_PRIVATE *psf, short *ptr, size_t items) ;
void	peak_update_int		(SF_PRIVATE *psf, int *ptr, size_t items) ;
void	peak_update_double	(SF_PRIVATE *psf, double *ptr, size_t items) ;

/* Init functions for a number of common data encodings. */

int 	pcm_read_init  (SF_PRIVATE *psf) ;
int 	pcm_write_init (SF_PRIVATE *psf) ;

int 	ulaw_read_init  (SF_PRIVATE *psf) ;
int 	ulaw_write_init (SF_PRIVATE *psf) ;

int 	alaw_read_init  (SF_PRIVATE *psf) ;
int 	alaw_write_init (SF_PRIVATE *psf) ;

int 	float32_read_init  (SF_PRIVATE *psf) ;
int 	float32_write_init (SF_PRIVATE *psf) ;

/* Functions for reading and writing different file formats.*/

int		aiff_open_read	(SF_PRIVATE *psf) ;
int		aiff_open_write	(SF_PRIVATE *psf) ;

int		au_open_read	(SF_PRIVATE *psf) ;
int		au_nh_open_read	(SF_PRIVATE *psf) ;	/* Headerless version of AU. */
int		au_open_write	(SF_PRIVATE *psf) ;

int		wav_open_read	(SF_PRIVATE *psf) ;
int		wav_open_write	(SF_PRIVATE *psf) ;

int		raw_open_read	(SF_PRIVATE *psf) ;
int		raw_open_write	(SF_PRIVATE *psf) ;

int		paf_open_read	(SF_PRIVATE *psf) ;
int		paf_open_write	(SF_PRIVATE *psf) ;

int		svx_open_read	(SF_PRIVATE *psf) ;
int		svx_open_write	(SF_PRIVATE *psf) ;

int		nist_open_read	(SF_PRIVATE *psf) ;
int		nist_open_write	(SF_PRIVATE *psf) ;

int		smpltd_open_read	(SF_PRIVATE *psf) ;
int		smpltd_open_write	(SF_PRIVATE *psf) ;

int		voc_open_read	(SF_PRIVATE *psf) ;
int		voc_open_write	(SF_PRIVATE *psf) ;

int		rx2_open_read	(SF_PRIVATE *psf) ;
int		rx2_open_write	(SF_PRIVATE *psf) ;

int		ircam_open_read		(SF_PRIVATE *psf) ;
int		ircam_open_write	(SF_PRIVATE *psf) ;


/*	Win32 does seem to have snprintf and vsnprintf but prepends
**	the names with an underscore. Why?
*/

#ifdef	WIN32
#define	snprintf	_snprintf
#define	vsnprintf	_vsnprintf
#endif

#ifdef _WIN32
	#pragma pack(pop,1)
#endif

#endif /* COMMON_H_INCLUDED */

