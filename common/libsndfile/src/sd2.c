/*
** Copyright (C) 2001-2002 Erik de Castro Lopo <erikd@zip.com.au>
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

#include	"sndfile.h"
#include	"config.h"
#include	"sfendian.h"
#include	"common.h"


/*------------------------------------------------------------------------------
 * Macros to handle big/little endian issues.
*/

#define	Sd2f_MARKER		MAKE_MARKER ('S', 'd', '2', 'f')

/*------------------------------------------------------------------------------
 * Typedefs for file chunks.
*/



/*------------------------------------------------------------------------------
 * Private static functions.
*/

static int	sd2_close	(SF_PRIVATE  *psf) ;

/*------------------------------------------------------------------------------
** Public functions.
*/

int 	
sd2_open	(SF_PRIVATE *psf)
{	int		marker, software, rsrc_offset, len ;
	int 	rsrc_data_offset, rsrc_map_offset, rsrc_data_length, rsrc_map_length ;
	char	slen ;
	float	srate ;	

	/* Read only so far. */

	psf_binheader_readf (psf, "Epmmj", 0x41, &marker, &software, 14) ;
	
	if (marker != Sd2f_MARKER)
	{	printf ("Whoops!!!\n") ;
		puts (psf->logbuffer) ;
		exit (1) ;
		} ;

	psf_log_printf (psf, "Marker   : %D\n"
						 "Software : %D\n", 
			marker, software) ;
	
	/* This seems to be a constant for binhex files. */
	psf->dataoffset = 0x80 ;
	
	/* All SD2 files are big endian. */
	psf->endian= SF_ENDIAN_BIG ;

	/*
	**	Resource header info from:
	**	http://developer.apple.com/techpubs/mac/MoreToolbox/MoreToolbox-99.html
	*/

	rsrc_offset = psf->datalength + psf->dataoffset ;
	if (rsrc_offset & 0x7F)
		rsrc_offset = rsrc_offset - (rsrc_offset & 0x7F) + psf->dataoffset ;

	psf_log_printf (psf, "Resource offset : 0x%X\n", rsrc_offset) ;

	/* Jump to the rsrc_offset fork section. */
	psf_binheader_readf (psf, "Ep", rsrc_offset) ;
	
	psf_binheader_readf (psf, "E4444", &rsrc_data_offset, &rsrc_map_offset, &rsrc_data_length, &rsrc_map_length) ;

	rsrc_data_offset += rsrc_offset ;
	rsrc_map_offset  += rsrc_offset ;

	psf_log_printf (psf, " data offset : 0x%X\n"
						 " map  offset : 0x%X\n"
						 " data length : 0x%X\n"
						 " map  length : 0x%X\n",
	
			rsrc_data_offset, rsrc_map_offset, rsrc_data_length, rsrc_map_length) ;
			
	if (rsrc_data_offset + rsrc_data_length > rsrc_map_offset || rsrc_map_offset + rsrc_map_length > psf->filelength)
	{	puts ("##############################") ;
		puts (psf->logbuffer) ;
		puts ("##############################") ;
		exit (1) ;
		} ;

	memset (psf->buffer, 0, sizeof (psf->buffer)) ;
	
	psf_binheader_readf (psf, "Ep41", rsrc_data_offset, &len, &slen) ;
	if (slen + 1 == len)
	{	psf_binheader_readf (psf, "Eb", psf->buffer, len - 1) ;
		((char*) psf->buffer) [len - 1] = 0 ;
		if (sscanf ((char*) psf->buffer, "%d", &len) == 1)
			psf->bytewidth = len ; 
		} ;
		
	psf_binheader_readf (psf, "E41", &len, &slen) ;
	if (slen + 1 == len)
	{	psf_binheader_readf (psf, "Eb", psf->buffer, len - 1) ;
		((char*) psf->buffer) [len - 1] = 0 ;
		if (sscanf ((char*) psf->buffer, "%f", &srate) == 1)
			psf->sf.samplerate = srate ; 
		} ;
		
	psf_binheader_readf (psf, "E41", &len, &slen) ;
	if (slen + 1 == len)
	{	psf_binheader_readf (psf, "Eb", psf->buffer, len - 1) ;
		((char*) psf->buffer) [len - 1] = 0 ;
		if (sscanf ((char*) psf->buffer, "%d", &len) == 1)
			psf->sf.channels = len ; 
		} ;
		
	psf_log_printf (psf, "  byte width  : %d\n", psf->bytewidth) ;
	psf_log_printf (psf, "  sample rate : %d\n", psf->sf.samplerate) ;
	psf_log_printf (psf, "  channels    : %d\n", psf->sf.channels) ;

	if (psf->bytewidth == 2)
	{	psf->sf.format = SF_FORMAT_SD2 | SF_FORMAT_PCM_16 ;
		
		psf->blockwidth = psf->bytewidth * psf->sf.channels ;
		
		psf->sf.samples = psf->datalength / psf->blockwidth ;
		} ;
	
	pcm_init (psf) ;

	psf_fseek (psf->filedes, psf->dataoffset, SEEK_SET) ;

	psf->close = sd2_close ;
	
	return 0 ;
} /* sd2_open */

/*------------------------------------------------------------------------------
*/

static int	
sd2_close	(SF_PRIVATE  *psf)
{	
	if (psf->mode == SFM_WRITE)
	{	/*  Now we know for certain the audio_length of the file we can re-write 
		**	correct values for the FORM, 8SVX and BODY chunks.
		*/
                
		} ;

	if (psf->fdata)
		free (psf->fdata) ;
	psf->fdata = NULL ;
	
	return 0 ;
} /* sd2_close */

