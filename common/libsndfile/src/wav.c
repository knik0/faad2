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
#include	<string.h>
#include	<ctype.h>
#include	<time.h>

#include	"sndfile.h"
#include	"config.h"
#include	"sfendian.h"
#include	"common.h"
#include	"wav_w64.h"

/*------------------------------------------------------------------------------
 * Macros to handle big/little endian issues.
 */

#define RIFF_MARKER	(MAKE_MARKER ('R', 'I', 'F', 'F')) 
#define WAVE_MARKER	(MAKE_MARKER ('W', 'A', 'V', 'E')) 
#define fmt_MARKER	(MAKE_MARKER ('f', 'm', 't', ' ')) 
#define data_MARKER	(MAKE_MARKER ('d', 'a', 't', 'a')) 
#define fact_MARKER	(MAKE_MARKER ('f', 'a', 'c', 't')) 
#define PEAK_MARKER	(MAKE_MARKER ('P', 'E', 'A', 'K')) 

#define cue_MARKER	(MAKE_MARKER ('c', 'u', 'e', ' ')) 
#define LIST_MARKER	(MAKE_MARKER ('L', 'I', 'S', 'T')) 
#define slnt_MARKER	(MAKE_MARKER ('s', 'l', 'n', 't')) 
#define wavl_MARKER	(MAKE_MARKER ('w', 'a', 'v', 'l')) 
#define INFO_MARKER	(MAKE_MARKER ('I', 'N', 'F', 'O')) 
#define plst_MARKER	(MAKE_MARKER ('p', 'l', 's', 't')) 
#define adtl_MARKER	(MAKE_MARKER ('a', 'd', 't', 'l')) 
#define labl_MARKER	(MAKE_MARKER ('l', 'a', 'b', 'l')) 
#define ltxt_MARKER	(MAKE_MARKER ('l', 't', 'x', 't')) 
#define note_MARKER	(MAKE_MARKER ('n', 'o', 't', 'e')) 
#define smpl_MARKER	(MAKE_MARKER ('s', 'm', 'p', 'l')) 
#define bext_MARKER	(MAKE_MARKER ('b', 'e', 'x', 't')) 
#define MEXT_MARKER	(MAKE_MARKER ('M', 'E', 'X', 'T')) 
#define DISP_MARKER	(MAKE_MARKER ('D', 'I', 'S', 'P')) 
#define acid_MARKER	(MAKE_MARKER ('a', 'c', 'i', 'd')) 
#define PAD_MARKER	(MAKE_MARKER ('P', 'A', 'D', ' ')) 
#define adtl_MARKER	(MAKE_MARKER ('a', 'd', 't', 'l')) 
#define afsp_MARKER	(MAKE_MARKER ('a', 'f', 's', 'p'))

#define ISFT_MARKER	(MAKE_MARKER ('I', 'S', 'F', 'T')) 
#define ICRD_MARKER	(MAKE_MARKER ('I', 'C', 'R', 'D')) 
#define ICOP_MARKER	(MAKE_MARKER ('I', 'C', 'O', 'P')) 
#define IART_MARKER	(MAKE_MARKER ('I', 'A', 'R', 'T')) 
#define INAM_MARKER	(MAKE_MARKER ('I', 'N', 'A', 'M')) 
#define IENG_MARKER	(MAKE_MARKER ('I', 'E', 'N', 'G')) 
#define IART_MARKER	(MAKE_MARKER ('I', 'A', 'R', 'T')) 
#define ICOP_MARKER	(MAKE_MARKER ('I', 'C', 'O', 'P')) 
#define IPRD_MARKER	(MAKE_MARKER ('I', 'P', 'R', 'D')) 
#define ISRC_MARKER	(MAKE_MARKER ('I', 'S', 'R', 'C')) 
#define ISBJ_MARKER	(MAKE_MARKER ('I', 'S', 'B', 'J')) 
#define ICMT_MARKER	(MAKE_MARKER ('I', 'C', 'M', 'T')) 

enum 
{	HAVE_RIFF	= 0x01,
	HAVE_WAVE	= 0x02,
	HAVE_fmt	= 0x04,
	HAVE_fact	= 0x08,
	HAVE_PEAK	= 0x10,
	HAVE_data	= 0x20
} ;

/*------------------------------------------------------------------------------
 * Private static functions.
 */

static int	wav_read_header	(SF_PRIVATE *psf, int *blockalign, int *samplesperblock) ;
static int	wav_write_header (SF_PRIVATE *psf) ;
static int	wav_write_tailer (SF_PRIVATE *psf) ;

static int	wav_close	(SF_PRIVATE  *psf) ;

static int 	wav_subchunk_parse	(SF_PRIVATE *psf, int chunk) ;

/*------------------------------------------------------------------------------
** Public function.
*/

int
wav_open	(SF_PRIVATE *psf)
{	int	subformat, error, blockalign = 0, samplesperblock = 0 ;
	
	if (psf->mode == SFM_READ || (psf->mode == SFM_RDWR && psf->filelength > 0))
	{	if ((error = wav_read_header (psf, &blockalign, &samplesperblock)))
			return error ;
		};
		
	subformat = psf->sf.format & SF_FORMAT_SUBMASK ;

	if (psf->mode == SFM_WRITE || psf->mode == SFM_RDWR)
	{	if ((psf->sf.format & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV)
			return	SFE_BAD_OPEN_FORMAT ;
		
		psf->endian = SF_ENDIAN_LITTLE ;	/* All WAV files are little endian. */
	
		psf->blockwidth  = psf->bytewidth * psf->sf.channels ;

		if (subformat == SF_FORMAT_IMA_ADPCM || subformat == SF_FORMAT_MS_ADPCM)
		{	blockalign = wav_w64_srate2blocksize (psf->sf.samplerate * psf->sf.channels) ;
			samplesperblock = -1 ;

			/* FIXME : This block must go */
			psf->filelength = SF_MAX_UINT ;
			psf->datalength = SF_MAX_UINT ;
			if (psf->sf.samples <= 0)
				psf->sf.samples = (psf->blockwidth) ? SF_MAX_COUNT / psf->blockwidth : SF_MAX_COUNT ; 
			/* EMXIF : This block must go */
			} ;

		if ((error = wav_write_header (psf)))
			return error ;

		psf->write_header = wav_write_header ;
		} ;
	
	psf->close = wav_close ;
	
	switch (subformat)
	{	case SF_FORMAT_PCM_U8 :
					psf->chars = SF_CHARS_UNSIGNED ;
					error = pcm_init (psf) ;
					break ;

		case SF_FORMAT_PCM_16 : 
		case SF_FORMAT_PCM_24 : 
		case SF_FORMAT_PCM_32 : 
					error = pcm_init (psf) ;
					break ;

		case SF_FORMAT_FLOAT : 
					error = float32_init (psf) ;
					break ;

		case SF_FORMAT_DOUBLE : 
					error = double64_init (psf) ;
					break ;

		case SF_FORMAT_ULAW : 
					error = ulaw_init (psf) ;
					break ;
					
		case SF_FORMAT_ALAW : 
					error = alaw_init (psf) ;
					break ;

		case SF_FORMAT_IMA_ADPCM : 
					error = wav_w64_ima_init (psf, blockalign, samplesperblock) ;
					break ;

		case SF_FORMAT_MS_ADPCM : 
					error = wav_w64_msadpcm_init (psf, blockalign, samplesperblock) ;
					break ;

		case SF_FORMAT_GSM610 : 
					error = gsm610_init (psf) ;
					break ;

		default : 	return SFE_UNIMPLEMENTED ;
		} ;

	return error ;
} /* wav_open */

/*=========================================================================
** Private functions.
*/

static int
wav_read_header	(SF_PRIVATE *psf, int *blockalign, int *samplesperblock)
{	WAV_FMT		wav_fmt ;
	FACT_CHUNK	fact_chunk ;
	int			dword, marker, RIFFsize ;
	int			parsestage = 0, error, format = 0 ;
	char		*cptr ;	
	
	/* Set position to start of file to begin reading header. */
	psf_binheader_readf (psf, "p", 0) ;	
		
	while (1)
	{	psf_binheader_readf (psf, "m", &marker) ;
		switch (marker)
		{	case RIFF_MARKER :
					if (parsestage)
						return SFE_WAV_NO_RIFF ;

					psf_binheader_readf (psf, "e4", &RIFFsize) ;
					
					if (psf->filelength  < RIFFsize + 2 * SIGNED_SIZEOF (dword))
					{	dword = psf->filelength - 2 * SIGNED_SIZEOF (dword);
						psf_log_printf (psf, "RIFF : %d (should be %d)\n", RIFFsize, dword) ;
						RIFFsize = dword ;
						}
					else
						psf_log_printf (psf, "RIFF : %d\n", RIFFsize) ;
					parsestage |= HAVE_RIFF ;
					break ;
					
			case WAVE_MARKER :
					if ((parsestage & HAVE_RIFF) != HAVE_RIFF)
						return SFE_WAV_NO_WAVE ;
					psf_log_printf (psf, "WAVE\n") ;
					parsestage |= HAVE_WAVE ;
					break ;
			
			case fmt_MARKER :
					if ((parsestage & (HAVE_RIFF | HAVE_WAVE)) != (HAVE_RIFF | HAVE_WAVE))
						return SFE_WAV_NO_FMT ;

					psf_binheader_readf (psf, "e4", &dword) ;
					psf_log_printf (psf, "fmt  : %d\n", dword) ;
	
					if ((error = wav_w64_read_fmt_chunk (psf, &wav_fmt, dword)))
						return error ;

					format     = wav_fmt.format ;
					parsestage |= HAVE_fmt ;
					break ;
					
			case data_MARKER :
					if ((parsestage & (HAVE_RIFF | HAVE_WAVE | HAVE_fmt)) != (HAVE_RIFF | HAVE_WAVE | HAVE_fmt))
						return SFE_WAV_NO_DATA ;
					
					psf_binheader_readf (psf, "e4", &dword) ;
					psf->datalength = dword ;

					psf->dataoffset = psf_ftell (psf->filedes) ;
					
					if (psf->datalength > psf->filelength - psf->dataoffset)
					{	psf_log_printf (psf, "data : %d (should be %d)\n", psf->datalength, psf->filelength - psf->dataoffset) ;
						psf->datalength = psf->filelength - psf->dataoffset ;
						}
					else
						psf_log_printf (psf, "data : %d\n", psf->datalength) ;

					if (format == WAVE_FORMAT_MS_ADPCM && psf->datalength % 2)
					{	psf->datalength ++ ;
						psf_log_printf (psf, "*** Data length odd. Increasing it by 1.\n") ;
						} ;
		
					parsestage |= HAVE_data ;

					if (! psf->sf.seekable)
						break ;
					
					/* Seek past data and continue reading header. */
					psf_fseek (psf->filedes, psf->datalength, SEEK_CUR) ;

					dword = psf_ftell (psf->filedes) ;
					if (dword != (sf_count_t) (psf->dataoffset + psf->datalength))
						psf_log_printf (psf, "*** psf_fseek past end error ***\n", dword, psf->dataoffset + psf->datalength) ;
					break ;

			case fact_MARKER :
					if ((parsestage & (HAVE_RIFF | HAVE_WAVE | HAVE_fmt)) != (HAVE_RIFF | HAVE_WAVE | HAVE_fmt))
						return SFE_WAV_BAD_FACT ;

					psf_binheader_readf (psf, "e44", &dword, &(fact_chunk.samples)) ;
					
					if (dword > SIGNED_SIZEOF (fact_chunk))
						psf_binheader_readf (psf, "j", (int) (dword - SIGNED_SIZEOF (fact_chunk))) ;
				
					if (dword)
						psf_log_printf (psf, "%D : %d\n", marker, dword) ;
					else
						psf_log_printf (psf, "%D : %d (should not be zero)\n", marker, dword) ;
					
					psf_log_printf (psf, "  samples : %d\n", fact_chunk.samples) ;
					parsestage |= HAVE_fact ;
					break ;

			case PEAK_MARKER :
					if ((parsestage & (HAVE_RIFF | HAVE_WAVE | HAVE_fmt)) != (HAVE_RIFF | HAVE_WAVE | HAVE_fmt))
						return SFE_WAV_PEAK_B4_FMT ;

					psf_binheader_readf (psf, "e4", &dword) ;
					
					psf_log_printf (psf, "%D : %d\n", marker, dword) ;
					if (dword > SIGNED_SIZEOF (psf->peak))
					{	psf_binheader_readf (psf, "j", dword) ;
						psf_log_printf (psf, "*** File PEAK chunk bigger than sizeof (PEAK_CHUNK).\n") ;
						return SFE_WAV_BAD_PEAK ;
						} ;
					if (dword != SIGNED_SIZEOF (psf->peak) - SIGNED_SIZEOF (psf->peak.peak) + psf->sf.channels * SIGNED_SIZEOF (PEAK_POS))
					{	psf_binheader_readf (psf, "j", dword) ;
						psf_log_printf (psf, "*** File PEAK chunk size doesn't fit with number of channels.\n") ;
						return SFE_WAV_BAD_PEAK ;
						} ;
					
					psf_binheader_readf (psf, "e44", &(psf->peak.version), &(psf->peak.timestamp)) ;

					if (psf->peak.version != 1)
						psf_log_printf (psf, "  version    : %d *** (should be version 1)\n", psf->peak.version) ;
					else
						psf_log_printf (psf, "  version    : %d\n", psf->peak.version) ;
						
					psf_log_printf (psf, "  time stamp : %d\n", psf->peak.timestamp) ;
					psf_log_printf (psf, "    Ch   Position       Value\n") ;

					cptr = (char *) psf->buffer ;
					for (dword = 0 ; dword < psf->sf.channels ; dword++)
					{	psf_binheader_readf (psf, "ef4", &(psf->peak.peak[dword].value), 
														&(psf->peak.peak[dword].position)) ;
					
						LSF_SNPRINTF (cptr, sizeof (psf->buffer), "    %2d   %-12d   %g\n", 
								dword, psf->peak.peak[dword].position, psf->peak.peak[dword].value) ;
						cptr [sizeof (psf->buffer) - 1] = 0 ;
						psf_log_printf (psf, cptr) ;
						};

					psf->has_peak = SF_TRUE ;
					break ;

			case INFO_MARKER :
			case LIST_MARKER :
					if ((error = wav_subchunk_parse (psf, marker)))
						return error ;
					break ;
			
			case bext_MARKER :
			case cue_MARKER :
			case DISP_MARKER :
			case MEXT_MARKER :
					psf_binheader_readf (psf, "e4", &dword);
					psf_log_printf (psf, "%D : %d\n", marker, dword) ;
					dword += (dword & 1) ;
					psf_binheader_readf (psf, "j", dword) ;
					break ;

			case smpl_MARKER :
			case acid_MARKER :
			case PAD_MARKER :
			case afsp_MARKER :
					psf_binheader_readf (psf, "e4", &dword);
					psf_log_printf (psf, " *** %D : %d\n", marker, dword) ;
					dword += (dword & 1) ;
					psf_binheader_readf (psf, "j", dword) ;
					break ;


			default : 
					if (isprint ((marker >> 24) & 0xFF) && isprint ((marker >> 16) & 0xFF)
						&& isprint ((marker >> 8) & 0xFF) && isprint (marker & 0xFF))
					{	psf_binheader_readf (psf, "e4", &dword);
						psf_log_printf (psf, "*** %D : %d (unknown marker)\n", marker, dword) ;

						psf_binheader_readf (psf, "j", dword);
						break ;
						} ;
					if (psf_ftell (psf->filedes) & 0x03)
					{	psf_log_printf (psf, "  Unknown chunk marker at position %d. Resynching.\n", dword - 4) ;
						psf_binheader_readf (psf, "j", -3) ;
						break ;
						} ;
					psf_log_printf (psf, "*** Unknown chunk marker : %X. Exiting parser.\n", marker) ;
					break ;
			} ;	/* switch (dword) */

		if (! psf->sf.seekable && (parsestage & HAVE_data))
			break ;

		if (psf_ferror (psf->filedes))
		{	psf_log_printf (psf, "*** Error on file handle. ***\n", marker) ;
			psf_fclearerr (psf->filedes) ;
			break ;
			} ;

		if (psf_ftell (psf->filedes) >= (int) (psf->filelength - (2 * sizeof (dword))))
			break ;

		if (psf->logindex >= sizeof (psf->logbuffer) - 2)
			return SFE_LOG_OVERRUN ;
		} ; /* while (1) */
		
	if (! psf->dataoffset)
		return SFE_WAV_NO_DATA ;

	psf->endian = SF_ENDIAN_LITTLE ;		/* All WAV files are little endian. */
	
	psf_fseek (psf->filedes, psf->dataoffset, SEEK_SET) ;
	
	psf->close = wav_close ;

	if (psf->blockwidth)
	{	if (psf->filelength - psf->dataoffset < psf->datalength)
			psf->sf.samples = (psf->filelength - psf->dataoffset) / psf->blockwidth ;
		else
			psf->sf.samples = psf->datalength / psf->blockwidth ;
		} ;

	switch (format)
	{	case WAVE_FORMAT_PCM :
		case WAVE_FORMAT_EXTENSIBLE :
					psf->sf.format = SF_FORMAT_WAV | u_bitwidth_to_subformat (psf->bytewidth * 8) ;
					break ;
					
		case WAVE_FORMAT_MULAW :
					psf->sf.format = (SF_FORMAT_WAV | SF_FORMAT_ULAW) ;
					break ;
	
		case WAVE_FORMAT_ALAW :
					psf->sf.format = (SF_FORMAT_WAV | SF_FORMAT_ALAW) ;
					break ;
		
		case WAVE_FORMAT_MS_ADPCM : 
					psf->sf.format = (SF_FORMAT_WAV | SF_FORMAT_MS_ADPCM) ;
					*blockalign = wav_fmt.msadpcm.blockalign ;
					*samplesperblock = wav_fmt.msadpcm.samplesperblock ;
					break ;

		case WAVE_FORMAT_IMA_ADPCM :
					psf->sf.format = (SF_FORMAT_WAV | SF_FORMAT_IMA_ADPCM) ;
					*blockalign = wav_fmt.ima.blockalign ;
					*samplesperblock = wav_fmt.ima.samplesperblock ;
					break ;
		
		case WAVE_FORMAT_GSM610 :
					psf->sf.format = (SF_FORMAT_WAV | SF_FORMAT_GSM610) ;
					break ;
		
		case WAVE_FORMAT_IEEE_FLOAT :
					psf->sf.format  = SF_FORMAT_WAV ;
					psf->sf.format |= (psf->bytewidth == 8) ? SF_FORMAT_DOUBLE : SF_FORMAT_FLOAT ;
					break ;
		
		default : return SFE_UNIMPLEMENTED ;
		} ;

	return 0 ;
} /* wav_read_header */

static int 
wav_write_header (SF_PRIVATE *psf)
{	int 	fmt_size, k, subformat, add_fact_chunk = SF_FALSE ;
	
	/* Reset the current header length to zero. */
	psf->header [0] = 0 ;
	psf->headindex = 0 ;
	psf_fseek (psf->filedes, 0, SEEK_SET) ;

	/* RIFF marker, length, WAVE and 'fmt ' markers. */		
	psf_binheader_writef (psf, "etm8mm", RIFF_MARKER, psf->filelength - 8, WAVE_MARKER, fmt_MARKER) ;

	subformat = psf->sf.format & SF_FORMAT_SUBMASK ;

	switch (subformat)
	{	case SF_FORMAT_PCM_U8 :
		case SF_FORMAT_PCM_16 : 
		case SF_FORMAT_PCM_24 : 
		case SF_FORMAT_PCM_32 : 
					fmt_size = 2 + 2 + 4 + 4 + 2 + 2 ;

					/* fmt : format, channels, samplerate */
					psf_binheader_writef (psf, "e4224", fmt_size, WAVE_FORMAT_PCM, psf->sf.channels, psf->sf.samplerate) ;
					/*  fmt : bytespersec */
					psf_binheader_writef (psf, "e4", psf->sf.samplerate * psf->bytewidth * psf->sf.channels) ;
					/*  fmt : blockalign, bitwidth */
					psf_binheader_writef (psf, "e22", psf->bytewidth * psf->sf.channels, psf->bytewidth * 8) ;
					break ;

		case SF_FORMAT_FLOAT : 
		case SF_FORMAT_DOUBLE : 
					/* Add the peak chunk to floating point files. */					
					psf->has_peak = SF_TRUE ;
					psf->peak_loc = SF_PEAK_START ;
					
					fmt_size = 2 + 2 + 4 + 4 + 2 + 2 ;

					/* fmt : format, channels, samplerate */
					psf_binheader_writef (psf, "e4224", fmt_size, WAVE_FORMAT_IEEE_FLOAT, psf->sf.channels, psf->sf.samplerate) ;
					/*  fmt : bytespersec */
					psf_binheader_writef (psf, "e4", psf->sf.samplerate * psf->bytewidth * psf->sf.channels) ;
					/*  fmt : blockalign, bitwidth */
					psf_binheader_writef (psf, "e22", psf->bytewidth * psf->sf.channels, psf->bytewidth * 8) ;

					add_fact_chunk = SF_TRUE ;
					break ;

		case SF_FORMAT_ULAW : 
					fmt_size = 2 + 2 + 4 + 4 + 2 + 2 ;
					
					/* fmt : format, channels, samplerate */
					psf_binheader_writef (psf, "e4224", fmt_size, WAVE_FORMAT_MULAW, psf->sf.channels, psf->sf.samplerate) ;
					/*  fmt : bytespersec */
					psf_binheader_writef (psf, "e4", psf->sf.samplerate * psf->bytewidth * psf->sf.channels) ;
					/*  fmt : blockalign, bitwidth */
					psf_binheader_writef (psf, "e22", psf->bytewidth * psf->sf.channels, 8) ;

					add_fact_chunk = SF_TRUE ;
					break ;
					
		case SF_FORMAT_ALAW : 
					fmt_size = 2 + 2 + 4 + 4 + 2 + 2 ;

					/* fmt : format, channels, samplerate */
					psf_binheader_writef (psf, "e4224", fmt_size, WAVE_FORMAT_ALAW, psf->sf.channels, psf->sf.samplerate) ;
					/*  fmt : bytespersec */
					psf_binheader_writef (psf, "e4", psf->sf.samplerate * psf->bytewidth * psf->sf.channels) ;
					/*  fmt : blockalign, bitwidth */
					psf_binheader_writef (psf, "e22", psf->bytewidth * psf->sf.channels, 8) ;

					add_fact_chunk = SF_TRUE ;
					break ;

		case SF_FORMAT_IMA_ADPCM : 
					{	int  blockalign, samplesperblock, bytespersec ;

						blockalign      = wav_w64_srate2blocksize (psf->sf.samplerate * psf->sf.channels) ;
						samplesperblock = 2 * (blockalign - 4 * psf->sf.channels) / psf->sf.channels + 1 ;
						bytespersec     = (psf->sf.samplerate * blockalign) / samplesperblock ;
	
						/* fmt chunk. */
						fmt_size = 2 + 2 + 4 + 4 + 2 + 2 + 2 + 2 ;
	
						/* fmt : size, WAV format type, channels, samplerate, bytespersec */
						psf_binheader_writef (psf, "e42244", fmt_size, WAVE_FORMAT_IMA_ADPCM, 
									psf->sf.channels, psf->sf.samplerate, bytespersec) ;

						/* fmt : blockalign, bitwidth, extrabytes, samplesperblock. */
						psf_binheader_writef (psf, "e2222", blockalign, 4, 2, samplesperblock) ;
						} ;
					add_fact_chunk = SF_TRUE ;
					break ;

		case SF_FORMAT_MS_ADPCM : 
					{	int  blockalign, samplesperblock, bytespersec, extrabytes ;

						blockalign      = wav_w64_srate2blocksize (psf->sf.samplerate * psf->sf.channels) ;	
						samplesperblock = 2 + 2 * (blockalign - 7 * psf->sf.channels) / psf->sf.channels ;
						bytespersec     = (psf->sf.samplerate * blockalign) / samplesperblock ;
	
						/* fmt chunk. */
						extrabytes = 2 + 2 + MSADPCM_ADAPT_COEFF_COUNT * (2 + 2) ;
						fmt_size   = 2 + 2 + 4 + 4 + 2 + 2 + 2 + extrabytes ;
	
						/* fmt : size, WAV format type, channels. */
						psf_binheader_writef (psf, "e422", fmt_size, WAVE_FORMAT_MS_ADPCM, psf->sf.channels) ;

						/* fmt : samplerate, bytespersec. */
						psf_binheader_writef (psf, "e44", psf->sf.samplerate, bytespersec) ;

						/* fmt : blockalign, bitwidth, extrabytes, samplesperblock. */
						psf_binheader_writef (psf, "e22222", blockalign, 4, extrabytes, samplesperblock, 7) ;

						msadpcm_write_adapt_coeffs (psf) ;
						} ;

					add_fact_chunk = SF_TRUE ;
					break ;

		case SF_FORMAT_GSM610 : 
					{	int  blockalign, samplesperblock, bytespersec ;

						blockalign      = WAV_W64_GSM610_BLOCKSIZE ;
						samplesperblock = WAV_W64_GSM610_SAMPLES ;
						bytespersec     = (psf->sf.samplerate * blockalign) / samplesperblock ;

						/* fmt chunk. */
						fmt_size = 2 + 2 + 4 + 4 + 2 + 2 + 2 + 2 ;
	
						/* fmt : size, WAV format type, channels. */
						psf_binheader_writef (psf, "e422", fmt_size, WAVE_FORMAT_GSM610, psf->sf.channels) ;
	
						/* fmt : samplerate, bytespersec. */
						psf_binheader_writef (psf, "e44", psf->sf.samplerate, bytespersec) ;

						/* fmt : blockalign, bitwidth, extrabytes, samplesperblock. */
						psf_binheader_writef (psf, "e2222", blockalign, 0, 2, samplesperblock) ;
						} ;
	
					add_fact_chunk = SF_TRUE ;
					break ;

		default : 	return SFE_UNIMPLEMENTED ;
		} ;

	if (add_fact_chunk)
		psf_binheader_writef (psf, "em44", fact_MARKER, 4, psf->sf.samples) ;

	if (psf->has_peak && psf->peak_loc == SF_PEAK_START)
	{	psf_binheader_writef (psf, "em4", PEAK_MARKER, 
			sizeof (psf->peak) - sizeof (psf->peak.peak) + psf->sf.channels * sizeof (PEAK_POS)) ;
		psf_binheader_writef (psf, "e44", 1, time (NULL)) ;
		for (k = 0 ; k < psf->sf.channels ; k++)
			psf_binheader_writef (psf, "ef4", psf->peak.peak[k].value, psf->peak.peak[k].position) ;
		} ;

	psf_binheader_writef (psf, "etm8", data_MARKER, psf->datalength) ;
	psf_fwrite (psf->header, psf->headindex, 1, psf->filedes) ;

	psf->dataoffset = psf->headindex ;

	return 0 ;
} /* wav_write_header */

static int
wav_write_tailer (SF_PRIVATE *psf)
{	int		k ;

	/* Reset the current header buffer length to zero. */
	psf->header [0] = 0 ;
	psf->headindex = 0 ;
	psf_fseek (psf->filedes, 0, SEEK_END) ;

	if (psf->has_peak && psf->peak_loc == SF_PEAK_END)
	{	psf_binheader_writef (psf, "em4", PEAK_MARKER, 
			sizeof (psf->peak) - sizeof (psf->peak.peak) + psf->sf.channels * sizeof (PEAK_POS)) ;
		psf_binheader_writef (psf, "e44", 1, time (NULL)) ;
		for (k = 0 ; k < psf->sf.channels ; k++)
			psf_binheader_writef (psf, "ef4", psf->peak.peak[k].value, psf->peak.peak[k].position) ; /* XXXXX */
		} ;

	if (psf->headindex > 0)
		psf_fwrite (psf->header, psf->headindex, 1, psf->filedes) ;

	return 0 ;
} /* wav_write_tailer */

static int	
wav_close	(SF_PRIVATE  *psf)
{	
	if (psf->mode == SFM_WRITE || psf->mode == SFM_RDWR)
	{	/*  Now we know for certain the length of the file we can
		 *  re-write correct values for the RIFF and data chunks.
		 */

		psf_fseek (psf->filedes, 0, SEEK_END) ;
		psf->dataend = psf_ftell (psf->filedes) ;
		wav_write_tailer (psf) ;

		psf_fseek (psf->filedes, 0, SEEK_END) ;
		psf->filelength = psf_ftell (psf->filedes) ;
		psf_fseek (psf->filedes, 0, SEEK_SET) ;
		
		psf->datalength = psf->filelength - psf->dataoffset - (psf->filelength - psf->dataend) ;
 		psf->sf.samples = psf->datalength / (psf->bytewidth * psf->sf.channels) ;

		wav_write_header (psf) ;
		} ;

	return 0 ;
} /* wav_close */

static int
wav_subchunk_parse (SF_PRIVATE *psf, int chunk)
{	sf_count_t	current_pos ;
	int 		dword, bytesread, length ;

	current_pos = psf_fseek (psf->filedes, 0, SEEK_CUR) ;

	bytesread = psf_binheader_readf (psf, "e4", &length);
	
	if (current_pos + length > psf->filelength)
	{	psf_log_printf (psf, "%D : %d (should be %d)\n", chunk, length, (int) (psf->filelength - current_pos)) ;
		length = psf->filelength - current_pos ;
		}
	else
		psf_log_printf (psf, "%D : %d\n", chunk, length) ;
	

	while (bytesread < length)
	{	bytesread += psf_binheader_readf (psf, "m", &chunk);

		switch (chunk)
		{	case adtl_MARKER :
			case INFO_MARKER :
					/* These markers don't contain anything. */
					psf_log_printf (psf, "  %D\n", chunk) ;
					break ;
					
			case data_MARKER:
					psf_log_printf (psf, "  %D inside a LIST block??? Backing out.\n", chunk) ;
					/* Jump back four bytes and return to caller. */
					psf_binheader_readf (psf, "j", -4);
					return 0 ;

			case IART_MARKER :
			case ICMT_MARKER : 
			case ICOP_MARKER :
			case ICRD_MARKER :
			case IENG_MARKER :
			
			case INAM_MARKER :
			case IPRD_MARKER :
			case ISBJ_MARKER :
			case ISFT_MARKER :
			case ISRC_MARKER :
					bytesread += psf_binheader_readf (psf, "e4", &dword);
					dword += (dword & 1) ;
					if (dword > SIGNED_SIZEOF (psf->buffer))
					{	psf_log_printf (psf, "  *** %D : %d (too big)\n", chunk, dword) ;
						return SFE_INTERNAL ;
						} ;
					bytesread += psf_binheader_readf (psf, "b", psf->buffer, dword) ;
					psf->buffer [dword - 1] = 0 ;
					psf_log_printf (psf, "    %D : %s\n", chunk, psf->buffer) ;
					break ;

			case labl_MARKER :
			case ltxt_MARKER :
			case note_MARKER :
					bytesread += psf_binheader_readf (psf, "e4", &dword);
					dword += (dword & 1) ;
					psf_binheader_readf (psf, "j", dword) ;
					bytesread += dword ;
					psf_log_printf (psf, "    %D : %d\n", chunk, dword) ;
					break ;

			default : 
					bytesread += psf_binheader_readf (psf, "e4", &dword);
					dword += (dword & 1) ;
					bytesread += psf_binheader_readf (psf, "j", dword) ;
					psf_log_printf (psf, "    *** %D : %d\n", chunk, dword) ;
					if (dword > length)
						return 0 ;
					break ;
			} ;
		if (psf->logindex >= sizeof (psf->logbuffer) - 2)
			return SFE_LOG_OVERRUN ;
		} ;

	return 0 ;
} /* wav_subchunk_parse */


