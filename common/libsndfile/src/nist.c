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

/*
**	Some of the information used to read NIST files was gleaned from 
**	reading the code of Bill Schottstaedt's sndlib library 
**		ftp://ccrma-ftp.stanford.edu/pub/Lisp/sndlib.tar.gz
**	However, no code from that package was used.
*/

#include	<stdio.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<string.h>
#include	<ctype.h>

#include	"sndfile.h"
#include	"config.h"
#include	"sfendian.h"
#include	"common.h"

/*------------------------------------------------------------------------------
*/

#define  NIST_HEADER_LENGTH    1024

/*------------------------------------------------------------------------------
** Private static functions.
*/

static	int	nist_close	(SF_PRIVATE *psf) ;
static	void nist_write_header (SF_PRIVATE  *psf) ;

/*------------------------------------------------------------------------------
*/

int 	
nist_open_read	(SF_PRIVATE *psf)
{	char	*psf_header ;
	int		error ;

	fseek (psf->file, 0, SEEK_SET) ;
	
	psf_header = (char*) psf->header ;

	fgets (psf_header, SF_HEADER_LEN, psf->file) ;
	psf_log_printf (psf, psf_header) ;
	if (strlen (psf_header) != 8 || strcmp (psf_header, "NIST_1A\n"))
		return SFE_NIST_BAD_HEADER ;
	
	fgets (psf_header, SF_HEADER_LEN, psf->file) ;
	psf_log_printf (psf, psf_header) ;
	if (strlen (psf_header) != 8 || atoi (psf_header) != 1024)
		return SFE_NIST_BAD_HEADER ;

	while (ftell (psf->file) < 1024 && !ferror (psf->file))
	{	fgets (psf_header, SF_HEADER_LEN, psf->file) ;
		psf_log_printf (psf, psf_header) ;
		
		if (strstr (psf_header, "channel_count -i ") == psf_header)
			sscanf (psf_header, "channel_count -i %u", &(psf->sf.channels)) ;
		
		if (strstr (psf_header, "sample_count -i ") == psf_header)
			sscanf (psf_header, "sample_count -i %u", &(psf->sf.samples)) ;

		if (strstr (psf_header, "sample_rate -i ") == psf_header)
			sscanf (psf_header, "sample_rate -i %u", &(psf->sf.samplerate)) ;

		if (strstr (psf_header, "sample_n_bytes -i ") == psf_header)
			sscanf (psf_header, "sample_n_bytes -i %u", &(psf->bytewidth)) ;

		if (strstr (psf_header, "sample_sig_bits -i ") == psf_header)
			sscanf (psf_header, "sample_sig_bits -i %u", &(psf->sf.pcmbitwidth)) ;

		if (strstr (psf_header, "sample_byte_format -s") == psf_header)
		{	int bytes ;
			char str [8] = { 0, 0, 0, 0, 0, 0, 0, 0 } ;
			
			sscanf (psf_header, "sample_byte_format -s%d %5s", &bytes, str) ;
			if (bytes < 2 || bytes > 4)
				return SFE_NIST_BAD_ENCODING ;
				
			psf->bytewidth = bytes ;
				
			if (strstr (str, "01") == str)
			{	psf->endian    = SF_ENDIAN_LITTLE ;
				psf->sf.format = SF_FORMAT_NIST | SF_FORMAT_PCM_LE ;
				}
			else if (strstr (str, "10"))
			{	psf->endian    = SF_ENDIAN_BIG ;
				psf->sf.format = SF_FORMAT_NIST | SF_FORMAT_PCM_BE ;
				} ;
			} ;

		if (strstr (psf_header, "sample_coding -s") == psf_header)
			return SFE_NIST_BAD_ENCODING ;

		if (strstr (psf_header, "end_head") == psf_header)
			break ;
		} ;
		
 	psf->dataoffset = NIST_HEADER_LENGTH ;
 	psf->current  = 0 ;
	psf->sf.seekable = SF_TRUE ;
	psf->sf.sections = 1 ;

	psf->close = (func_close) nist_close ;

	psf->blockwidth = psf->sf.channels * psf->bytewidth ;
	psf->datalength = psf->filelength - psf->dataoffset ;

	if ((error = pcm_read_init (psf)))
		return error ;

	fseek (psf->file, psf->dataoffset, SEEK_SET) ;

	return 0 ;
} /* nist_open_read */

/*------------------------------------------------------------------------------
*/

int 	
nist_open_write	(SF_PRIVATE *psf)
{	int subformat, error ;
	
	if ((psf->sf.format & SF_FORMAT_TYPEMASK) != SF_FORMAT_NIST)
		return	SFE_BAD_OPEN_FORMAT ;

	subformat = psf->sf.format & SF_FORMAT_SUBMASK ;

	if (subformat == SF_FORMAT_PCM_BE)
		psf->endian = SF_ENDIAN_BIG ;
	else if (subformat == SF_FORMAT_PCM_LE)
		psf->endian = SF_ENDIAN_LITTLE ;
	else if (CPU_IS_BIG_ENDIAN && subformat == SF_FORMAT_PCM)
		psf->endian = SF_ENDIAN_BIG ;
	else if (CPU_IS_LITTLE_ENDIAN && subformat == SF_FORMAT_PCM)
		psf->endian = SF_ENDIAN_LITTLE ;
	else
		return	SFE_BAD_OPEN_FORMAT ;
		
	psf->bytewidth = BITWIDTH2BYTES (psf->sf.pcmbitwidth) ;
		
	psf->blockwidth  = psf->bytewidth * psf->sf.channels ;
 	psf->dataoffset  = NIST_HEADER_LENGTH ;
	psf->datalength  = psf->blockwidth * psf->sf.samples ;
	psf->filelength  = psf->datalength + psf->dataoffset ;
	psf->error       = 0 ;

	if ((error = pcm_write_init (psf)))
		return error ;

	psf->close        = (func_close)  nist_close ;
	psf->write_header = (func_wr_hdr) nist_write_header ;
		
	nist_write_header (psf) ;
	
	return 0 ;
} /* nist_open_write */

/*------------------------------------------------------------------------------
*/

static int	
nist_close	(SF_PRIVATE  *psf)
{
	if (psf->mode == SF_MODE_WRITE)
	{	/*  Now we know for certain the length of the file we can
		**  re-write correct values for the datasize header element.
		*/

		fseek (psf->file, 0, SEEK_END) ;
		psf->filelength = ftell (psf->file) ;

		psf->dataoffset = NIST_HEADER_LENGTH ;
		psf->datalength = psf->filelength - psf->dataoffset ;

		psf->sf.samples = psf->datalength / psf->blockwidth ;

		nist_write_header (psf) ;
		} ;

	if (psf->fdata)
		free (psf->fdata) ;
	psf->fdata = NULL ;
	
	return 0 ;
} /* nist_close */

/*=========================================================================
*/

static void 
nist_write_header (SF_PRIVATE  *psf)
{	char	*end_str ;

	if (psf->endian == SF_ENDIAN_BIG)
		end_str = "10" ;
	else if (psf->endian == SF_ENDIAN_LITTLE)
		end_str = "01" ;
	else
		end_str = "error" ;

	/* Clear the whole header. */
	memset (psf->header, 0, sizeof (psf->header)) ;
		
	psf_asciiheader_printf (psf, "NIST_1A\n   1024\n") ;
	psf_asciiheader_printf (psf, "channel_count -i %d\n", psf->sf.channels) ;
	psf_asciiheader_printf (psf, "sample_rate -i %d\n", psf->sf.samplerate) ;
	psf_asciiheader_printf (psf, "sample_n_bytes -i %d\n", psf->bytewidth) ;
	psf_asciiheader_printf (psf, "sample_byte_format -s%d %s\n", psf->bytewidth, end_str) ;
	psf_asciiheader_printf (psf, "sample_sig_bits -i %d\n", psf->sf.pcmbitwidth) ;
	psf_asciiheader_printf (psf, "sample_count -i %d\n", psf->sf.samples) ;
	psf_asciiheader_printf (psf, "end_head\n") ;

	fseek (psf->file, 0, SEEK_SET) ;

	/* Zero fill to dataoffset. */
	psf_binheader_writef (psf, "z", NIST_HEADER_LENGTH - psf->headindex) ;

	fwrite (psf->header, psf->headindex, 1, psf->file) ;

	return ;		
} /* nist_write_header */

/*-

These were used to parse the sample_byte_format field but were discarded in favour
os a simpler method using strstr ().

static  
int strictly_ascending (char *str)
{	int  k ;

	if (strlen (str) < 2)
		return 0 ;

	for (k = 1 ; str [k] ; k++) 
		if (str [k] != str [k-1] + 1)
			return 0 ;
	
	return 1 ;
} /+* strictly_ascending *+/

static  int strictly_descending (char *str)
{	int  k ;

	if (strlen (str) < 2)
		return 0 ;

	for (k = 1 ; str [k] ; k++) 
		if (str [k] + 1 != str [k-1])
			return 0 ;
	
	return 1 ;
} /+* strictly_descending *+/

-*/
