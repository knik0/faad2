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
#include	"wav.h"

/*------------------------------------------------------------------------------
** List of known WAV format tags
*/

enum
{	
	WAVE_FORMAT_UNKNOWN				= 0x0000,		/* Microsoft Corporation */
	WAVE_FORMAT_PCM     		    = 0x0001, 		/* Microsoft PCM format */

	WAVE_FORMAT_MS_ADPCM			= 0x0002,		/* Microsoft ADPCM */
	WAVE_FORMAT_IEEE_FLOAT			= 0x0003,		/* Micrososft 32 bit float format */
	
	WAVE_FORMAT_IBM_CVSD			= 0x0005,		/* IBM Corporation */
	WAVE_FORMAT_ALAW				= 0x0006,		/* Microsoft Corporation */
	WAVE_FORMAT_MULAW				= 0x0007,		/* Microsoft Corporation */
	WAVE_FORMAT_OKI_ADPCM			= 0x0010,		/* OKI */
	WAVE_FORMAT_IMA_ADPCM			= 0x0011,		/* Intel Corporation */
	WAVE_FORMAT_MEDIASPACE_ADPCM	= 0x0012,		/* Videologic */
	WAVE_FORMAT_SIERRA_ADPCM		= 0x0013,		/* Sierra Semiconductor Corp */
	WAVE_FORMAT_G723_ADPCM			= 0x0014,		/* Antex Electronics Corporation */
	WAVE_FORMAT_DIGISTD				= 0x0015,		/* DSP Solutions, Inc. */
	WAVE_FORMAT_DIGIFIX				= 0x0016,		/* DSP Solutions, Inc. */
	WAVE_FORMAT_DIALOGIC_OKI_ADPCM	= 0x0017,		/*  Dialogic Corporation  */
	WAVE_FORMAT_MEDIAVISION_ADPCM	= 0x0018,		/*  Media Vision, Inc. */

	WAVE_FORMAT_YAMAHA_ADPCM		= 0x0020,		/* Yamaha Corporation of America */
	WAVE_FORMAT_SONARC				= 0x0021,		/* Speech Compression */
	WAVE_FORMAT_DSPGROUP_TRUESPEECH = 0x0022,		/* DSP Group, Inc */
	WAVE_FORMAT_ECHOSC1				= 0x0023,		/* Echo Speech Corporation */
	WAVE_FORMAT_AUDIOFILE_AF18  	= 0x0024,		/* Audiofile, Inc. */
	WAVE_FORMAT_APTX				= 0x0025,		/* Audio Processing Technology */
	WAVE_FORMAT_AUDIOFILE_AF10  	= 0x0026,		/* Audiofile, Inc. */

	WAVE_FORMAT_DOLBY_AC2			= 0x0030,		/* Dolby Laboratories */
	WAVE_FORMAT_GSM610				= 0x0031,		/* Microsoft Corporation */
	WAVE_FORMAT_MSNAUDIO			= 0x0032,		/* Microsoft Corporation */
	WAVE_FORMAT_ANTEX_ADPCME		= 0x0033, 		/* Antex Electronics Corporation */
	WAVE_FORMAT_CONTROL_RES_VQLPC	= 0x0034,		/* Control Resources Limited */
	WAVE_FORMAT_DIGIREAL			= 0x0035,		/* DSP Solutions, Inc. */
	WAVE_FORMAT_DIGIADPCM			= 0x0036,		/* DSP Solutions, Inc. */
	WAVE_FORMAT_CONTROL_RES_CR10	= 0x0037,		/* Control Resources Limited */
	WAVE_FORMAT_NMS_VBXADPCM		= 0x0038,		/* Natural MicroSystems */
	WAVE_FORMAT_ROCKWELL_ADPCM		= 0x003B,		/* Rockwell International */
	WAVE_FORMAT_ROCKWELL_DIGITALK	= 0x003C, 		/* Rockwell International */

	WAVE_FORMAT_G721_ADPCM			= 0x0040,		/* Antex Electronics Corporation */
	WAVE_FORMAT_MPEG				= 0x0050,		/* Microsoft Corporation */

	WAVE_FORMAT_MPEGLAYER3			= 0x0055,		/* MPEG 3 Layer 1 */

	IBM_FORMAT_MULAW				= 0x0101,		/* IBM mu-law format */
	IBM_FORMAT_ALAW					= 0x0102,		/* IBM a-law format */
	IBM_FORMAT_ADPCM				= 0x0103,		/* IBM AVC Adaptive Differential PCM format */

	WAVE_FORMAT_CREATIVE_ADPCM		= 0x0200,		/* Creative Labs, Inc */

	WAVE_FORMAT_FM_TOWNS_SND		= 0x0300,		/* Fujitsu Corp. */
	WAVE_FORMAT_OLIGSM				= 0x1000,		/* Ing C. Olivetti & C., S.p.A. */
	WAVE_FORMAT_OLIADPCM			= 0x1001,		/* Ing C. Olivetti & C., S.p.A. */
	WAVE_FORMAT_OLICELP				= 0x1002,		/* Ing C. Olivetti & C., S.p.A. */
	WAVE_FORMAT_OLISBC				= 0x1003,		/* Ing C. Olivetti & C., S.p.A. */
	WAVE_FORMAT_OLIOPR				= 0x1004,		/* Ing C. Olivetti & C., S.p.A. */

	WAVE_FORMAT_EXTENSIBLE			= 0xFFFE
} ;

#define	FACT_CHUNK_SIZE	sizeof (int)

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
#define note_MARKER	(MAKE_MARKER ('n', 'o', 't', 'e')) 
#define smpl_MARKER	(MAKE_MARKER ('s', 'm', 'p', 'l')) 
#define bext_MARKER	(MAKE_MARKER ('b', 'e', 'x', 't')) 
#define MEXT_MARKER	(MAKE_MARKER ('M', 'E', 'X', 'T')) 
#define DISP_MARKER	(MAKE_MARKER ('D', 'I', 'S', 'P')) 
#define acid_MARKER	(MAKE_MARKER ('a', 'c', 'i', 'd')) 
#define PAD_MARKER	(MAKE_MARKER ('P', 'A', 'D', ' ')) 
#define adtl_MARKER	(MAKE_MARKER ('a', 'd', 't', 'l')) 

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



enum {
	HAVE_RIFF	= 0x01,
	HAVE_WAVE	= 0x02,
	HAVE_fmt	= 0x04,
	HAVE_fact	= 0x08,
	HAVE_PEAK	= 0x10,
	HAVE_data	= 0x20
} ;

/*------------------------------------------------------------------------------
 * Private static functions.
 */

static int		wav_close	(SF_PRIVATE  *psf) ;

static int		read_fmt_chunk	(SF_PRIVATE *psf, WAV_FMT *wav_fmt) ;
static int		wav_write_header (SF_PRIVATE *psf) ;
static int		wav_write_tailer (SF_PRIVATE *psf) ;

static int 		wav_subchunk_parse	(SF_PRIVATE *psf, int chunk) ;

static const 	char* wav_format_str (int k) ;

/*------------------------------------------------------------------------------
** Public functions.
*/

int
wav_open_read	(SF_PRIVATE *psf)
{	WAV_FMT			wav_fmt ;
	FACT_CHUNK		fact_chunk ;
	unsigned int	dword, marker, RIFFsize ;
	int				parsestage = 0, error, format = 0 ;
	char			*cptr ;	
	
	/* Set position to start of file to begin reading header. */
	psf_binheader_readf (psf, "p", 0) ;	
		
	while (1)
	{	psf_binheader_readf (psf, "m", &marker) ;
		switch (marker)
		{	case RIFF_MARKER :
					if (parsestage)
						return SFE_WAV_NO_RIFF ;

					psf_binheader_readf (psf, "l", &RIFFsize) ;
					
					if (psf->filelength  < RIFFsize + 2 * sizeof (dword))
					{	dword = psf->filelength - 2 * sizeof (dword);
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
					if ((error = read_fmt_chunk (psf, &wav_fmt)))
						return error ;
						
					format     = wav_fmt.format ;
					parsestage |= HAVE_fmt ;
					break ;
					
			case data_MARKER :
					if ((parsestage & (HAVE_RIFF | HAVE_WAVE | HAVE_fmt)) != (HAVE_RIFF | HAVE_WAVE | HAVE_fmt))
						return SFE_WAV_NO_DATA ;
					
					psf_binheader_readf (psf, "l", &(psf->datalength)) ;

					psf->dataoffset = ftell (psf->file) ;
					
					if (psf->filelength < psf->dataoffset + psf->datalength)
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
					fseek (psf->file, psf->datalength, SEEK_CUR) ;

					dword = ftell (psf->file) ;
					if (dword != (long) (psf->dataoffset + psf->datalength))
						psf_log_printf (psf, "*** fseek past end error ***\n", dword, psf->dataoffset + psf->datalength) ;
					break ;

			case fact_MARKER :
					if ((parsestage & (HAVE_RIFF | HAVE_WAVE | HAVE_fmt)) != (HAVE_RIFF | HAVE_WAVE | HAVE_fmt))
						return SFE_WAV_BAD_FACT ;

					psf_binheader_readf (psf, "ll", &dword, &(fact_chunk.samples)) ;
					
					if (dword > sizeof (fact_chunk))
						psf_binheader_readf (psf, "j", (int) (dword - sizeof (fact_chunk))) ;

					psf_log_printf (psf, "%D : %d\n", marker, dword) ;
					psf_log_printf (psf, "  samples : %d\n", fact_chunk.samples) ;
					parsestage |= HAVE_fact ;
					break ;

			case PEAK_MARKER :
					if ((parsestage & (HAVE_RIFF | HAVE_WAVE | HAVE_fmt)) != (HAVE_RIFF | HAVE_WAVE | HAVE_fmt))
						return SFE_WAV_PEAK_B4_FMT ;

					psf_binheader_readf (psf, "l", &dword) ;
					
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
					
					psf_binheader_readf (psf, "ll", &(psf->peak.version), &(psf->peak.timestamp)) ;

					if (psf->peak.version != 1)
						psf_log_printf (psf, "  version    : %d *** (should be version 1)\n", psf->peak.version) ;
					else
						psf_log_printf (psf, "  version    : %d\n", psf->peak.version) ;
						
					psf_log_printf (psf, "  time stamp : %d\n", psf->peak.timestamp) ;
					psf_log_printf (psf, "    Ch   Position       Value\n") ;

					cptr = (char *) psf->buffer ;
					for (dword = 0 ; dword < psf->sf.channels ; dword++)
					{	psf_binheader_readf (psf, "fl", &(psf->peak.peak[dword].value), 
														&(psf->peak.peak[dword].position)) ;
					
						snprintf (cptr, sizeof (psf->buffer), "    %2d   %-12d   %g\n", 
								dword, psf->peak.peak[dword].position, psf->peak.peak[dword].value) ;
						cptr [sizeof (psf->buffer) - 1] = 0 ;
						psf_log_printf (psf, cptr) ;
						};

					psf->has_peak = SF_TRUE ;
					break ;

			case INFO_MARKER :
			case LIST_MARKER :
					psf_log_printf (psf, "%D\n", marker) ;
					if ((error = wav_subchunk_parse (psf, marker)))
						return error ;
					break ;
			
			case bext_MARKER :
			case cue_MARKER :
			case DISP_MARKER :
			case MEXT_MARKER :
					psf_binheader_readf (psf, "l", &dword);
					psf_log_printf (psf, "%D : %d\n", marker, dword) ;
					dword += (dword & 1) ;
					psf_binheader_readf (psf, "j", dword) ;
					break ;

			case smpl_MARKER :
			case acid_MARKER :
			case PAD_MARKER :
					psf_binheader_readf (psf, "l", &dword);
					psf_log_printf (psf, " *** %D : %d\n", marker, dword) ;
					dword += (dword & 1) ;
					psf_binheader_readf (psf, "j", dword) ;
					break ;


			default : 
					if (isprint ((marker >> 24) & 0xFF) && isprint ((marker >> 16) & 0xFF)
						&& isprint ((marker >> 8) & 0xFF) && isprint (marker & 0xFF))
					{	psf_binheader_readf (psf, "l", &dword);
						psf_log_printf (psf, "*** %D : %d (unknown marker)\n", marker, dword) ;

						psf_binheader_readf (psf, "j", dword);
						break ;
						} ;
					if (ftell (psf->file) & 0x03)
					{	psf_log_printf (psf, "  Unknown chunk marker at position %d. Resynching.\n", dword - 4) ;
						psf_binheader_readf (psf, "j", -3) ;
						break ;
						} ;
					psf_log_printf (psf, "*** Unknown chunk marker : %X. Exiting parser.\n", marker) ;
					break ;
			} ;	/* switch (dword) */

		if (! psf->sf.seekable && (parsestage & HAVE_data))
			break ;

		if (ferror (psf->file))
		{	psf_log_printf (psf, "*** Error on file handle. ***\n", marker) ;
			clearerr (psf->file) ;
			break ;
			} ;

		if (ftell (psf->file) >= (int) (psf->filelength - (2 * sizeof (dword))))
			break ;
		} ; /* while (1) */
		
	if (! psf->dataoffset)
		return SFE_WAV_NO_DATA ;

	psf->current     = 0 ;
	psf->endian      = SF_ENDIAN_LITTLE ;		/* All WAV files are little endian. */
	psf->sf.sections = 1 ;
	
	fseek (psf->file, psf->dataoffset, SEEK_SET) ;
	
	psf->close = (func_close) wav_close ;

	if (psf->blockwidth)
	{	if (psf->filelength - psf->dataoffset < psf->datalength)
			psf->sf.samples = (psf->filelength - psf->dataoffset) / psf->blockwidth ;
		else
			psf->sf.samples = psf->datalength / psf->blockwidth ;
		} ;

	switch (format)
	{	case 	WAVE_FORMAT_PCM :
		case	WAVE_FORMAT_EXTENSIBLE :
					psf->sf.format = (SF_FORMAT_WAV | SF_FORMAT_PCM) ;
					if (psf->bytewidth == 1)
						psf->chars = SF_CHARS_UNSIGNED ;
	
					if ((error = pcm_read_init (psf)))
						return error ;
					return 0 ;
					
		case	WAVE_FORMAT_IEEE_FLOAT :
					psf->sf.format   = (SF_FORMAT_WAV | SF_FORMAT_FLOAT) ;
					float32_read_init (psf) ;
					return 0 ;
		
		default :	return SFE_UNIMPLEMENTED ;
		} ;

	return 0 ;
} /* wav_open_read */

/*------------------------------------------------------------------------------
 */

int
wav_open_write	(SF_PRIVATE *psf)
{	unsigned int	subformat ;
	int				error ;
	
	if ((psf->sf.format & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV)
		return	SFE_BAD_OPEN_FORMAT ;
	
	psf->endian      = SF_ENDIAN_LITTLE ;		/* All WAV files are little endian. */
	psf->sf.seekable = SF_TRUE ;
	psf->error       = 0 ;

	subformat = psf->sf.format & SF_FORMAT_SUBMASK ;
	if (subformat == SF_FORMAT_ULAW || subformat == SF_FORMAT_ALAW)
		psf->bytewidth = 1 ;
	else
		psf->bytewidth = BITWIDTH2BYTES (psf->sf.pcmbitwidth) ;

	psf->blockwidth  = psf->bytewidth * psf->sf.channels ;

	/* Set sample count artifically high and fix later. */ 
	psf->sf.samples = 0x7FFFFFFF / psf->blockwidth ; 
	psf->datalength = psf->blockwidth * psf->sf.samples ;
	psf->filelength = 0x7FFFFFFF ;

	/* Set standard wav_close and write_header now, may be overridden in wav_write_header. */
	psf->close        = (func_close)  wav_close ;
	psf->write_header = (func_wr_hdr) wav_write_header ;

	if ((error = wav_write_header (psf)))
		return error ;

	return 0 ;
} /* wav_open_write */

/*------------------------------------------------------------------------------
 */

static int	
wav_close	(SF_PRIVATE  *psf)
{	
	if (psf->mode == SF_MODE_WRITE)
	{	/*  Now we know for certain the length of the file we can
		 *  re-write correct values for the RIFF and data chunks.
		 */

		fseek (psf->file, 0, SEEK_END) ;
		psf->tailstart = ftell (psf->file) ;
		wav_write_tailer (psf) ;

		fseek (psf->file, 0, SEEK_END) ;
		psf->filelength = ftell (psf->file) ;
		fseek (psf->file, 0, SEEK_SET) ;
		
		psf->datalength = psf->filelength - psf->dataoffset - (psf->filelength - psf->tailstart) ;
 		psf->sf.samples = psf->datalength / (psf->bytewidth * psf->sf.channels) ;

		wav_write_header (psf) ;
		} ;

	if (psf->fdata)
		free (psf->fdata) ;
	psf->fdata = NULL ;

	return 0 ;
} /* wav_close */

/*=========================================================================
** Private functions.
*/

static int
read_fmt_chunk (SF_PRIVATE *psf, WAV_FMT *wav_fmt)
{	unsigned int	bytesread, k, structsize, bytespersec = 0  ;
	
	memset (wav_fmt, 0, sizeof (WAV_FMT)) ;

	psf_binheader_readf (psf, "l", &structsize) ;
	
	psf_log_printf (psf, "fmt  : %d\n", structsize) ;
	
	if (structsize < 16)
		return SFE_WAV_FMT_SHORT ;
	if (structsize > sizeof (WAV_FMT))
		return SFE_WAV_FMT_TOO_BIG ;

	/* Read the minimal WAV file header here. */	
	bytesread =
	psf_binheader_readf (psf, "wwllww", &(wav_fmt->format), &(wav_fmt->min.channels),
			&(wav_fmt->min.samplerate), &(wav_fmt->min.bytespersec), 
			&(wav_fmt->min.blockalign), &(wav_fmt->min.bitwidth))  ;

	psf_log_printf (psf, "  Format        : 0x%X => %s\n", wav_fmt->format, wav_format_str (wav_fmt->format)) ;
	psf_log_printf (psf, "  Channels      : %d\n", wav_fmt->min.channels) ;
	psf_log_printf (psf, "  Sample Rate   : %d\n", wav_fmt->min.samplerate) ;
	psf_log_printf (psf, "  Block Align   : %d\n", wav_fmt->min.blockalign) ;
	
	if (wav_fmt->format == WAVE_FORMAT_GSM610 && wav_fmt->min.bitwidth != 0)
		psf_log_printf (psf, "  Bit Width     : %d (should be 0)\n", wav_fmt->min.bitwidth) ;
	else
		psf_log_printf (psf, "  Bit Width     : %d\n", wav_fmt->min.bitwidth) ;
	
	psf->sf.samplerate		= wav_fmt->min.samplerate ;
	psf->sf.samples 		= 0 ;					/* Correct this when reading data chunk. */
	psf->sf.channels		= wav_fmt->min.channels ;
	
	switch (wav_fmt->format)
	{	case WAVE_FORMAT_PCM :
		case WAVE_FORMAT_IEEE_FLOAT :
				bytespersec = wav_fmt->min.samplerate * wav_fmt->min.blockalign ;
				if (wav_fmt->min.bytespersec != bytespersec)
					psf_log_printf (psf, "  Bytes/sec     : %d (should be %d)\n", wav_fmt->min.bytespersec, bytespersec) ;
				else
					psf_log_printf (psf, "  Bytes/sec     : %d\n", wav_fmt->min.bytespersec) ;
		
				psf->sf.pcmbitwidth	= wav_fmt->min.bitwidth ;
				psf->bytewidth      = BITWIDTH2BYTES (wav_fmt->min.bitwidth) ;
				break ;

		case WAVE_FORMAT_EXTENSIBLE :
				if (wav_fmt->ext.bytespersec / wav_fmt->ext.blockalign != wav_fmt->ext.samplerate)
					psf_log_printf (psf, "  Bytes/sec     : %d (should be %d)\n", wav_fmt->ext.bytespersec, wav_fmt->ext.samplerate * wav_fmt->ext.blockalign) ;
				else
					psf_log_printf (psf, "  Bytes/sec     : %d\n", wav_fmt->ext.bytespersec) ;

				bytesread += 
				psf_binheader_readf (psf, "wwl", &(wav_fmt->ext.extrabytes), &(wav_fmt->ext.validbits),
						&(wav_fmt->ext.channelmask)) ;

				psf_log_printf (psf, "  Valid Bits    : %d\n", wav_fmt->ext.validbits) ;
				psf_log_printf (psf, "  Channel Mask  : 0x%X\n", wav_fmt->ext.channelmask) ;

				bytesread += 
				psf_binheader_readf (psf, "lww", &(wav_fmt->ext.esf.esf_field1), &(wav_fmt->ext.esf.esf_field2),
						&(wav_fmt->ext.esf.esf_field3)) ;

				psf_log_printf (psf, "  Subformat\n") ;
				psf_log_printf (psf, "    esf_field1 : 0x%X\n", wav_fmt->ext.esf.esf_field1) ;
				psf_log_printf (psf, "    esf_field2 : 0x%X\n", wav_fmt->ext.esf.esf_field2) ;
				psf_log_printf (psf, "    esf_field3 : 0x%X\n", wav_fmt->ext.esf.esf_field3) ;
				psf_log_printf (psf, "    esf_field4 : ") ;
				for (k = 0 ; k < 8 ; k++)
				{	bytesread += psf_binheader_readf (psf, "b", &(wav_fmt->ext.esf.esf_field4 [k])) ;
					psf_log_printf (psf, "0x%X ", wav_fmt->ext.esf.esf_field4 [k] & 0xFF) ;
					} ;
				psf_log_printf (psf, "\n") ;
				psf->sf.pcmbitwidth = wav_fmt->ext.bitwidth ;
				psf->bytewidth      = BITWIDTH2BYTES (wav_fmt->ext.bitwidth) ;
				break ;

		default : break ;
		} ;

	if (bytesread > structsize)	
	{	psf_log_printf (psf, "*** read_fmt_chunk (bytesread > structsize)\n") ;
		return SFE_WAV_FMT_SHORT ;
		}
	else
		fread (psf->buffer, 1, structsize - bytesread, psf->file) ;

	psf->blockwidth = wav_fmt->min.channels * psf->bytewidth ;

	return 0 ;
} /* read_fmt_chunk */

static int 
wav_write_header (SF_PRIVATE *psf)
{	unsigned int 	fmt_size ;
	int 			k, error, subformat ;
	
	/* Reset the current header length to zero. */
	psf->header [0] = 0 ;
	psf->headindex = 0 ;
	fseek (psf->file, 0, SEEK_SET) ;

	/* RIFF marker, length, WAVE and 'fmt ' markers. */		
	psf_binheader_writef (psf, "mlmm", RIFF_MARKER, psf->filelength - 8, WAVE_MARKER, fmt_MARKER) ;

	subformat = psf->sf.format & SF_FORMAT_SUBMASK ;

	switch (subformat)
	{	case	SF_FORMAT_PCM : 
					psf->sf.format = (SF_FORMAT_WAV | SF_FORMAT_PCM) ;

					if (psf->bytewidth == 1)
						psf->chars = SF_CHARS_UNSIGNED ;
	
					if ((error = pcm_write_init (psf)))
						return error ;
		
					fmt_size = 2 + 2 + 4 + 4 + 2 + 2 ;

					/* fmt : format, channels, samplerate */
					psf_binheader_writef (psf, "lwwl", fmt_size, WAVE_FORMAT_PCM, psf->sf.channels, psf->sf.samplerate) ;
					/*  fmt : bytespersec */
					psf_binheader_writef (psf, "l", psf->sf.samplerate * psf->bytewidth * psf->sf.channels) ;
					/*  fmt : blockalign, bitwidth */
					psf_binheader_writef (psf, "ww", psf->bytewidth * psf->sf.channels, psf->sf.pcmbitwidth) ;
					break ;

		case	SF_FORMAT_FLOAT : 
					psf->sf.format = (SF_FORMAT_WAV | SF_FORMAT_FLOAT) ;

					/* Add the peak chunk to floating point files. */					
					psf->has_peak = SF_TRUE ;
					psf->peak_loc = SF_PEAK_START ;
					float32_write_init (psf) ;
					
					fmt_size = 2 + 2 + 4 + 4 + 2 + 2 ;

					/* fmt : format, channels, samplerate */
					psf_binheader_writef (psf, "lwwl", fmt_size, WAVE_FORMAT_IEEE_FLOAT, psf->sf.channels, psf->sf.samplerate) ;
					/*  fmt : bytespersec */
					psf_binheader_writef (psf, "l", psf->sf.samplerate * psf->bytewidth * psf->sf.channels) ;
					/*  fmt : blockalign, bitwidth */
					psf_binheader_writef (psf, "ww", psf->bytewidth * psf->sf.channels, psf->sf.pcmbitwidth) ;

					/* Write 'fact' chunk. */
					psf_binheader_writef (psf, "mll", fact_MARKER, FACT_CHUNK_SIZE, psf->sf.samples) ;
					break ;

		default : 	return SFE_UNIMPLEMENTED ;
		} ;

	if (psf->has_peak && psf->peak_loc == SF_PEAK_START)
	{	psf_binheader_writef (psf, "ml", PEAK_MARKER, 
			sizeof (psf->peak) - sizeof (psf->peak.peak) + psf->sf.channels * sizeof (PEAK_POS)) ;
		psf_binheader_writef (psf, "ll", 1, time (NULL)) ;
		for (k = 0 ; k < psf->sf.channels ; k++)
			psf_binheader_writef (psf, "fl", psf->peak.peak[k].value, psf->peak.peak[k].position) ;
		} ;

	psf_binheader_writef (psf, "ml", data_MARKER, psf->datalength) ;
	fwrite (psf->header, psf->headindex, 1, psf->file) ;

	psf->dataoffset = psf->headindex ;

	return 0 ;
} /* wav_write_header */

static int 
wav_write_tailer (SF_PRIVATE *psf)
{	int		k ;

	/* Reset the current header buffer length to zero. */
	psf->header [0] = 0 ;
	psf->headindex = 0 ;
	fseek (psf->file, 0, SEEK_END) ;

	if (psf->has_peak && psf->peak_loc == SF_PEAK_END)
	{	psf_binheader_writef (psf, "ml", PEAK_MARKER, 
			sizeof (psf->peak) - sizeof (psf->peak.peak) + psf->sf.channels * sizeof (PEAK_POS)) ;
		psf_binheader_writef (psf, "ll", 1, time (NULL)) ;
		for (k = 0 ; k < psf->sf.channels ; k++)
			psf_binheader_writef (psf, "fl", psf->peak.peak[k].value, psf->peak.peak[k].position) ; /* XXXXX */
		} ;

	if (psf->headindex > 0)
		fwrite (psf->header, psf->headindex, 1, psf->file) ;

	return 0 ;
} /* wav_write_tailer */

static int
wav_subchunk_parse (SF_PRIVATE *psf, int chunk)
{	unsigned int dword, bytesread, length ;

	bytesread = psf_binheader_readf (psf, "l", &length);

	while (bytesread < length)
	{	bytesread += psf_binheader_readf (psf, "m", &chunk);

		switch (chunk)
		{	case adtl_MARKER :
			case INFO_MARKER :
					/* These markers don't contain anything. */
					psf_log_printf (psf, "  %D\n", chunk) ;
					break ;


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
					bytesread += psf_binheader_readf (psf, "l", &dword);
					dword += (dword & 1) ;
					if (dword > sizeof (psf->buffer))
					{	psf_log_printf (psf, "  *** %D : %d (too big)\n", chunk, dword) ;
						return SFE_INTERNAL ;
						} ;
					bytesread += psf_binheader_readf (psf, "B", psf->buffer, dword) ;
					psf->buffer [dword - 1] = 0 ;
					psf_log_printf (psf, "    %D : %s\n", chunk, psf->buffer) ;
					break ;

			case labl_MARKER :
			case note_MARKER :
					bytesread += psf_binheader_readf (psf, "l", &dword);
					dword += (dword & 1) ;
					psf_binheader_readf (psf, "j", dword) ;
					bytesread += dword ;
					psf_log_printf (psf, "    %D : %d\n", chunk, dword) ;
					break ;

			default : 
					bytesread += psf_binheader_readf (psf, "l", &dword);
					dword += (dword & 1) ;
					bytesread += psf_binheader_readf (psf, "j", dword) ;
					psf_log_printf (psf, "    *** %D : %d\n", chunk, dword) ;
					break ;
			} ;
		} ;

	return 0 ;
} /* wav_subchunk_parse */

static char const* 
wav_format_str (int k)
{	switch (k)
	{	case WAVE_FORMAT_UNKNOWN :
			return "WAVE_FORMAT_UNKNOWN" ;
		case WAVE_FORMAT_PCM          :
			return "WAVE_FORMAT_PCM         " ;
		case WAVE_FORMAT_MS_ADPCM :
			return "WAVE_FORMAT_MS_ADPCM" ;
		case WAVE_FORMAT_IEEE_FLOAT :
			return "WAVE_FORMAT_IEEE_FLOAT" ;
		case WAVE_FORMAT_IBM_CVSD :
			return "WAVE_FORMAT_IBM_CVSD" ;
		case WAVE_FORMAT_ALAW :
			return "WAVE_FORMAT_ALAW" ;
		case WAVE_FORMAT_MULAW :
			return "WAVE_FORMAT_MULAW" ;
		case WAVE_FORMAT_OKI_ADPCM :
			return "WAVE_FORMAT_OKI_ADPCM" ;
		case WAVE_FORMAT_IMA_ADPCM :
			return "WAVE_FORMAT_IMA_ADPCM" ;
		case WAVE_FORMAT_MEDIASPACE_ADPCM :
			return "WAVE_FORMAT_MEDIASPACE_ADPCM" ;
		case WAVE_FORMAT_SIERRA_ADPCM :
			return "WAVE_FORMAT_SIERRA_ADPCM" ;
		case WAVE_FORMAT_G723_ADPCM :
			return "WAVE_FORMAT_G723_ADPCM" ;
		case WAVE_FORMAT_DIGISTD :
			return "WAVE_FORMAT_DIGISTD" ;
		case WAVE_FORMAT_DIGIFIX :
			return "WAVE_FORMAT_DIGIFIX" ;
		case WAVE_FORMAT_DIALOGIC_OKI_ADPCM :
			return "WAVE_FORMAT_DIALOGIC_OKI_ADPCM" ;
		case WAVE_FORMAT_MEDIAVISION_ADPCM :
			return "WAVE_FORMAT_MEDIAVISION_ADPCM" ;
		case WAVE_FORMAT_YAMAHA_ADPCM :
			return "WAVE_FORMAT_YAMAHA_ADPCM" ;
		case WAVE_FORMAT_SONARC :
			return "WAVE_FORMAT_SONARC" ;
		case WAVE_FORMAT_DSPGROUP_TRUESPEECH  :
			return "WAVE_FORMAT_DSPGROUP_TRUESPEECH " ;
		case WAVE_FORMAT_ECHOSC1 :
			return "WAVE_FORMAT_ECHOSC1" ;
		case WAVE_FORMAT_AUDIOFILE_AF18   :
			return "WAVE_FORMAT_AUDIOFILE_AF18  " ;
		case WAVE_FORMAT_APTX :
			return "WAVE_FORMAT_APTX" ;
		case WAVE_FORMAT_AUDIOFILE_AF10   :
			return "WAVE_FORMAT_AUDIOFILE_AF10  " ;
		case WAVE_FORMAT_DOLBY_AC2 :
			return "WAVE_FORMAT_DOLBY_AC2" ;
		case WAVE_FORMAT_GSM610 :
			return "WAVE_FORMAT_GSM610" ;
		case WAVE_FORMAT_MSNAUDIO :
			return "WAVE_FORMAT_MSNAUDIO" ;
		case WAVE_FORMAT_ANTEX_ADPCME :
			return "WAVE_FORMAT_ANTEX_ADPCME" ;
		case WAVE_FORMAT_CONTROL_RES_VQLPC :
			return "WAVE_FORMAT_CONTROL_RES_VQLPC" ;
		case WAVE_FORMAT_DIGIREAL :
			return "WAVE_FORMAT_DIGIREAL" ;
		case WAVE_FORMAT_DIGIADPCM :
			return "WAVE_FORMAT_DIGIADPCM" ;
		case WAVE_FORMAT_CONTROL_RES_CR10 :
			return "WAVE_FORMAT_CONTROL_RES_CR10" ;
		case WAVE_FORMAT_NMS_VBXADPCM :
			return "WAVE_FORMAT_NMS_VBXADPCM" ;
		case WAVE_FORMAT_ROCKWELL_ADPCM :
			return "WAVE_FORMAT_ROCKWELL_ADPCM" ;
		case WAVE_FORMAT_ROCKWELL_DIGITALK :
			return "WAVE_FORMAT_ROCKWELL_DIGITALK" ;
		case WAVE_FORMAT_G721_ADPCM :
			return "WAVE_FORMAT_G721_ADPCM" ;
		case WAVE_FORMAT_MPEG :
			return "WAVE_FORMAT_MPEG" ;
		case WAVE_FORMAT_MPEGLAYER3 :
			return "WAVE_FORMAT_MPEGLAYER3" ;
		case IBM_FORMAT_MULAW :
			return "IBM_FORMAT_MULAW" ;
		case IBM_FORMAT_ALAW :
			return "IBM_FORMAT_ALAW" ;
		case IBM_FORMAT_ADPCM :
			return "IBM_FORMAT_ADPCM" ;
		case WAVE_FORMAT_CREATIVE_ADPCM :
			return "WAVE_FORMAT_CREATIVE_ADPCM" ;
		case WAVE_FORMAT_FM_TOWNS_SND :
			return "WAVE_FORMAT_FM_TOWNS_SND" ;
		case WAVE_FORMAT_OLIGSM :
			return "WAVE_FORMAT_OLIGSM" ;
		case WAVE_FORMAT_OLIADPCM :
			return "WAVE_FORMAT_OLIADPCM" ;
		case WAVE_FORMAT_OLICELP :
			return "WAVE_FORMAT_OLICELP" ;
		case WAVE_FORMAT_OLISBC :
			return "WAVE_FORMAT_OLISBC" ;
		case WAVE_FORMAT_OLIOPR :
			return "WAVE_FORMAT_OLIOPR" ;
		case WAVE_FORMAT_EXTENSIBLE :
			return "WAVE_FORMAT_EXTENSIBLE" ;
		break ;
		} ;
	return "Unknown format" ;
} /* wav_format_str */
