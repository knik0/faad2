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


#include	<stdio.h>
#include	<unistd.h>
#include	<string.h>
#include	<ctype.h>
#include	<time.h>

#include	"sndfile.h"
#include	"config.h"
#include	"sfendian.h"
#include	"common.h"


/*------------------------------------------------------------------------------
 * Macros to handle big/little endian issues.
 */

#if (CPU_IS_LITTLE_ENDIAN == 1)
	#define	MAKE_MARKER(a,b,c,d)		((a)|((b)<<8)|((c)<<16)|((d)<<24))
#elif (CPU_IS_BIG_ENDIAN == 1)
	#define	MAKE_MARKER(a,b,c,d)		(((a)<<24)|((b)<<16)|((c)<<8)|(d))
#else
	#error "Cannot determine endian-ness of processor."
#endif

#define FORM_MARKER	(MAKE_MARKER ('F', 'O', 'R', 'M')) 
#define AIFF_MARKER	(MAKE_MARKER ('A', 'I', 'F', 'F')) 
#define AIFC_MARKER	(MAKE_MARKER ('A', 'I', 'F', 'C')) 
#define COMM_MARKER	(MAKE_MARKER ('C', 'O', 'M', 'M')) 
#define SSND_MARKER	(MAKE_MARKER ('S', 'S', 'N', 'D')) 
#define MARK_MARKER	(MAKE_MARKER ('M', 'A', 'R', 'K')) 
#define INST_MARKER	(MAKE_MARKER ('I', 'N', 'S', 'T')) 
#define APPL_MARKER	(MAKE_MARKER ('A', 'P', 'P', 'L')) 

#define c_MARKER	(MAKE_MARKER ('(', 'c', ')', ' ')) 
#define NAME_MARKER	(MAKE_MARKER ('N', 'A', 'M', 'E')) 
#define AUTH_MARKER	(MAKE_MARKER ('A', 'U', 'T', 'H')) 
#define ANNO_MARKER	(MAKE_MARKER ('A', 'N', 'N', 'O')) 
#define COMT_MARKER	(MAKE_MARKER ('C', 'O', 'M', 'T')) 
#define FVER_MARKER	(MAKE_MARKER ('F', 'V', 'E', 'R')) 
#define SFX_MARKER	(MAKE_MARKER ('S', 'F', 'X', '!')) 


#define PEAK_MARKER	(MAKE_MARKER ('P', 'E', 'A', 'K')) 

/* Supported AIFC encodings.*/
#define NONE_MARKER	(MAKE_MARKER ('N', 'O', 'N', 'E')) 
#define sowt_MARKER	(MAKE_MARKER ('s', 'o', 'w', 't')) 
#define twos_MARKER	(MAKE_MARKER ('t', 'w', 'o', 's')) 
#define fl32_MARKER	(MAKE_MARKER ('f', 'l', '3', '2')) 
#define FL32_MARKER	(MAKE_MARKER ('F', 'L', '3', '2')) 

/* Unsupported AIFC encodings.*/
#define fl64_MARKER	(MAKE_MARKER ('f', 'l', '6', '4')) 
#define FL64_MARKER	(MAKE_MARKER ('F', 'L', '6', '4')) 
#define MAC3_MARKER	(MAKE_MARKER ('M', 'A', 'C', '3')) 
#define MAC6_MARKER	(MAKE_MARKER ('M', 'A', 'C', '6')) 
#define ima4_MARKER	(MAKE_MARKER ('i', 'm', 'a', '4')) 
#define ulaw_MARKER	(MAKE_MARKER ('a', 'l', 'a', 'w')) 
#define alaw_MARKER	(MAKE_MARKER ('u', 'l', 'a', 'w')) 
#define ADP4_MARKER	(MAKE_MARKER ('A', 'D', 'P', '4')) 

/* Predfined chunk sizes. */
#define AIFF_COMM_SIZE		18
#define AIFC_COMM_SIZE		24
#define SSND_CHUNK_SIZE		8

/*------------------------------------------------------------------------------
 * Typedefs for file chunks.
 */

enum
{	HAVE_FORM	= 0x01,
	HAVE_AIFF	= 0x02,
	HAVE_COMM	= 0x04,
	HAVE_SSND	= 0x08
} ;

typedef struct
{	unsigned int	type ;
	unsigned int	size ;
	short			numChannels ;
	unsigned int	numSampleFrames ;
	short			sampleSize ;
	unsigned char	sampleRate [10] ;
	unsigned int	encoding ;
	char			zero_bytes [2] ;
} COMM_CHUNK ; 

typedef struct
{	unsigned int	offset ;
	unsigned int	blocksize ;
} SSND_CHUNK ; 

typedef struct 
{	short           playMode;
    int		        beginLoop;
	int		        endLoop;
} INST_CHUNK ;

/*------------------------------------------------------------------------------
 * Private static functions.
 */

static	int		aiff_close	(SF_PRIVATE  *psf) ;

static	int     tenbytefloat2int (unsigned char *bytes) ;
static	void	uint2tenbytefloat (unsigned int num, unsigned char *bytes) ;

static  int  	aiff_read_comm_chunk (SF_PRIVATE *psf, COMM_CHUNK *comm_fmt) ;

static int		aiff_write_header (SF_PRIVATE *psf) ;
static int		aiff_write_tailer (SF_PRIVATE *psf) ;

/*------------------------------------------------------------------------------
** Public functions.
*/

int
aiff_open_read	(SF_PRIVATE *psf)
{	COMM_CHUNK	comm_fmt ;
	SSND_CHUNK	ssnd_fmt ;
	int			marker, dword ;
	long		FORMsize, SSNDsize ;
	int			filetype, found_chunk = 0, done = 0, error = 0 ;
	char		*cptr ;

	psf->sf.seekable = SF_TRUE ;

	/* Set position to start of file to begin reading header. */
	psf_binheader_readf (psf, "p", 0) ;	

	memset (&comm_fmt, 0, sizeof (comm_fmt)) ;

	/* Until recently AIF* file were all BIG endian. */ 	
	psf->endian = SF_ENDIAN_BIG ;

	/* 	AIFF files can apparently have their chunks in any order. However, they must
	**	a FORM chunk. Approach here is to read allthe chunks one by one and then 
	**	check for the mandatory chunks at the end.
	*/
	while (! done)
	{	psf_binheader_readf (psf, "m", &marker) ;
		switch (marker)
		{	case FORM_MARKER :
					if (found_chunk)
						return SFE_AIFF_NO_FORM ;

					psf_binheader_readf (psf, "L", &FORMsize) ;

					if (FORMsize != psf->filelength - 2 * sizeof (dword))
					{	dword = psf->filelength - 2 * sizeof (dword);
						psf_log_printf (psf, "FORM : %d (should be %d)\n", FORMsize, dword) ;
						FORMsize = dword ;
						}
					else
						psf_log_printf (psf, "FORM : %d\n", FORMsize) ;
					found_chunk |= HAVE_FORM ;
					break ;
					
			case AIFC_MARKER :
			case AIFF_MARKER :
					if (! (found_chunk & HAVE_FORM))
						return SFE_AIFF_AIFF_NO_FORM ;
					filetype = marker ;
					psf_log_printf (psf, " %D\n", marker) ;
					found_chunk |= HAVE_AIFF ;
					break ;

			case COMM_MARKER :
					error = aiff_read_comm_chunk (psf, &comm_fmt) ;

					psf->sf.samplerate 		= tenbytefloat2int (comm_fmt.sampleRate) ;
					psf->sf.samples 		= comm_fmt.numSampleFrames ;
					psf->sf.channels 		= comm_fmt.numChannels ;
					psf->sf.pcmbitwidth 	= comm_fmt.sampleSize ;
					psf->sf.sections 		= 1 ;
					
					if (error)
						return error ;
						
					found_chunk |= HAVE_COMM ;
					break ;

			case PEAK_MARKER :
					/* Must have COMM chunk before PEAK chunk. */
					if ((found_chunk & (HAVE_FORM | HAVE_AIFF | HAVE_COMM)) != (HAVE_FORM | HAVE_AIFF | HAVE_COMM))
						return SFE_AIFF_PEAK_B4_COMM ;

					psf_binheader_readf (psf, "L", &dword) ;
					
					psf_log_printf (psf, "%D : %d\n", marker, dword) ;
					if (dword > sizeof (psf->peak))
					{	psf_binheader_readf (psf, "j", dword) ;
						psf_log_printf (psf, "*** File PEAK chunk bigger than sizeof (PEAK_CHUNK).\n") ;
						return SFE_WAV_BAD_PEAK ;
						} ;
					if (dword != sizeof (psf->peak) - sizeof (psf->peak.peak) + psf->sf.channels * sizeof (PEAK_POS))
					{	psf_binheader_readf (psf, "j", dword) ;
						psf_log_printf (psf, "*** File PEAK chunk size doesn't fit with number of channels.\n") ;
						return SFE_WAV_BAD_PEAK ;
						} ;
					
					/* read in rest of PEAK chunk. */	
					psf_binheader_readf (psf, "LL", &(psf->peak.version), &(psf->peak.timestamp)) ;

					if (psf->peak.version != 1)
						psf_log_printf (psf, "  version    : %d *** (should be version 1)\n", psf->peak.version) ;
					else
						psf_log_printf (psf, "  version    : %d\n", psf->peak.version) ;
						
					psf_log_printf (psf, "  time stamp : %d\n", psf->peak.timestamp) ;
					psf_log_printf (psf, "    Ch   Position       Value\n") ;

					cptr = (char *) psf->buffer ;
					for (dword = 0 ; dword < psf->sf.channels ; dword++)
					{	psf_binheader_readf (psf, "FL", &(psf->peak.peak[dword].value), 
														&(psf->peak.peak[dword].position)) ;
					
						snprintf (cptr, sizeof (psf->buffer), "    %2d   %-12d   %g\n", 
								dword, psf->peak.peak[dword].position, psf->peak.peak[dword].value) ;
						cptr [sizeof (psf->buffer) - 1] = 0 ;
						psf_log_printf (psf, cptr) ;
						};

					psf->has_peak = SF_TRUE ;
					break ;

			case SSND_MARKER :
					psf_binheader_readf (psf, "LLL", &SSNDsize, &(ssnd_fmt.offset), &(ssnd_fmt.blocksize)) ;
											
					psf->dataoffset = ftell (psf->file) ;
					psf->datalength = psf->filelength - psf->dataoffset ;
					if (SSNDsize > psf->datalength + sizeof (SSND_CHUNK))
						psf_log_printf (psf, " SSND : %d (should be %d)\n", SSNDsize, psf->datalength + sizeof (SSND_CHUNK)) ;
					else
						psf_log_printf (psf, " SSND : %d\n", SSNDsize) ;
					
					psf_log_printf (psf, "  Offset     : %d\n", ssnd_fmt.offset) ;
					psf_log_printf (psf, "  Block Size : %d\n", ssnd_fmt.blocksize) ;
					
					found_chunk |= HAVE_SSND ;

					if (! psf->sf.seekable)
						break ;

					/* 	Seek to end of SSND chunk. */
					fseek (psf->file, SSNDsize - 8, SEEK_CUR) ;
					break ;

			case c_MARKER :
			case ANNO_MARKER :
			case AUTH_MARKER :
			case COMT_MARKER :
			case NAME_MARKER :
					psf_binheader_readf (psf, "L", &dword);
					dword += (dword & 1) ;
					if (dword == 0)
						break ;
					if (dword > sizeof (psf->buffer))
					{	psf_log_printf (psf, "  *** %D : %d (too big)\n", marker, dword) ;
						return SFE_INTERNAL ;
						} ;
					psf_binheader_readf (psf, "B", psf->buffer, dword) ;
					psf->buffer [dword - 1] = 0 ;
					psf_log_printf (psf, "***    %D : %s\n", marker, psf->buffer) ;
					break ;

			case FVER_MARKER :
			case SFX_MARKER :
					psf_binheader_readf (psf, "L", &dword) ;
					psf_log_printf (psf, " %D : %d\n", marker, dword) ;

					psf_binheader_readf (psf, "j", dword) ;
					break ;

			case APPL_MARKER :
			case INST_MARKER :
			case MARK_MARKER :
					psf_binheader_readf (psf, "L", &dword) ;
					psf_log_printf (psf, " %D : %d\n", marker, dword) ;

					psf_binheader_readf (psf, "j", dword) ;
					break ;

			default : 
					if (isprint ((marker >> 24) & 0xFF) && isprint ((marker >> 16) & 0xFF)
						&& isprint ((marker >> 8) & 0xFF) && isprint (marker & 0xFF))
					{	psf_binheader_readf (psf, "L", &dword) ;
						psf_log_printf (psf, "%D : %d (unknown marker)\n", marker, dword) ;

						psf_binheader_readf (psf, "j", dword) ;
						break ;
						} ;
					if ((dword = ftell (psf->file)) & 0x03)
					{	psf_log_printf (psf, "  Unknown chunk marker at position %d. Resyncing.\n", dword - 4) ;

						psf_binheader_readf (psf, "j", -3) ;
						break ;
						} ;
					psf_log_printf (psf, "*** Unknown chunk marker : %X. Exiting parser.\n", marker) ;
					done = 1 ;
					break ;
			} ;	/* switch (marker) */
			
		if ((! psf->sf.seekable) && (found_chunk & HAVE_SSND))
			break ;

		if (ferror (psf->file))
		{	psf_log_printf (psf, "*** Error on file handle. ***\n") ;
			clearerr (psf->file) ;
			break ;
			} ;

		if (ftell (psf->file) >= (long) (psf->filelength - (2 * sizeof (dword))))
			break ;
		} ; /* while (1) */
		
	if (! (found_chunk & HAVE_FORM))
		return SFE_AIFF_NO_FORM ;

	if (! (found_chunk & HAVE_AIFF))
		return SFE_AIFF_COMM_NO_FORM ;

	if (! (found_chunk & HAVE_COMM))
		return SFE_AIFF_SSND_NO_COMM ;

	if (! psf->dataoffset)
		return SFE_AIFF_NO_DATA ;

	psf->current     = 0 ;
	psf->sf.seekable = SF_TRUE ;
	psf->bytewidth   = BITWIDTH2BYTES (psf->sf.pcmbitwidth) ;

	psf->blockwidth  = psf->sf.channels * psf->bytewidth ;
	
	fseek (psf->file, psf->dataoffset, SEEK_SET) ;

	psf->close = (func_close) aiff_close ;

	switch (psf->sf.format & SF_FORMAT_SUBMASK)
	{	case SF_FORMAT_PCM :
		case SF_FORMAT_PCM_LE :
		case SF_FORMAT_PCM_BE :
				if (psf->bytewidth == 1)
					psf->chars = SF_CHARS_SIGNED ;

				error = pcm_read_init (psf) ;
				break ;
				
		case SF_FORMAT_FLOAT :
				error = float32_read_init (psf) ;
				break ;

		default : return SFE_UNIMPLEMENTED ;
		} ;

	return error ;
} /* aiff_open_read */

/*------------------------------------------------------------------------------
 */

int
aiff_open_write	(SF_PRIVATE *psf)
{	int error ;

	if ((psf->sf.format & SF_FORMAT_TYPEMASK) != SF_FORMAT_AIFF)
		return	SFE_BAD_OPEN_FORMAT ;
	
	psf->sf.seekable = SF_TRUE ;
	psf->error       = 0 ;

	psf->bytewidth   = BITWIDTH2BYTES (psf->sf.pcmbitwidth) ;
	psf->blockwidth  = psf->bytewidth * psf->sf.channels ;

	/* Set sample count artifically high and fix later. */ 
	psf->sf.samples = 0x7FFFFFFF / psf->blockwidth ; 
	
	psf->datalength  = psf->blockwidth * psf->sf.samples ;
	psf->filelength  = 0x7FFFFFFF ;

	if ((error = aiff_write_header (psf)))
		return error ;

	psf->close        = (func_close)  aiff_close ;
	psf->write_header = (func_wr_hdr) aiff_write_header ;

	switch (psf->sf.format & SF_FORMAT_SUBMASK)
	{	case SF_FORMAT_PCM :
		case SF_FORMAT_PCM_LE :
		case SF_FORMAT_PCM_BE :
				if (psf->bytewidth == 1)
					psf->chars = SF_CHARS_SIGNED ;

				error = pcm_write_init (psf) ;
				break ;
				
		case SF_FORMAT_FLOAT :
				error = float32_write_init (psf) ;
				break ;

		default : return SFE_UNIMPLEMENTED ;
		} ;

	return error ;
} /* aiff_open_write */

/*==========================================================================================
** Private functions.
*/

static int	
aiff_close	(SF_PRIVATE  *psf)
{
	if (psf->mode == SF_MODE_WRITE)
	{	/*  Now we know for certain the length of the file we can re-write 
		**	correct values for the FORM, COMM and SSND chunks.
		*/
		 
		aiff_write_tailer (psf) ;

		fseek (psf->file, 0, SEEK_END) ;		
		psf->filelength = ftell (psf->file) ;
		fseek (psf->file, 0, SEEK_SET) ;
		
		psf->datalength = psf->filelength - psf->dataoffset ;
 		psf->sf.samples = psf->datalength / (psf->bytewidth * psf->sf.channels) ;
		
		aiff_write_header (psf) ;
		} ;

	if (psf->fdata)
		free (psf->fdata) ;
	psf->fdata = NULL ;
	
	return 0 ;
} /* aiff_close */

static int
aiff_read_comm_chunk (SF_PRIVATE *psf, COMM_CHUNK *comm_fmt)
{	int error = 0, bytesread ;

	bytesread = psf_binheader_readf (psf, "L", &(comm_fmt->size)) ;
	
	/* The COMM chunk has an int aligned to an odd word boundary. Some 
	** procesors are not able to deal with this (ie bus fault) so we have 
	** to take special care.
	*/

	bytesread +=
	psf_binheader_readf (psf, "WLWB", &(comm_fmt->numChannels), &(comm_fmt->numSampleFrames), 
			&(comm_fmt->sampleSize), &(comm_fmt->sampleRate), sizeof (comm_fmt->sampleRate)) ;

	if (comm_fmt->size == AIFF_COMM_SIZE)
		comm_fmt->encoding = NONE_MARKER ;
	else if (comm_fmt->size >= AIFC_COMM_SIZE)
	{	bytesread +=
		psf_binheader_readf (psf, "mB", &(comm_fmt->encoding), &(comm_fmt->zero_bytes), 2) ;

		psf_binheader_readf (psf, "B", psf->header, comm_fmt->size - AIFC_COMM_SIZE) ;
		} ;
		
	psf_log_printf (psf, " COMM : %d\n", comm_fmt->size) ;
	psf_log_printf (psf, "  Sample Rate : %d\n", tenbytefloat2int (comm_fmt->sampleRate)) ;
	psf_log_printf (psf, "  Samples     : %d\n", comm_fmt->numSampleFrames) ;
	psf_log_printf (psf, "  Channels    : %d\n", comm_fmt->numChannels) ;

	/*	Found some broken 'fl32' files with comm.samplesize == 16. Fix it here. */ 

	if ((comm_fmt->encoding == fl32_MARKER || comm_fmt->encoding == FL32_MARKER) && comm_fmt->sampleSize != 32)
	{	psf_log_printf (psf, "  Sample Size : %d (should be 32)\n", comm_fmt->sampleSize) ;
		comm_fmt->sampleSize = 32 ;
		}
	else
		psf_log_printf (psf, "  Sample Size : %d\n", comm_fmt->sampleSize) ;

	switch (comm_fmt->encoding)
	{	case NONE_MARKER :
				psf->endian = SF_ENDIAN_BIG ;
				psf->sf.format = (SF_FORMAT_AIFF | SF_FORMAT_PCM) ;
				break ;
				
		case twos_MARKER :
				psf->endian = SF_ENDIAN_BIG ;
				psf->sf.format = (SF_FORMAT_AIFF | SF_FORMAT_PCM_BE) ;
				break ;

		case sowt_MARKER :
				psf->endian = SF_ENDIAN_LITTLE ;
				psf->sf.format = (SF_FORMAT_AIFF | SF_FORMAT_PCM_LE) ;
				break ;
				
		case fl32_MARKER :
		case FL32_MARKER :
				psf->endian = SF_ENDIAN_BIG ;
				psf->sf.format = (SF_FORMAT_AIFF | SF_FORMAT_FLOAT) ;
				break ;

		default :
			psf_log_printf (psf, "AIFC : Unimplemented format : %D\n", comm_fmt->encoding) ;
			error = SFE_UNIMPLEMENTED ;
		} ;

	psf_log_printf (psf, "  Encoding    : %D\n", comm_fmt->encoding) ;
	
	return error ;
} /* aiff_read_comm_chunk */

static int
aiff_write_header (SF_PRIVATE *psf)
{	unsigned char 	comm_sample_rate [10], comm_zero_bytes [2] = { 0, 0 } ;
	unsigned int 	comm_type, comm_size, comm_encoding ;
	int				k ;
	
	switch (psf->sf.format & SF_FORMAT_SUBMASK)
	{	case SF_FORMAT_PCM :					/* Standard big endian AIFF. */
				psf->endian = SF_ENDIAN_BIG ;			
				comm_type = AIFF_MARKER ;
				comm_size = AIFF_COMM_SIZE ;
				comm_encoding = 0 ;
				break ;

		case SF_FORMAT_PCM_BE :					/* Big endian AIFC. */
				psf->endian = SF_ENDIAN_BIG ;
				comm_type = AIFC_MARKER ;
				comm_size = AIFC_COMM_SIZE ;
				comm_encoding = twos_MARKER ;
				break ;
				
		case SF_FORMAT_PCM_LE :					/* Little endian AIFC. */
				psf->endian = SF_ENDIAN_LITTLE ;
				comm_type = AIFC_MARKER ;
				comm_size = AIFC_COMM_SIZE ;
				comm_encoding = sowt_MARKER ;
				break ;

		case SF_FORMAT_FLOAT :					/* Big endian floating point. */
				psf->endian = SF_ENDIAN_BIG ;
				comm_type = AIFC_MARKER ;
				comm_size = AIFC_COMM_SIZE ;
				comm_encoding = FL32_MARKER ; /* Use 'FL32' because its easier to read. */
				psf->has_peak = SF_TRUE ;
				psf->peak_loc = SF_PEAK_START ;
				break ;

		default :	return	SFE_BAD_OPEN_FORMAT ;
		} ;
	
	/* Reset the current header length to zero. */
	psf->header [0] = 0 ;
	psf->headindex = 0 ;
	fseek (psf->file, 0, SEEK_SET) ;

	psf_binheader_writef (psf, "mL", FORM_MARKER, psf->filelength - 8) ;
	
	/* Write COMM chunk. */
	psf_binheader_writef (psf, "mmL", comm_type, COMM_MARKER, comm_size) ;

	uint2tenbytefloat (psf->sf.samplerate, comm_sample_rate)  ;

	psf_binheader_writef (psf, "WLW", psf->sf.channels, psf->sf.samples, psf->sf.pcmbitwidth) ;
	psf_binheader_writef (psf, "B", comm_sample_rate, sizeof (comm_sample_rate)) ;
	
	/* AIFC chunks have some extra data. */
	if (comm_type == AIFC_MARKER)
		psf_binheader_writef (psf, "mB", comm_encoding, comm_zero_bytes, sizeof (comm_zero_bytes)) ;
		
	if (psf->has_peak && psf->peak_loc == SF_PEAK_START)
	{	psf_binheader_writef (psf, "mL", PEAK_MARKER, 
			sizeof (psf->peak) - sizeof (psf->peak.peak) + psf->sf.channels * sizeof (PEAK_POS)) ;
		psf_binheader_writef (psf, "LL", 1, time (NULL)) ;
		for (k = 0 ; k < psf->sf.channels ; k++)
			psf_binheader_writef (psf, "FL", psf->peak.peak[k].value, psf->peak.peak[k].position) ; /* XXXXX */
		} ;
		
	/* Write SSND chunk. */
	psf_binheader_writef (psf, "mLLL", SSND_MARKER, psf->datalength + SSND_CHUNK_SIZE, 0, 0) ;

	/* Header cunstruction complete so write it out. */
	fwrite (psf->header, psf->headindex, 1, psf->file) ;

	psf->dataoffset = psf->headindex ;

	return 0 ;
} /* aiff_write_header */

static int
aiff_write_tailer (SF_PRIVATE *psf)
{	int		k ;
	
	/* Reset the current header length to zero. */
	psf->header [0] = 0 ;
	psf->headindex = 0 ;
	fseek (psf->file, 0, SEEK_SET) ;

	if (psf->has_peak && psf->peak_loc == SF_PEAK_END)
	{	psf_binheader_writef (psf, "mL", PEAK_MARKER, 
			sizeof (psf->peak) - sizeof (psf->peak.peak) + psf->sf.channels * sizeof (PEAK_POS)) ;
		psf_binheader_writef (psf, "LL", 1, time (NULL)) ;
		for (k = 0 ; k < psf->sf.channels ; k++)
			psf_binheader_writef (psf, "FL", psf->peak.peak[k].value, psf->peak.peak[k].position) ; /* XXXXX */
		} ;
		
	if (psf->headindex)
		fwrite (psf->header, psf->headindex, 1, psf->file) ;

	return 0 ;
} /* aiff_write_tailer */

/*-
static void
endswap_comm_fmt (COMM_CHUNK *comm)
{	comm->size            = ENDSWAP_INT (comm->size) ;
	comm->numChannels     = ENDSWAP_SHORT (comm->numChannels) ;
	comm->numSampleFrames = ENDSWAP_INT (comm->numSampleFrames) ;
	comm->sampleSize      = ENDSWAP_SHORT (comm->sampleSize) ;
} /+* endswap_comm_fmt *+/

static void
endswap_ssnd_fmt (SSND_CHUNK *ssnd)
{	ssnd->offset 	= ENDSWAP_INT (ssnd->offset) ;
	ssnd->blocksize = ENDSWAP_INT (ssnd->blocksize) ;
} /+* endswap_ssnd_fmt *+/
-*/

/*==========================================================================================
**	Rough hack at converting from 80 bit IEEE float in AIFF header to an int and
**	back again. It assumes that all sample rates are between 1 and 800MHz, which 
**	should be OK as other sound file formats use a 32 bit integer to store sample 
**	rate.
**	There is another (probably better) version in the source code to the SoX but it
**	has a copyright which probably prevents it from being allowable as GPL/LGPL.
*/

static int
tenbytefloat2int (unsigned char *bytes)
{       int val = 3 ;

	if (bytes [0] & 0x80)   /* Negative number. */
		return 0 ;

	if (bytes [0] <= 0x3F)  /* Less than 1. */
		return 1 ;
		
	if (bytes [0] > 0x40)   /* Way too big. */
		return  0x4000000 ;

	if (bytes [0] == 0x40 && bytes [1] > 0x1C) /* Too big. */
		return 800000000 ;
		
	/* Ok, can handle it. */
		
	val = (bytes [2] << 23) | (bytes [3] << 15) | (bytes [4] << 7)  | (bytes [5] >> 1) ;
	
	val >>= (29 - bytes [1]) ;
	
	return val ;
} /* tenbytefloat2int */

static void
uint2tenbytefloat (unsigned int num, unsigned char *bytes)
{	int		count, mask = 0x40000000 ;

	memset (bytes, 0, 10) ;

	if (num <= 1)
	{	bytes [0] = 0x3F ;
		bytes [1] = 0xFF ;
		bytes [2] = 0x80 ;
		return ;
		} ;

	bytes [0] = 0x40 ;
	
	if (num >= mask)
	{	bytes [1] = 0x1D ;
		return ;
		} ;
	
	for (count = 0 ; count <= 32 ; count ++)
	{	if (num & mask)
			break ;
		mask >>= 1 ;
		} ;
		
	num <<= count + 1 ;
	bytes [1] = 29 - count ;
	bytes [2] = (num >> 24) & 0xFF ;
	bytes [3] = (num >> 16) & 0xFF ;
	bytes [4] = (num >>  8) & 0xFF ;
	bytes [5] = num & 0xFF ;
	
} /* uint2tenbytefloat */

