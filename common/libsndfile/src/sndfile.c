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

#include	<stdio.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<string.h>
#include	<ctype.h>
#include	<math.h>

/* EMX / gcc compiler for OS/2. */
#ifdef __EMX__
#include	<sys/types.h>
#endif

/* For the Metrowerks CodeWarrior Pro Compiler (mainly MacOS) */
#if	(defined (__MWERKS__))
#include	<stat.h>
#else
#include	<sys/stat.h>
#endif

#include	"sndfile.h"
#include	"config.h"
#include	"sfendian.h"
#include	"common.h"

#define		SNDFILE_MAGICK	0x1234C0DE

typedef struct
{	int 	error ;
	char	*str ;
} ErrorStruct ;

static
ErrorStruct SndfileErrors [] = 
{ 
	{	SFE_NO_ERROR			, "No Error." },
	{	SFE_BAD_FILE			, "File does not exist or is not a regular file (possibly a pipe?)." },
	{	SFE_BAD_FILE_READ		, "File exists but no data could be read." },
	{	SFE_OPEN_FAILED			, "Could not open file." },
	{	SFE_BAD_OPEN_FORMAT		, "Bad format specified for file open." },
	{	SFE_BAD_SNDFILE_PTR		, "Not a valid SNDFILE* pointer." },
	{	SFE_BAD_SF_INFO_PTR		, "NULL SF_INFO pointer passed to libsndfile." },
	{	SFE_BAD_SF_INCOMPLETE	, "SF_PRIVATE struct incomplete and end of header parsing." },
	{	SFE_BAD_FILE_PTR		, "Bad FILE pointer." },
	{	SFE_BAD_INT_PTR			, "Internal error, Bad pointer." },
	{	SFE_MALLOC_FAILED		, "Internal malloc () failed." },
	{	SFE_UNIMPLEMENTED		, "File contains data in an unimplemented format." },
	{	SFE_BAD_READ_ALIGN  	, "Attempt to read a non-integer number of channels." },
	{	SFE_BAD_WRITE_ALIGN 	, "Attempt to write a non-integer number of channels." },
	{	SFE_UNKNOWN_FORMAT		, "File contains data in an unknown format." },
	{	SFE_NOT_READMODE		, "Read attempted on file currently open for write." },
	{	SFE_NOT_WRITEMODE		, "Write attempted on file currently open for read." },
	{	SFE_BAD_MODE_RW			, "This file format does not support read/write mode." },
	{	SFE_BAD_SF_INFO			, "Internal error : SF_INFO struct incomplete." },

	{	SFE_SHORT_READ			, "Short read error." },
	{	SFE_SHORT_WRITE			, "Short write error." },
	{	SFE_INTERNAL			, "Unspecified internal error." },
	{	SFE_LOG_OVERRUN			, "Log buffer has overrun. File probably broken." },
	{	SFE_BAD_CONTROL_CMD		, "Bad command passed to function sf_command()." },
	{	SFE_BAD_ENDIAN			, "Bad endian-ness. Try default endian-ness" },
	{	SFE_CHANNEL_COUNT		, "Too many channels specified." },
	
	{	SFE_BAD_SEEK			, "Internal psf_fseek() failed." },
	{	SFE_NOT_SEEKABLE		, "Seek attempted on unseekable file type." },
	{	SFE_AMBIGUOUS_SEEK		, "Error: combination of file open mode and seek command is ambiguous." },
	{	SFE_WRONG_SEEK			, "Error: invalid seek parameters." },
	{	SFE_SEEK_FAILED			, "Error: parameters OK, but psf_seek() failed." },

	{	SFE_BAD_OPEN_MODE		, "Error: bad mode parameter for file open." },		
	{	SFE_OPEN_PIPE_RDWR		, "Error: attempt toopen a pipe in read/write mode." },
	{	SFE_RDWR_POSITION		, "Error on RDWR position (cryptic)." },

	{	SFE_WAV_NO_RIFF			, "Error in WAV file. No 'RIFF' chunk marker." },
	{	SFE_WAV_NO_WAVE			, "Error in WAV file. No 'WAVE' chunk marker." },
	{	SFE_WAV_NO_FMT			, "Error in WAV file. No 'fmt ' chunk marker." },
	{	SFE_WAV_FMT_SHORT		, "Error in WAV file. Short 'fmt ' chunk." },

	{	SFE_WAV_FMT_TOO_BIG		, "Error in WAV file. 'fmt ' chunk too large." },
	{	SFE_WAV_BAD_FACT		, "Error in WAV file. 'fact' chunk out of place." },
	{	SFE_WAV_BAD_PEAK		, "Error in WAV file. Bad 'PEAK' chunk." },
	{	SFE_WAV_PEAK_B4_FMT		, "Error in WAV file. 'PEAK' chunk found before 'fmt ' chunk." },

	{	SFE_WAV_BAD_FORMAT		, "Error in WAV file. Errors in 'fmt ' chunk." },
	{	SFE_WAV_BAD_BLOCKALIGN	, "Error in WAV file. Block alignment in 'fmt ' chunk is incorrect." },
	{	SFE_WAV_NO_DATA			, "Error in WAV file. No 'data' chunk marker." },
	{	SFE_WAV_UNKNOWN_CHUNK	, "Error in WAV file. File contains an unknown chunk marker." },
	
	{	SFE_WAV_ADPCM_NOT4BIT	, "Error in ADPCM WAV file. Invalid bit width." },
	{	SFE_WAV_ADPCM_CHANNELS	, "Error in ADPCM WAV file. Invalid number of channels." },
	{	SFE_WAV_GSM610_FORMAT	, "Error in GSM610 WAV file. Invalid format chunk." },
  
	{	SFE_AIFF_NO_FORM		, "Error in AIFF file, bad 'FORM' marker." },
	{	SFE_AIFF_AIFF_NO_FORM	, "Error in AIFF file, 'AIFF' marker without 'FORM'." },
	{	SFE_AIFF_COMM_NO_FORM	, "Error in AIFF file, 'COMM' marker without 'FORM'." },
	{	SFE_AIFF_SSND_NO_COMM	, "Error in AIFF file, 'SSND' marker without 'COMM'." },
	{	SFE_AIFF_UNKNOWN_CHUNK	, "Error in AIFF file, unknown chunk." },
	{	SFE_AIFF_COMM_CHUNK_SIZE, "Error in AIFF file, bad 'COMM' chunk size." },
	{	SFE_AIFF_BAD_COMM_CHUNK , "Error in AIFF file, bad 'COMM' chunk." },
	{	SFE_AIFF_PEAK_B4_COMM	, "Error in AIFF file. 'PEAK' chunk found before 'COMM' chunk." },
	{	SFE_AIFF_BAD_PEAK		, "Error in AIFF file. Bad 'PEAK' chunk." },
	{	SFE_AIFF_NO_SSND		, "Error in AIFF file, bad 'SSND' chunk." },
	{	SFE_AIFF_NO_DATA		, "Error in AIFF file, no sound data." },
	
	{	SFE_AU_UNKNOWN_FORMAT	, "Error in AU file, unknown format." },
	{	SFE_AU_NO_DOTSND		, "Error in AU file, missing '.snd' or 'dns.' marker." },

	{	SFE_RAW_READ_BAD_SPEC	, "Error while opening RAW file for read. Must specify format and channels.\n"
									"Possibly trying to open unsupported format."
									 },
	{	SFE_RAW_BAD_BITWIDTH	, "Error. RAW file bitwidth must be a multiple of 8." },

	{	SFE_PAF_NO_MARKER		, "Error in PAF file, no marker." },
	{	SFE_PAF_VERSION			, "Error in PAF file, bad version." }, 
	{	SFE_PAF_UNKNOWN_FORMAT	, "Error in PAF file, unknown format." }, 
	{	SFE_PAF_SHORT_HEADER	, "Error in PAF file. File shorter than minimal header." },
	
	{	SFE_SVX_NO_FORM			, "Error in 8SVX / 16SV file, no 'FORM' marker." }, 
	{	SFE_SVX_NO_BODY			, "Error in 8SVX / 16SV file, no 'BODY' marker." }, 
	{	SFE_SVX_NO_DATA			, "Error in 8SVX / 16SV file, no sound data." },
	{	SFE_SVX_BAD_COMP		, "Error in 8SVX / 16SV file, unsupported compression format." },
	
	{	SFE_NIST_BAD_HEADER		, "Error in NIST file, bad header." },
	{	SFE_NIST_BAD_ENCODING	, "Error in NIST file, unsupported compression format." },

	{	SFE_SMTD_NO_SEKD		, "Error in Samplitude file, no 'SEKD' marker." },  
	{	SFE_SMTD_NO_SAMR		, "Error in Samplitude file, no 'SAMR' marker." }, 

	{	SFE_VOC_NO_CREATIVE		, "Error in VOC file, no 'Creative Voice File' marker." }, 
	{	SFE_VOC_BAD_FORMAT		, "Error in VOC file, bad format." }, 
	{	SFE_VOC_BAD_VERSION		, "Error in VOC file, bad version number." }, 
	{	SFE_VOC_BAD_MARKER		, "Error in VOC file, bad marker in file." }, 
	{	SFE_VOC_BAD_SECTIONS	, "Error in VOC file, incompatible VOC sections." }, 
	{	SFE_VOC_MULTI_SAMPLERATE, "Error in VOC file, more than one sample rate defined." }, 
	{	SFE_VOC_MULTI_SECTION	, "Unimplemented VOC file feature, file contains multiple sound sections." },
	{	SFE_VOC_MULTI_PARAM		, "Error in VOC file, file contains multiple bit or channel widths." },
	{	SFE_VOC_SECTION_COUNT	, "Error in VOC file, too many sections." }, 

	{	SFE_IRCAM_NO_MARKER		, "Error in IRCAM file, bad IRCAM marker." }, 
	{	SFE_IRCAM_BAD_CHANNELS	, "Error in IRCAM file, bad channel count." }, 
	{	SFE_IRCAM_UNKNOWN_FORMAT, "Error in IRCAM file, unknow encoding format." }, 

	{	SFE_W64_64_BIT			, "Error in W64 file, file contains 64 bit offset." },

	{	SFE_W64_NO_RIFF			, "Error in W64 file. No 'riff' chunk marker." },
	{	SFE_W64_NO_WAVE			, "Error in W64 file. No 'wave' chunk marker." },
	{	SFE_W64_NO_FMT			, "Error in W64 file. No 'fmt ' chunk marker." },
	{	SFE_W64_NO_DATA			, "Error in W64 file. No 'data' chunk marker." },

	{	SFE_W64_FMT_SHORT		, "Error in W64 file. Short 'fmt ' chunk." },
	{	SFE_W64_FMT_TOO_BIG		, "Error in W64 file. 'fmt ' chunk too large." },
	
	{	SFE_W64_ADPCM_NOT4BIT	, "Error in ADPCM W64 file. Invalid bit width." },
	{	SFE_W64_ADPCM_CHANNELS	, "Error in ADPCM W64 file. Invalid number of channels." },
	{	SFE_W64_GSM610_FORMAT	, "Error in GSM610 W64 file. Invalid format chunk." },

	{	SFE_DWVW_BAD_BITWIDTH	, "Error : Bad bit width for DWVW encoding. Must be 12, 16 or 24." },

	{	SFE_MAX_ERROR			, "Maximum error number." }
} ;

/*------------------------------------------------------------------------------
*/

static int 	does_extension_match (const char *ext, const char *test) ;
static int 	is_au_snd_file (const char *filename) ;
static int	guess_file_type (SF_PRIVATE *psf, const char *filename) ;
static int	validate_sfinfo (SF_INFO *sfinfo) ;
static int	validate_psf (SF_PRIVATE *psf) ;
static void save_header_info (SF_PRIVATE *psf) ;
static void	copy_filename (SF_PRIVATE *psf, const char *path) ;

/*-static int	hash_command (const char *cmd) ;-*/

/*------------------------------------------------------------------------------
** Private (static) variables.
*/

static	int		sf_errno = 0 ;
static	char	sf_logbuffer [SF_BUFFER_LEN] = { 0 } ;

/*------------------------------------------------------------------------------
*/

#define	VALIDATE_SNDFILE_AND_ASSIGN_PSF(a,b)		\
		{	if (! (a))								\
				return SFE_BAD_SNDFILE_PTR ;		\
			(b) = (SF_PRIVATE*) (a) ;				\
			if (((b)->filedes) < 0)					\
				return SFE_BAD_FILE_PTR ;			\
			if ((b)->Magick != SNDFILE_MAGICK)		\
				return	SFE_BAD_SNDFILE_PTR ;		\
			(b)->error = 0 ;						\
			} 

/*------------------------------------------------------------------------------
**	Public functions.
*/

SNDFILE*
sf_open	(const char *path, int mode, SF_INFO *sfinfo)
{	SF_PRIVATE 	*psf ;
	int			error ;

	if (mode != SFM_READ && mode != SFM_WRITE && mode != SFM_RDWR)
	{	sf_errno = SFE_BAD_OPEN_MODE ;
		return	NULL ;
		} ;
	
	if (! path)
	{	sf_errno = SFE_BAD_FILE ;
		return	NULL ;
		} ;
	
	if (! sfinfo)
	{	sf_errno = SFE_BAD_SF_INFO_PTR ;
		return	NULL ;
		} ;
		
	sf_errno = error = 0 ;
	sf_logbuffer [0] = 0 ;
	
	if (! (psf = malloc (sizeof (SF_PRIVATE))))
	{	sf_errno = SFE_MALLOC_FAILED ;
		return	NULL ;
		} ;

	memset (psf, 0, sizeof (SF_PRIVATE)) ;

	psf_log_printf (psf, "File : %s\n", path) ;
	copy_filename (psf, path) ;

	psf->Magick 		= SNDFILE_MAGICK ;
	psf->norm_float 	= SF_TRUE ;
	psf->norm_double	= SF_TRUE ;
	psf->mode 			= mode ;
	psf->filelength		= -1 ;
	psf->filedes		= -1 ;
	psf->dataoffset		= -1 ;
	psf->datalength		= -1 ;
	psf->read_current	= -1 ;
	psf->write_current	= -1 ;

	psf->new_seek		= psf_default_seek ;
	psf->sf.format		= sfinfo->format ;

	if (strcmp (path, "-") == 0)
	{	/* File is a either stdin or stdout. */
		psf->sf.seekable = SF_FALSE ;
		switch (mode)
		{	case SFM_RDWR :
					error = SFE_OPEN_PIPE_RDWR ;
					break ;

			case SFM_READ :
					psf->filedes = 0 ;
					break ;

			case SFM_WRITE :
					psf->filedes = 1 ;
					break ;

			default :
					error = SFE_BAD_OPEN_MODE ;
					break ;
			} ;
		psf->filelength = SF_MAX_COUNT ;
		}
	else if ((psf->filedes = psf_open (path, mode)) < 0)
		error = SFE_OPEN_FAILED ;

	if (error)
	{	if (psf->filedes >= 0)
			psf_fclose (psf->filedes) ;
		sf_close (psf) ;
		sf_errno = error ;
		return NULL ;
		} ;
		
	/* File is open, so get the length. */
	psf->filelength = psf_get_filelen (psf->filedes) ;

	memcpy (&(psf->sf), sfinfo, sizeof (SF_INFO)) ;

	psf->sf.seekable = SF_TRUE ;
	psf->sf.sections = 1 ;

	if (mode == SFM_WRITE || (mode == SFM_RDWR && psf->filelength == 0))
	{	/* If the file is being opened for write or RDWR and the file is currently
		** empty, then the SF_INFO struct must contain valid data.
		*/ 
		if (! sf_format_check (sfinfo))
			error = SFE_BAD_OPEN_FORMAT ;
		}
	else if ((psf->sf.format & SF_FORMAT_TYPEMASK) != SF_FORMAT_RAW)
	{	/* If type RAW has not been specified then need to figure out file type. */
		psf->sf.format = guess_file_type (psf, psf->filename) ;
		}  ;
	
	if (error)
	{	if (psf->filedes >= 0)
			psf_fclose (psf->filedes) ;
		sf_close (psf) ;
		sf_errno = error ;
		return NULL ;
		} ;

	/* Set bytewidth if known. */
	switch (sfinfo->format & SF_FORMAT_SUBMASK)
	{	case SF_FORMAT_PCM_S8 :
		case SF_FORMAT_PCM_U8 : 
		case SF_FORMAT_ULAW : 
		case SF_FORMAT_ALAW : 
				psf->bytewidth = 1 ;
				break ;

		case SF_FORMAT_PCM_16 : 
				psf->bytewidth = 2 ;
				break ;

		case SF_FORMAT_PCM_24 : 
				psf->bytewidth = 3 ;
				break ;

		case SF_FORMAT_PCM_32 : 
		case SF_FORMAT_FLOAT : 
				psf->bytewidth = 4 ;
				break ;

		case SF_FORMAT_DOUBLE : 
				psf->bytewidth = 8 ;
				break ;
		} ;

	/* Call the initialisation function for the relevant file type. */
	switch (psf->sf.format & SF_FORMAT_TYPEMASK)
	{	case	SF_FORMAT_WAV :
				error = wav_open (psf) ;
				break ;

		case	SF_FORMAT_AIFF :
				error = aiff_open (psf) ;
				break ;

		case	SF_FORMAT_AU :
				error = au_open (psf) ;
				break ;

		case	SF_FORMAT_AU | SF_FORMAT_ULAW :
				error = au_nh_open (psf) ;
				break ;

		case	SF_FORMAT_RAW :
				error = raw_open (psf) ;
				break ;

		case	SF_FORMAT_PAF :
				error = paf_open (psf) ;
				break ;

		case	SF_FORMAT_SVX :
				error = svx_open (psf) ;
				break ;

		case	SF_FORMAT_NIST :
				error = nist_open (psf) ;
				break ;

		case	SF_FORMAT_SMPLTD :
				error = smpltd_open (psf) ;
				break ;

		case	SF_FORMAT_IRCAM :
				error = ircam_open (psf) ;
				break ;

		case	SF_FORMAT_VOC :
				error = voc_open (psf) ;
				break ;

		case	SF_FORMAT_W64 :
				error = w64_open (psf) ;
				break ;

		case	SF_FORMAT_SD2 :
				error = sd2_open (psf) ;
				break ;

		case	SF_FORMAT_REX2 :
				error = rx2_open (psf) ;
				break ;

		case	SF_FORMAT_TXW :
				error = txw_open (psf) ;
				break ;

		default :	
				error = SFE_UNKNOWN_FORMAT ;
		} ;
	
	if (error)
	{	sf_errno = error ;
		save_header_info (psf) ;
		sf_close (psf) ;
		return NULL ;
		} ;

	if (! validate_sfinfo (&(psf->sf)))
	{
#ifndef REMOVE_ME
printf ("validate_sfinfo\n") ;
puts (psf->logbuffer) ;
#endif
		psf_log_SF_INFO (psf) ;
		save_header_info (psf) ;
		sf_errno = SFE_BAD_SF_INFO ;
		sf_close (psf) ;
		return NULL ;
		} ;
		
	if (! validate_psf (psf))
	{	
#ifndef REMOVE_ME
printf ("validate_psf\n") ;
puts (psf->logbuffer) ;
#endif
		save_header_info (psf) ;
		sf_errno = SFE_INTERNAL ;
		sf_close (psf) ;
		return NULL ;
		} ;

	psf->read_current  = 0 ;
	psf->write_current=  (psf->mode == SFM_RDWR) ? psf->sf.samples : 0 ;

	memcpy (sfinfo, &(psf->sf), sizeof (SF_INFO)) ;

	return (SNDFILE*) psf ;
} /* sf_open */

/*------------------------------------------------------------------------------
*/

int
sf_error	(SNDFILE *sndfile)
{	SF_PRIVATE	*psf ;
	
	if (! sndfile)
	{	if (sf_error)
			return 1 ;
		return 0 ;
		} ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile, psf) ;
	
	if (psf->error)
		return 1 ;
	
	return 0 ;
} /* sf_error */

/*------------------------------------------------------------------------------
*/

int	
sf_perror	(SNDFILE *sndfile)
{	SF_PRIVATE 	*psf ;
	int 		k, errnum ;

	if (! sndfile)
	{	errnum = sf_errno ;
		}
	else
	{	VALIDATE_SNDFILE_AND_ASSIGN_PSF(sndfile,psf) ;
		errnum = psf->error ;
		} ;
		
	errnum = abs (errnum) ;
	errnum = (errnum >= SFE_MAX_ERROR || errnum < 0) ? 0 : errnum ;

	for (k = 0 ; SndfileErrors[k].str ; k++)
		if (errnum == SndfileErrors[k].error)
		{	fprintf (stderr, "%s\n", SndfileErrors[k].str) ;
			return SFE_NO_ERROR ;
			} ;
	
	fprintf (stderr, "No error string for error number %d.\n", errnum) ;
	return SFE_NO_ERROR ;
} /* sf_perror */


/*------------------------------------------------------------------------------
*/

int	
sf_error_str	(SNDFILE *sndfile, char *str, size_t maxlen)
{	SF_PRIVATE 	*psf ;
	int 		errnum, k ;

	if (! sndfile)
	{	errnum = sf_errno ;
		}
	else
	{	VALIDATE_SNDFILE_AND_ASSIGN_PSF(sndfile,psf) ;
		errnum = psf->error ;
		} ;
		
	errnum = abs (errnum) ;
	errnum = (errnum >= SFE_MAX_ERROR || errnum < 0) ? 0 : errnum ;

	for (k = 0 ; SndfileErrors[k].str ; k++)
		if (errnum == SndfileErrors[k].error)
		{	if (str)
			{	strncpy (str, SndfileErrors [errnum].str, maxlen) ;
				str [maxlen-1] = 0 ;
				} ;
			return SFE_NO_ERROR ;
			} ;
			
	if (str)
	{	strncpy (str, "No error defined for this error number. This is a bug in libsndfile.", maxlen) ;		
		str [maxlen-1] = 0 ;
		} ;
			
	return SFE_NO_ERROR ;
} /* sf_error_str */

/*------------------------------------------------------------------------------
*/

int	
sf_error_number	(int errnum, char *str, size_t maxlen)
{	int 		k ;

	errnum = abs (errnum) ;
	errnum = (errnum >= SFE_MAX_ERROR || errnum < 0) ? 0 : errnum ;

	for (k = 0 ; SndfileErrors[k].str ; k++)
		if (errnum == SndfileErrors[k].error)
		{	strncpy (str, SndfileErrors [errnum].str, maxlen) ;
			str [maxlen-1] = 0 ;
			return SFE_NO_ERROR ;
			} ;
			
	strncpy (str, "No error defined for this error number. This is a bug in libsndfile.", maxlen) ;		
	str [maxlen-1] = 0 ;
			
	return SFE_NO_ERROR ;
} /* sf_error_number */

/*------------------------------------------------------------------------------
*/

int
sf_format_check	(const SF_INFO *info)
{	int	subformat, endian ;

	subformat = info->format & SF_FORMAT_SUBMASK ;
	endian = info->format & SF_FORMAT_SUBMASK ;

	/* This is the place where each file format can check if the suppiled
	** SF_INFO struct is valid.
	** Return 0 on failure, 1 ons success. 
	*/
	
	if (info->channels < 1 || info->channels > 256)
		return 0 ;

	switch (info->format & SF_FORMAT_TYPEMASK)
	{	case SF_FORMAT_WAV :
				/* WAV is strictly little endian. */
				if (endian == SF_ENDIAN_BIG || endian == SF_ENDIAN_CPU)
					return 0 ;
				if (subformat == SF_FORMAT_PCM_U8 || subformat == SF_FORMAT_PCM_16)
					return 1 ;
				if (subformat == SF_FORMAT_PCM_24 || subformat == SF_FORMAT_PCM_32)
					return 1 ;
				if ((subformat == SF_FORMAT_IMA_ADPCM || subformat == SF_FORMAT_MS_ADPCM) && info->channels <= 2)
					return 1 ;
				if (subformat == SF_FORMAT_GSM610 && info->channels == 1)
					return 1 ;
				if (subformat == SF_FORMAT_ULAW || subformat == SF_FORMAT_ALAW)
					return 1 ;
				if (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE)
					return 1 ;
				break ;
				
		case SF_FORMAT_AIFF :
				/* AIFF does allow both endian-nesses for PCM data.*/
				if (subformat == SF_FORMAT_PCM_16 || subformat == SF_FORMAT_PCM_24 || subformat == SF_FORMAT_PCM_32)
					return 1 ;
				/* Other encodings. Check for endian-ness. */
				if (endian == SF_ENDIAN_LITTLE || endian == SF_ENDIAN_CPU)
					return 0 ;
				if (subformat == SF_FORMAT_PCM_U8 || subformat == SF_FORMAT_PCM_S8)
					return 1 ;
				if (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE)
					return 1 ;
				if (subformat == SF_FORMAT_ULAW || subformat == SF_FORMAT_ALAW)
					return 1 ;
				if ((subformat == SF_FORMAT_DWVW_12 || subformat == SF_FORMAT_DWVW_16 || 
							subformat == SF_FORMAT_DWVW_24) && info-> channels == 1)
					return 1 ;
				break ;
				
		case SF_FORMAT_AU :
				if (subformat == SF_FORMAT_PCM_S8 || subformat == SF_FORMAT_PCM_16)
					return 1 ;
				if (subformat == SF_FORMAT_PCM_24 || subformat == SF_FORMAT_PCM_32)
					return 1 ;
				if (subformat == SF_FORMAT_ULAW || subformat == SF_FORMAT_ALAW)
					return 1 ;
				if (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE)
					return 1 ;
				if (subformat == SF_FORMAT_G721_32 && info->channels == 1)
					return 1 ;
				if (subformat == SF_FORMAT_G723_24 && info->channels == 1)
					return 1 ;
				break ;
				
		case SF_FORMAT_RAW :
				if (subformat == SF_FORMAT_PCM_U8 || subformat == SF_FORMAT_PCM_S8 || subformat == SF_FORMAT_PCM_16)
					return 1 ;
				if (subformat == SF_FORMAT_PCM_24 || subformat == SF_FORMAT_PCM_32)
					return 1 ;
				if (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE)
					return 1 ;
				if (subformat == SF_FORMAT_ALAW || subformat == SF_FORMAT_ULAW)
					return 1 ;
				if ((subformat == SF_FORMAT_DWVW_12 || subformat == SF_FORMAT_DWVW_16 || 
							subformat == SF_FORMAT_DWVW_24) && info-> channels == 1)
					return 1 ;
				if (subformat == SF_FORMAT_GSM610 && info->channels == 1)
					return 1 ;
				break ;

		case SF_FORMAT_PAF :
				if (subformat == SF_FORMAT_PCM_S8 || subformat == SF_FORMAT_PCM_16)
					return 1 ;
				if (subformat == SF_FORMAT_PCM_24 || subformat == SF_FORMAT_PCM_32)
					return 1 ;
				break ;

		case SF_FORMAT_SVX :
				/* SVX currently does not support more than one channel. */
				if (info->channels != 1)
					return 0 ;
				/* Always big endian. */
				if (endian == SF_ENDIAN_LITTLE || endian == SF_ENDIAN_CPU)
					return 0 ;

				if ((subformat == SF_FORMAT_PCM_S8 || subformat == SF_FORMAT_PCM_16) && info->channels == 1)
					return 1 ;
				break ;

		case SF_FORMAT_NIST :
				if (subformat == SF_FORMAT_PCM_S8 || subformat == SF_FORMAT_PCM_16)
					return 1 ;
				if (subformat == SF_FORMAT_PCM_24 || subformat == SF_FORMAT_PCM_32)
					return 1 ;
				if (subformat == SF_FORMAT_ULAW || subformat == SF_FORMAT_ALAW)
					return 1 ;
				break ;
				
		case SF_FORMAT_IRCAM :
				if (subformat == SF_FORMAT_PCM_16 || subformat == SF_FORMAT_PCM_24 || subformat == SF_FORMAT_PCM_32)
					return 1 ;
				if (subformat == SF_FORMAT_ULAW || subformat == SF_FORMAT_ALAW || subformat == SF_FORMAT_FLOAT)
					return 1 ;
				break ;
				
		case SF_FORMAT_VOC :
				/* VOC is strictly little endian. */
				if (endian == SF_ENDIAN_BIG || endian == SF_ENDIAN_CPU)
					return 0 ;
				if (subformat == SF_FORMAT_PCM_U8 || subformat == SF_FORMAT_PCM_16)
					return 1 ;
				if (subformat == SF_FORMAT_ULAW || subformat == SF_FORMAT_ALAW)
					return 1 ;
				break ;
				
		case SF_FORMAT_W64 :
				/* W64 is strictly little endian. */
				if (endian == SF_ENDIAN_BIG || endian == SF_ENDIAN_CPU)
					return 0 ;
				if (subformat == SF_FORMAT_PCM_U8 || subformat == SF_FORMAT_PCM_16)
					return 1 ;
				if (subformat == SF_FORMAT_PCM_24 || subformat == SF_FORMAT_PCM_32)
					return 1 ;
				if ((subformat == SF_FORMAT_IMA_ADPCM || subformat == SF_FORMAT_MS_ADPCM) && info->channels <= 2)
					return 1 ;
				if (subformat == SF_FORMAT_GSM610 && info->channels == 1)
					return 1 ;
				if (subformat == SF_FORMAT_ULAW || subformat == SF_FORMAT_ALAW)
					return 1 ;
				if (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE)
					return 1 ;
				break ;
				
		case SF_FORMAT_SD2 :
				/* SD2 is strictly big endian. */
				if (endian == SF_ENDIAN_LITTLE || endian == SF_ENDIAN_CPU)
					return 0 ;
				if (subformat == SF_FORMAT_PCM_16)
					return 1 ;
				break ;
				
		default : break ;
		} ;

	return 0 ;
} /* sf_format_check */

/*------------------------------------------------------------------------------
*/

int	
sf_command	(SNDFILE *sndfile, int command, void *data, int datasize)
{	SF_PRIVATE 		*psf = NULL ;
	
	/*-command = hash_command (cmd) ;-*/
	
	/* This set of commands do not need the sndfile parameter. */
	switch (command)
	{	case  SFC_GET_LIB_VERSION :
			strncpy (data, PACKAGE "-" VERSION, datasize - 1) ;
		((char*) data) [datasize - 1] = 0 ;
		return 0 ;

		case SFC_GET_SIMPLE_FORMAT_COUNT :
			if (! data || datasize != SIGNED_SIZEOF (int))
				return (sf_errno = SFE_BAD_CONTROL_CMD) ;
			*((int*) data) = psf_get_format_simple_count () ;
			return 0 ;
	
		case SFC_GET_SIMPLE_FORMAT :
			if (! data || datasize != SIGNED_SIZEOF (SF_FORMAT_INFO))
				return (sf_errno = SFE_BAD_CONTROL_CMD) ;
			return psf_get_format_simple (data) ;

		case SFC_GET_FORMAT_MAJOR_COUNT :
			if (! data || datasize != SIGNED_SIZEOF (int))
				return (sf_errno = SFE_BAD_CONTROL_CMD) ;
			*((int*) data) = psf_get_format_major_count () ;
			return 0 ;
		
		case SFC_GET_FORMAT_MAJOR :
			if (! data || datasize != SIGNED_SIZEOF (SF_FORMAT_INFO))
				return (sf_errno = SFE_BAD_CONTROL_CMD) ;
			return psf_get_format_major (data) ;

		case SFC_GET_FORMAT_SUBTYPE_COUNT :
			if (! data || datasize != SIGNED_SIZEOF (int))
				return (sf_errno = SFE_BAD_CONTROL_CMD) ;
			*((int*) data) = psf_get_format_subtype_count () ;
			return 0 ;
		
		case SFC_GET_FORMAT_SUBTYPE :
			if (! data || datasize != SIGNED_SIZEOF (SF_FORMAT_INFO))
				return (sf_errno = SFE_BAD_CONTROL_CMD) ;
			return psf_get_format_subtype (data) ;

		} ;
		
	if (! sndfile && command == SFC_GET_LOG_INFO)
	{	if (! data)
			return (psf->error = SFE_BAD_CONTROL_CMD) ;
		strncpy (data, sf_logbuffer, datasize) ;
		((char*) data) [datasize - 1] = 0 ;
		return 0 ;
		} ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile, psf) ;

	switch (command)
	{	case SFC_SET_NORM_FLOAT :
			psf->norm_float = (datasize) ? SF_TRUE : SF_FALSE ;
			break ;		

		case SFC_SET_NORM_DOUBLE :
			psf->norm_double = (datasize) ? SF_TRUE : SF_FALSE ;
			break ;

		case SFC_GET_NORM_FLOAT :
			return psf->norm_float ;

		case SFC_GET_NORM_DOUBLE :
			return psf->norm_double ;

		case SFC_GET_LOG_INFO :
			if (! data)
				return (psf->error = SFE_BAD_CONTROL_CMD) ;
			strncpy (data, psf->logbuffer, datasize) ;
			((char*) data) [datasize - 1] = 0 ;
			break ;

		case SFC_CALC_SIGNAL_MAX :
			if (! data || datasize != sizeof (double))
				return (psf->error = SFE_BAD_CONTROL_CMD) ;
			*((double*) data) = psf_calc_signal_max (psf) ;
			break ;

		default :
			/* Must be a file specific command. Pass it on. */
			if (psf->command)
				return psf->command (psf, command, data, datasize) ;

			psf_log_printf (psf, "*** sf_command : cmd = 0x%X\n", command) ;
			return (psf->error = SFE_BAD_CONTROL_CMD) ;
		} ;

	return 0 ;
} /* sf_command */

/*------------------------------------------------------------------------------
*/

sf_count_t	
sf_seek	(SNDFILE *sndfile, sf_count_t offset, int whence)
{	SF_PRIVATE 	*psf ;
	sf_count_t	seek_from_start = 0, retval ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile, psf) ;
	
	if (! psf->sf.seekable)
	{	psf->error = SFE_NOT_SEEKABLE ;
		return	((sf_count_t) -1) ;
		} ;
	
	/* If the whence parameter has a mode ORed in, check to see that 
	** it makes sense.
	*/
	if (((whence & SFM_MASK) == SFM_WRITE && psf->mode == SFM_READ) ||
			((whence & SFM_MASK) == SFM_WRITE && psf->mode == SFM_WRITE))
	{	psf->error = SFE_WRONG_SEEK ;
		return ((sf_count_t) -1) ;
		} ;
	
	/* Convert all SEEK_CUR and SEEK_END into seek_from_start to be 
	** used with SEEK_SET.
	*/
	switch (whence)
	{	/* The SEEK_SET behaviour is independant of mode. */
		case SEEK_SET :
		case SEEK_SET | SFM_READ :
		case SEEK_SET | SFM_WRITE :
		case SEEK_SET | SFM_RDWR :
				seek_from_start = offset ;
				break ;
				
		/* The SEEK_CUR is a little more tricky. */
		case SEEK_CUR :
				if (psf->mode == SFM_READ)
					seek_from_start = psf->read_current + offset ;
				else if (psf->mode == SFM_WRITE)	
					seek_from_start = psf->write_current + offset ;
				else
					psf->error = SFE_AMBIGUOUS_SEEK ;
				break ;
 
		case SEEK_CUR | SFM_READ :
				/*=====================================
				if (offset == 0)
					return psf->read_current ;
				=====================================*/
				seek_from_start = psf->read_current + offset ;
				break ;
			
		case SEEK_CUR | SFM_WRITE :
				seek_from_start = psf->write_current + offset ;
				break ;
			
		/* The SEEK_END */
		case SEEK_END :
				if (psf->mode != SFM_READ && psf->mode != SFM_WRITE)
					psf->error = SFE_AMBIGUOUS_SEEK ;
				else
					seek_from_start = psf->sf.samples + offset ;
				break ;

		case SEEK_END | SFM_READ :
				seek_from_start = psf->sf.samples + offset ;
				/*=======================================
				if (seek_from_start == psf->read_current)
					return psf->read_current ;
				=======================================*/
				break ;
		case SEEK_END | SFM_WRITE :
				seek_from_start = psf->sf.samples + offset ;
				break ;

		default :
				psf->error = SFE_BAD_SEEK ;
				break ;
		} ;	

	if (psf->error)
		return ((sf_count_t) -1) ;

	if (seek_from_start < 0 || seek_from_start > psf->sf.samples)
	{	psf->error = SFE_BAD_SEEK ;
		return ((sf_count_t) -1) ;
		} ;

	if (psf->new_seek)
	{	int new_mode = (whence & SFM_MASK) ? (whence & SFM_MASK) : psf->mode ;
	
		retval = psf->new_seek (psf, new_mode, seek_from_start) ;
		
		switch (new_mode)
		{	case SFM_READ :
					psf->read_current = retval ;
					break ;
			case SFM_WRITE :
					psf->write_current = retval ;
					break ;
			case SFM_RDWR :
					psf->read_current = retval ;
					psf->write_current = retval ;
					new_mode = SFM_READ ;
					break ;
			} ;
		
		psf->last_op = new_mode ;

		return retval ;
		} ;
	
	psf->error = SFE_AMBIGUOUS_SEEK ;
	return ((sf_count_t) -1) ;
} /* sf_seek */

/*------------------------------------------------------------------------------
*/

sf_count_t
sf_read_raw		(SNDFILE *sndfile, void *ptr, sf_count_t bytes)
{	SF_PRIVATE 	*psf ;
	sf_count_t	count ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile, psf) ;
	
	if (psf->mode == SFM_WRITE)
	{	psf->error = SFE_NOT_READMODE ;
		return	0 ;
		} ;

	if (bytes < 0 || psf->read_current >= psf->datalength)
	{	memset (ptr, 0, bytes) ;
		return 0 ;
		} ;

	if (bytes % (psf->sf.channels * psf->bytewidth))
	{	psf->error = SFE_BAD_READ_ALIGN ;
		return 0 ;
		} ;

	count = psf_fread (ptr, 1, bytes, psf->filedes) ;

	if (count < bytes)
		memset (((char*)ptr) + count, 0, bytes - count) ;

	psf->read_current += count / psf->blockwidth ;

	psf->last_op = SFM_READ ;

	return count ;
} /* sf_read_raw */

/*------------------------------------------------------------------------------
*/

sf_count_t
sf_read_short	(SNDFILE *sndfile, short *ptr, sf_count_t len)
{	SF_PRIVATE 	*psf ;
	sf_count_t	count, extra ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile, psf) ;
	
	if (psf->mode == SFM_WRITE)
	{	psf->error = SFE_NOT_READMODE ;
		return 0 ;
		} ;
	
	if (len % psf->sf.channels)
	{	psf->error = SFE_BAD_READ_ALIGN ;
		return 0 ;
		} ;
	
	if (len <= 0 || psf->read_current >= psf->sf.samples)
	{	memset (ptr, 0, len * sizeof (short)) ;
		return 0 ; /* End of file. */
		} ;

	if (! psf->read_short || ! psf->new_seek)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return	0 ;
		} ;
		
	if (psf->last_op != SFM_READ)
		if (psf->new_seek (psf, SFM_READ, psf->read_current) < 0)
			return 0 ;
		
	count = psf->read_short (psf, ptr, len) ;
	
	if (psf->read_current + count / psf->sf.channels > psf->sf.samples)
	{	count = (psf->sf.samples - psf->read_current) * psf->sf.channels ;
		extra = len - count ;
		memset (ptr + count, 0, extra * sizeof (short)) ;
		psf->read_current = psf->sf.samples ;
		} ;
	
	psf->read_current += count / psf->sf.channels ;
	
	psf->last_op = SFM_READ ;

	return count ;
} /* sf_read_short */

sf_count_t
sf_readf_short		(SNDFILE *sndfile, short *ptr, sf_count_t frames)
{	SF_PRIVATE 	*psf ;
	sf_count_t		count, extra ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile, psf) ;
	
	if (psf->mode == SFM_WRITE)
	{	psf->error = SFE_NOT_READMODE ;
		return 0 ;
		} ;
	
	if (frames <= 0 || psf->read_current >= psf->sf.samples)
	{	memset (ptr, 0, frames * psf->sf.channels * sizeof (short)) ;
		return 0 ; /* End of file. */
		} ;

	if (! psf->read_short || ! psf->new_seek)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return 0 ;
		} ;
		
	if (psf->last_op != SFM_READ)
		if (psf->new_seek (psf, SFM_READ, psf->read_current) < 0)
			return 0 ;
		
	count = psf->read_short (psf, ptr, frames * psf->sf.channels) ;
	
	if (psf->read_current + count / psf->sf.channels > psf->sf.samples)
	{	count = (psf->sf.samples - psf->read_current) * psf->sf.channels ;
		extra =  frames * psf->sf.channels - count ;
		memset (ptr + count, 0, extra * sizeof (short)) ;
		psf->read_current = psf->sf.samples ;
		} ;
	
	psf->read_current += count / psf->sf.channels ;
	
	psf->last_op = SFM_READ ;

	return count / psf->sf.channels ;
} /* sf_readf_short */

/*------------------------------------------------------------------------------
*/

sf_count_t	
sf_read_int		(SNDFILE *sndfile, int *ptr, sf_count_t len)
{	SF_PRIVATE 	*psf ;
	sf_count_t		count, extra ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF   (sndfile,psf) ;

	if (psf->mode == SFM_WRITE)
	{	psf->error = SFE_NOT_READMODE ;
		return 0 ;
		} ;
	
	if (len % psf->sf.channels)
	{	psf->error = SFE_BAD_READ_ALIGN ;
		return 0 ;
		} ;
	
	if (len <= 0 || psf->read_current >= psf->sf.samples)
	{	memset (ptr, 0, len * sizeof (int)) ;
		return 0 ;
		} ;

	if (! psf->read_int || ! psf->new_seek)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return 0 ;
		} ;
		
	if (psf->last_op != SFM_READ)
		if (psf->new_seek (psf, SFM_READ, psf->read_current) < 0)
			return 0 ;
		
	count = psf->read_int (psf, ptr, len) ;
	
	if (psf->read_current + count / psf->sf.channels > psf->sf.samples)
	{	count = (psf->sf.samples - psf->read_current) * psf->sf.channels ;
		extra = len - count ;
		memset (ptr + count, 0, extra * sizeof (int)) ;
		psf->read_current = psf->sf.samples ;
		} ;
	
	psf->read_current += count / psf->sf.channels ;
	
	psf->last_op = SFM_READ ;

	return count ;
} /* sf_read_int */

sf_count_t	
sf_readf_int	(SNDFILE *sndfile, int *ptr, sf_count_t frames)
{	SF_PRIVATE 	*psf ;
	sf_count_t		count, extra ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF   (sndfile,psf) ;

	if (psf->mode == SFM_WRITE)
	{	psf->error = SFE_NOT_READMODE ;
		return 0 ;
		} ;
	
	if (frames <= 0 || psf->read_current >= psf->sf.samples)
	{	memset (ptr, 0, frames * psf->sf.channels * sizeof (int)) ;
		return 0 ;
		} ;

	if (! psf->read_int || ! psf->new_seek)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return	0 ;
		} ;
		
	if (psf->last_op != SFM_READ)
		if (psf->new_seek (psf, SFM_READ, psf->read_current) < 0)
			return 0 ;
		
	count = psf->read_int (psf, ptr, frames * psf->sf.channels) ;
	
	if (psf->read_current + count / psf->sf.channels > psf->sf.samples)
	{	count = (psf->sf.samples - psf->read_current) * psf->sf.channels ;
		extra = frames * psf->sf.channels - count ;
		memset (ptr + count, 0, extra * sizeof (int)) ;
		psf->read_current = psf->sf.samples ;
		} ;
	
	psf->read_current += count / psf->sf.channels ;
	
	psf->last_op = SFM_READ ;

	return count / psf->sf.channels ;
} /* sf_readf_int */

/*------------------------------------------------------------------------------
*/

sf_count_t	
sf_read_float	(SNDFILE *sndfile, float *ptr, sf_count_t len)
{	SF_PRIVATE 	*psf ;
	sf_count_t		count, extra ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF   (sndfile,psf) ;

	if (psf->mode == SFM_WRITE)
	{	psf->error = SFE_NOT_READMODE ;
		return 0 ;
		} ;
	
	if (len % psf->sf.channels)
	{	psf->error = SFE_BAD_READ_ALIGN ;
		return 0 ;
		} ;
	
	if (len <= 0 || psf->read_current >= psf->sf.samples)
	{	memset (ptr, 0, len * sizeof (float)) ;
		return 0 ;
		} ;

	if (! psf->read_float || ! psf->new_seek)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return	0 ;
		} ;
		
	if (psf->last_op != SFM_READ)
		if (psf->new_seek (psf, SFM_READ, psf->read_current) < 0)
			return 0 ;
		
	count = psf->read_float (psf, ptr, len) ;
	
	if (psf->read_current + count / psf->sf.channels > psf->sf.samples)
	{	count = (psf->sf.samples - psf->read_current) * psf->sf.channels ;
		extra = len - count ;
		memset (ptr + count, 0, extra * sizeof (float)) ;
		psf->read_current = psf->sf.samples ;
		} ;
	
	psf->read_current += count / psf->sf.channels ;
	
	psf->last_op = SFM_READ ;

	return count ;
} /* sf_read_float */

sf_count_t	
sf_readf_float	(SNDFILE *sndfile, float *ptr, sf_count_t frames)
{	SF_PRIVATE 	*psf ;
	sf_count_t		count, extra ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF   (sndfile,psf) ;

	if (psf->mode == SFM_WRITE)
	{	psf->error = SFE_NOT_READMODE ;
		return 0 ;
		} ;
	
	if (frames <= 0 || psf->read_current >= psf->sf.samples)
	{	memset (ptr, 0, frames * psf->sf.channels * sizeof (float)) ;
		return 0 ;
		} ;

	if (! psf->read_float || ! psf->new_seek)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return	0 ;
		} ;
		
	if (psf->last_op != SFM_READ)
		if (psf->new_seek (psf, SFM_READ, psf->read_current) < 0)
			return 0 ;
		
	count = psf->read_float (psf, ptr, frames * psf->sf.channels) ;
	
	if (psf->read_current + count / psf->sf.channels > psf->sf.samples)
	{	count = (psf->sf.samples - psf->read_current) * psf->sf.channels ;
		extra = frames * psf->sf.channels - count ;
		memset (ptr + count, 0, extra * sizeof (float)) ;
		psf->read_current = psf->sf.samples ;
		} ;
	
	psf->read_current += count / psf->sf.channels ;
	
	psf->last_op = SFM_READ ;

	return count / psf->sf.channels ;
} /* sf_readf_float */

/*------------------------------------------------------------------------------
*/

sf_count_t	
sf_read_double	(SNDFILE *sndfile, double *ptr, sf_count_t len)
{	SF_PRIVATE 	*psf ;
	sf_count_t		count, extra ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile, psf) ;
	
	if (psf->mode == SFM_WRITE)
	{	psf->error = SFE_NOT_READMODE ;
		return 0 ;
		} ;
	
	if (len % psf->sf.channels)
	{	psf->error = SFE_BAD_READ_ALIGN ;
		return 0 ;
		} ;
	
	if (len <= 0 || psf->read_current >= psf->sf.samples)
	{	memset (ptr, 0, len * sizeof (double)) ;
		return 0 ;
		} ;
		
	if (! psf->read_double || ! psf->new_seek)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return	0 ;
		} ;
		
	if (psf->last_op != SFM_READ)
		if (psf->new_seek (psf, SFM_READ, psf->read_current) < 0)
			return 0 ;
		
	count = psf->read_double (psf, ptr, len) ;
	
	if (psf->read_current + count / psf->sf.channels > psf->sf.samples)
	{	count = (psf->sf.samples - psf->read_current) * psf->sf.channels ;
		extra = len - count ;
		memset (ptr + count, 0, extra * sizeof (double)) ;
		psf->read_current = psf->sf.samples ;
		} ;
	
	psf->read_current += count / psf->sf.channels ;
	
	psf->last_op = SFM_READ ;

	return count ;
} /* sf_read_double */

sf_count_t	
sf_readf_double	(SNDFILE *sndfile, double *ptr, sf_count_t frames)
{	SF_PRIVATE 	*psf ;
	sf_count_t		count, extra ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile, psf) ;
	
	if (psf->mode == SFM_WRITE)
	{	psf->error = SFE_NOT_READMODE ;
		return 0 ;
		} ;
	
	if (frames <= 0 || psf->read_current >= psf->sf.samples)
	{	memset (ptr, 0, frames * psf->sf.channels * sizeof (double)) ;
		return 0 ;
		} ;
		
	if (! psf->read_double || ! psf->new_seek)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return	0 ;
		} ;
		
	if (psf->last_op != SFM_READ)
		if (psf->new_seek (psf, SFM_READ, psf->read_current) < 0)
			return 0 ;
		
	count = psf->read_double (psf, ptr, frames * psf->sf.channels) ;
	
	if (psf->read_current + count / psf->sf.channels > psf->sf.samples)
	{	count = (psf->sf.samples - psf->read_current) * psf->sf.channels ;
		extra = frames * psf->sf.channels - count ;
		memset (ptr + count, 0, extra * sizeof (double)) ;
		psf->read_current = psf->sf.samples ;
		} ;
	
	psf->read_current += count / psf->sf.channels ;
	
	psf->last_op = SFM_READ ;

	return count / psf->sf.channels ;
} /* sf_readf_double */

/*------------------------------------------------------------------------------
*/

sf_count_t	
sf_write_raw	(SNDFILE *sndfile, void *ptr, sf_count_t len)
{	SF_PRIVATE 	*psf ;
	sf_count_t	count ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile, psf) ;

	if (psf->mode == SFM_READ)
	{	psf->error = SFE_NOT_WRITEMODE ;
		return 0 ;
		} ;
	
	if (len % (psf->sf.channels * psf->bytewidth))
	{	psf->error = SFE_BAD_WRITE_ALIGN ;
		return 0 ;
		} ;
	
	count = psf_fwrite (ptr, 1, len, psf->filedes) ;
	
	psf->write_current += count / psf->blockwidth ;
	
	if (psf->write_current > psf->sf.samples)
		psf->sf.samples = psf->write_current ;	
	
	psf->last_op = SFM_WRITE ;

	return count ;
} /* sf_write_raw */

/*------------------------------------------------------------------------------
*/

sf_count_t	
sf_write_short	(SNDFILE *sndfile, short *ptr, sf_count_t len)
{	SF_PRIVATE 	*psf ;
	sf_count_t	count ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile, psf) ;

	if (psf->mode == SFM_READ)
	{	psf->error = SFE_NOT_WRITEMODE ;
		return 0 ;
		} ;
	
	if (len % psf->sf.channels)
	{	psf->error = SFE_BAD_WRITE_ALIGN ;
		return 0 ;
		} ;
	
	if (! psf->write_short || ! psf->new_seek)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return 0 ;
		} ;
		
	if (psf->last_op != SFM_WRITE)
		if (psf->new_seek (psf, SFM_WRITE, psf->write_current) < 0)
			return 0 ;
		
	count = psf->write_short (sndfile, ptr, len) ;
	
	psf->write_current += count / psf->sf.channels ;
	
	if (psf->write_current > psf->sf.samples)
		psf->sf.samples = psf->write_current ;	

	psf->last_op = SFM_WRITE ;

	return count ;
} /* sf_write_short */

sf_count_t	
sf_writef_short	(SNDFILE *sndfile, short *ptr, sf_count_t frames)
{	SF_PRIVATE 	*psf ;
	sf_count_t	count ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile, psf) ;

	if (psf->mode == SFM_READ)
	{	psf->error = SFE_NOT_WRITEMODE ;
		return 0 ;
		} ;
	
	if (! psf->write_short || ! psf->new_seek)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return 0 ;
		} ;
		
	if (psf->last_op != SFM_WRITE)
		if (psf->new_seek (psf, SFM_WRITE, psf->write_current) < 0)
			return 0 ;
		
	count = psf->write_short (sndfile, ptr, frames * psf->sf.channels) ;
	
	psf->write_current += count / psf->sf.channels ;
	
	if (psf->write_current > psf->sf.samples)
		psf->sf.samples = psf->write_current ;	

	psf->last_op = SFM_WRITE ;

	return count / psf->sf.channels ;
} /* sf_writef_short */

/*------------------------------------------------------------------------------
*/

sf_count_t	
sf_write_int	(SNDFILE *sndfile, int *ptr, sf_count_t len)
{	SF_PRIVATE 	*psf ;
	sf_count_t	count ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile, psf) ;

	if (psf->mode == SFM_READ)
	{	psf->error = SFE_NOT_WRITEMODE ;
		return 0 ;
		} ;
	
	if (len % psf->sf.channels)
	{	psf->error = SFE_BAD_WRITE_ALIGN ;
		return 0 ;
		} ;
	
	if (! psf->write_int || ! psf->new_seek)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return 0 ;
		} ;
		
	if (psf->last_op != SFM_WRITE)
		if (psf->new_seek (psf, SFM_WRITE, psf->write_current) < 0)
			return 0 ;
		
	count = psf->write_int (sndfile, ptr, len) ;

	psf->write_current += count / psf->sf.channels ;
	
	if (psf->write_current > psf->sf.samples)
		psf->sf.samples = psf->write_current ;	

	psf->last_op = SFM_WRITE ;

	return count ;
} /* sf_write_int */

sf_count_t	
sf_writef_int	(SNDFILE *sndfile, int *ptr, sf_count_t frames)
{	SF_PRIVATE 	*psf ;
	sf_count_t	count ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile, psf) ;

	if (psf->mode == SFM_READ)
	{	psf->error = SFE_NOT_WRITEMODE ;
		return 0 ;
		} ;
	
	if (! psf->write_int || ! psf->new_seek)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return 0 ;
		} ;
		
	if (psf->last_op != SFM_WRITE)
		if (psf->new_seek (psf, SFM_WRITE, psf->write_current) < 0)
			return 0 ;
		
	count = psf->write_int (sndfile, ptr, frames * psf->sf.channels) ;

	psf->write_current += count / psf->sf.channels ;
	
	if (psf->write_current > psf->sf.samples)
		psf->sf.samples = psf->write_current ;	

	psf->last_op = SFM_WRITE ;

	return count / psf->sf.channels ;
} /* sf_writef_int */

/*------------------------------------------------------------------------------
*/

sf_count_t	
sf_write_float	(SNDFILE *sndfile, float *ptr, sf_count_t len)
{	SF_PRIVATE 	*psf ;
	sf_count_t	count ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile, psf) ;

	if (psf->mode == SFM_READ)
	{	psf->error = SFE_NOT_WRITEMODE ;
		return 0 ;
		} ;
	
	if (len % psf->sf.channels)
	{	psf->error = SFE_BAD_WRITE_ALIGN ;
		return 0 ;
		} ;
	
	if (! psf->write_float || ! psf->new_seek)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return 0 ;
		} ;
		
	if (psf->last_op != SFM_WRITE)
		if (psf->new_seek (psf, SFM_WRITE, psf->write_current) < 0)
			return 0 ;

		
	count = psf->write_float (sndfile, ptr, len) ;

	psf->write_current += count / psf->sf.channels ;
	
	if (psf->write_current > psf->sf.samples)
		psf->sf.samples = psf->write_current ;	

	psf->last_op = SFM_WRITE ;

	return count ;
} /* sf_write_float */

sf_count_t	
sf_writef_float	(SNDFILE *sndfile, float *ptr, sf_count_t frames)
{	SF_PRIVATE 	*psf ;
	sf_count_t	count ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile, psf) ;

	if (psf->mode == SFM_READ)
	{	psf->error = SFE_NOT_WRITEMODE ;
		return 0 ;
		} ;
	
	if (! psf->write_float || ! psf->new_seek)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return 0 ;
		} ;
		
	if (psf->last_op != SFM_WRITE)
		if (psf->new_seek (psf, SFM_WRITE, psf->write_current) < 0)
			return 0 ;
		
	count = psf->write_float (sndfile, ptr, frames * psf->sf.channels) ;

	psf->write_current += count / psf->sf.channels ;
	
	if (psf->write_current > psf->sf.samples)
		psf->sf.samples = psf->write_current ;	

	psf->last_op = SFM_WRITE ;

	return count / psf->sf.channels ;
} /* sf_writef_float */

/*------------------------------------------------------------------------------
*/

sf_count_t	
sf_write_double	(SNDFILE *sndfile, double *ptr, sf_count_t len)
{	SF_PRIVATE 	*psf ;
	sf_count_t	count ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile, psf) ;

	if (psf->mode == SFM_READ)
	{	psf->error = SFE_NOT_WRITEMODE ;
		return 0 ;
		} ;
	
	if (len % psf->sf.channels)
	{	psf->error = SFE_BAD_WRITE_ALIGN ;
		return	0 ;
		} ;
		
	if (! psf->write_double || ! psf->new_seek)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return 0 ;
		} ;
		
	if (psf->last_op != SFM_WRITE)
		if (psf->new_seek (psf, SFM_WRITE, psf->write_current) < 0)
			return 0 ;
		
	count = psf->write_double (sndfile, ptr, len) ;
	
	psf->write_current += count / psf->sf.channels ;
	
	if (psf->write_current > psf->sf.samples)
		psf->sf.samples = psf->write_current ;	

	psf->last_op = SFM_WRITE ;

	return count ;
} /* sf_write_double */

sf_count_t	
sf_writef_double	(SNDFILE *sndfile, double *ptr, sf_count_t frames)
{	SF_PRIVATE 	*psf ;
	sf_count_t	count ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile, psf) ;

	if (psf->mode == SFM_READ)
	{	psf->error = SFE_NOT_WRITEMODE ;
		return 0 ;
		} ;
	
	if (! psf->write_double || ! psf->new_seek)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return 0 ;
		} ;
		
	if (psf->last_op != SFM_WRITE)
		if (psf->new_seek (psf, SFM_WRITE, psf->write_current) < 0)
			return 0 ;
		
	count = psf->write_double (sndfile, ptr, frames * psf->sf.channels) ;
	
	psf->write_current += count / psf->sf.channels ;
	
	if (psf->write_current > psf->sf.samples)
		psf->sf.samples = psf->write_current ;	

	psf->last_op = SFM_WRITE ;

	return count / psf->sf.channels ;
} /* sf_writef_double */

/*------------------------------------------------------------------------------
*/

int	
sf_close	(SNDFILE *sndfile)
{	SF_PRIVATE  *psf ;
	int			error ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile, psf) ;

	if (psf->close)
		error = psf->close (psf) ;
	
	psf_fclose (psf->filedes) ;
	
	if (psf->fdata)
		free (psf->fdata) ;
	memset (psf, 0, sizeof (SF_PRIVATE)) ;
		
	free (psf) ;

	return 0 ;
} /* sf_close */

/*=========================================================================
** Private functions.
*/

static int 
does_extension_match (const char *ext, const char *test)
{	char c1, c2 ;

	if ((! ext) || (! test))
		return 0 ;

	if (strlen (ext) != strlen (test))
		return 0 ;

	while (*ext && *test)
	{	c1 = tolower (*ext) ;
		c2 = tolower (*test) ;
		if (c1 > c2)
			return 0 ;
		if (c1 < c2)
			return 0 ;
		ext ++ ;
		test ++ ;
		} 

	return 1 ;
} /* does_extension_match */

static int 
is_au_snd_file (const char *filename)
{	const char *cptr ;

	if (! (cptr = strrchr (filename, '.')))
		return 0 ;
	cptr ++ ;
	
	if (does_extension_match (cptr, "au"))
		return 1 ;
		
	if (does_extension_match (cptr, "snd"))
		return 1 ;
		
	return 0 ;
} /* is_au_snd_file */

static int 
guess_file_type (SF_PRIVATE *psf, const char *filename)
{	int 	buffer [3] ;
	unsigned char	cptr [0x40] ;
			
	if (psf_binheader_readf (psf, "b", &buffer, sizeof (buffer)) != sizeof (buffer))
	{	psf->error = SFE_BAD_FILE_READ ;
		return 0 ;
		} ;

	if (buffer [0] == MAKE_MARKER ('R','I','F','F') && buffer [2] == MAKE_MARKER ('W','A','V','E'))
		return SF_FORMAT_WAV ;
		
	if (buffer [0] == MAKE_MARKER ('F','O','R','M'))
	{	if (buffer [2] == MAKE_MARKER ('A','I','F','F') || buffer [2] == MAKE_MARKER ('A','I','F','C'))
			return SF_FORMAT_AIFF ;
		if (buffer [2] == MAKE_MARKER ('8','S','V','X') || buffer [2] == MAKE_MARKER ('1','6','S','V'))
			return SF_FORMAT_SVX ;
		return 0 ;
		} ;
		
	if ((buffer [0] == MAKE_MARKER ('.','s','n','d') || buffer [0] == MAKE_MARKER ('d','n','s','.')))
		return SF_FORMAT_AU ;
		
	if ((buffer [0] == MAKE_MARKER ('f','a','p',' ') || buffer [0] == MAKE_MARKER (' ','p','a','f')))
		return SF_FORMAT_PAF ;
	
	if (buffer [0] == MAKE_MARKER ('N','I','S','T'))
		return SF_FORMAT_NIST ;
		
	if (buffer [0] == MAKE_MARKER ('C','r','e','a') && buffer [1] == MAKE_MARKER ('t','i','v','e'))
		return SF_FORMAT_VOC ;
		
	if ((buffer [0] & MAKE_MARKER (0xFF, 0xFF, 0xF8, 0xFF)) == MAKE_MARKER (0x64, 0xA3, 0x00, 0x00) || 
		(buffer [0] & MAKE_MARKER (0xFF, 0xF8, 0xFF, 0xFF)) == MAKE_MARKER (0x00, 0x00, 0xA3, 0x64))
		return SF_FORMAT_IRCAM ;
		
	if ((buffer [0] == MAKE_MARKER ('r', 'i', 'f', 'f')))
		return SF_FORMAT_W64 ;
		
	if (buffer [0] == MAKE_MARKER ('S','E','K','D') && buffer [1] == MAKE_MARKER ('S','A','M','R'))
		return SF_FORMAT_SMPLTD ;
		
	if (buffer [0] == MAKE_MARKER ('C', 'A', 'T', ' ') && buffer [2] == MAKE_MARKER ('R', 'E', 'X', '2'))
		return SF_FORMAT_REX2 ;

	if (buffer [0] == MAKE_MARKER (0x30, 0x26, 0xB2, 0x75) && buffer [1] == MAKE_MARKER (0x8E, 0x66, 0xCF, 0x11))
		return SF_FORMAT_WMA ;

	/* Turtle Beach SMP 16-bit */
	if (buffer [0] == MAKE_MARKER ('S', 'O', 'U', 'N') && buffer [1] == MAKE_MARKER ('D', ' ', 'S', 'A'))
		return 0 ;

	if (buffer [0] == MAKE_MARKER ('S', 'Y', '8', '0') || buffer [0] == MAKE_MARKER ('S', 'Y', '8', '5'))
		return 0 ;

	if (buffer [0] == MAKE_MARKER ('L', 'M', '8', '9'))
		return SF_FORMAT_TXW ;

	if (buffer [0] == MAKE_MARKER ('a', 'j', 'k', 'g'))
		return SF_FORMAT_SHN ;

	/*	Detect wacky MacOS header stuff. This might be "Sound Designer II". */
	memcpy (cptr , buffer, sizeof (buffer)) ;
	if (cptr [0] == 0 && cptr [1] > 0 && psf->sf.seekable)
	{	psf_binheader_readf (psf, "pb", 0, &cptr, sizeof (cptr)) ;
		
		if (cptr [1] < (sizeof (cptr) - 3) && cptr [cptr [1] + 2] == 0 && strlen (((char*) cptr) + 2) == cptr [1])
		{	psf_log_printf (psf, "Weird MacOS Header.\n") ;
			psf_binheader_readf (psf, "em", &buffer) ;
			if (buffer [0] == MAKE_MARKER (0, 'S', 'd', '2'))
				return SF_FORMAT_SD2 ;
			} ;
		} ;

	/* This must be the last one. */		
	if (filename && is_au_snd_file (filename))
		return SF_FORMAT_AU | SF_FORMAT_ULAW ;

	/* Default to header-less RAW PCM file type. */
	return SF_FORMAT_RAW ;
} /* guess_file_type */


static int 
validate_sfinfo (SF_INFO *sfinfo)
{	if (sfinfo->samplerate < 0)
		return 0 ;	
	if (sfinfo->samples < 0)
		return 0 ;	
	if (sfinfo->channels < 0)
		return 0 ;	
	if (! sfinfo->format & SF_FORMAT_TYPEMASK)
		return 0 ;	
	if (! sfinfo->format & SF_FORMAT_SUBMASK)
		return 0 ;	
	if (! sfinfo->sections)
		return 0 ;	
	return 1 ;
} /* validate_sfinfo */

static int
validate_psf (SF_PRIVATE *psf)
{	
	if (psf->datalength < 0)
	{	psf_log_printf (psf, "Invalid SF_PRIVATE field : datalength == %d.\n", psf->datalength) ;
		return 0 ;
		} ;
	if (psf->dataoffset < 0)
	{	psf_log_printf (psf, "Invalid SF_PRIVATE field : datalength == %d.\n", psf->datalength) ;
		return 0 ;
		} ;
	if (psf->blockwidth && psf->blockwidth != psf->sf.channels * psf->bytewidth)
	{	psf_log_printf (psf, "Invalid SF_PRIVATE field : channels * bytewidth == %d.\n", 
								psf->sf.channels * psf->bytewidth) ;
		return 0 ;	
		} ;
	return 1 ;
} /* validate_psf */

static void 
save_header_info (SF_PRIVATE *psf)
{	memset (sf_logbuffer, 0, sizeof (sf_logbuffer)) ;
	strncpy (sf_logbuffer, psf->logbuffer, sizeof (sf_logbuffer)) ;
} /* save_header_info */

static void 
copy_filename (SF_PRIVATE *psf, const char *path)
{	const char *cptr ;

	if ((cptr = strrchr (path, '/')) || (cptr = strrchr (path, '\\')))
		cptr ++ ;
	else
		cptr = path ;
		
	memset (psf->filename, 0, SF_FILENAME_LEN) ;
	strncpy (psf->filename, cptr, SF_FILENAME_LEN - 1) ;
	psf->filename [SF_FILENAME_LEN - 1] = 0 ;
} /* copy_filename */

/*-static int	
hash_command (const char *cmd)
{	int	hash = 0 ;

	if (! cmd)
		return 0 ;

	while (cmd [0])
	{	hash = (hash << 1) ^ cmd [0] ;
		cmd ++ ;
		} ;
		
	return hash ;
} /+* hash_command *+/
-*/
