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
#include	<math.h>

#include	"sndfile.h"
#include	"config.h"
#include	"common.h"

static SF_FORMAT_INFO simple_formats [] =
{
	{	SF_FORMAT_AIFF | SF_FORMAT_PCM_16,
		"AIFF (Apple/SGI 16 bit PCM)", "aiff" 
		},
	
	{	SF_FORMAT_AIFF | SF_FORMAT_FLOAT,
		"AIFF (Apple/SGI 32 bit float)", "aifc" 
		},

	{	SF_FORMAT_AIFF | SF_FORMAT_PCM_S8,
		"AIFF (Apple/SGI 8 bit PCM)", "aiff"
		},

	{	SF_FORMAT_AU | SF_FORMAT_PCM_16,
		"AU (Sun/Next 16 bit PCM)", "au"
		},
		
	{	SF_FORMAT_AU | SF_FORMAT_ULAW,
		"AU (Sun/Next 8-bit u-law)", "au"
		},
		
	{	SF_FORMAT_WAV | SF_FORMAT_PCM_16,
		"WAV (Microsoft 16 bit PCM)", "wav"
		},

	{	SF_FORMAT_WAV | SF_FORMAT_FLOAT,
		"WAV (Microsoft 32 bit float)", "wav"
		},

	{	SF_FORMAT_WAV | SF_FORMAT_IMA_ADPCM,
		"WAV (Microsoft 4 bit IMA ADPCM)", "wav"
		},
		
	{	SF_FORMAT_WAV | SF_FORMAT_MS_ADPCM,
		"WAV (Microsoft 4 bit MS ADPCM)", "wav"
		},
	
	{	SF_FORMAT_WAV | SF_FORMAT_PCM_U8,
		"WAV (Microsoft 8 bit PCM)", "wav"
		},

} ; /* simple_formats */

int		
psf_get_format_simple_count	(void)
{	return (sizeof (simple_formats) / sizeof (SF_FORMAT_INFO)) ;
} /* psf_get_format_simple_count */

int
psf_get_format_simple (SF_FORMAT_INFO *data) 
{	int index ;

	if (data->format < 0 || data->format >= (SIGNED_SIZEOF (simple_formats) / SIGNED_SIZEOF (SF_FORMAT_INFO)))
		return SFE_BAD_CONTROL_CMD ;

	index = data->format ;
	memcpy (data, &(simple_formats [index]), SIGNED_SIZEOF (SF_FORMAT_INFO)) ;
	
	return 0 ;
} /* psf_get_format_simple */

/*============================================================================
** Major format info.
*/

static SF_FORMAT_INFO major_formats [] =
{
	{	SF_FORMAT_AIFF,		"AIFF (Apple/SGI)", 			"aiff" 	},
	{	SF_FORMAT_AU,		"AU (Sun/NeXT)", 				"au"	},
	{	SF_FORMAT_SVX,		"IFF (Amiga IFF/SVX8/SV16)", 	"iff"	},
	{	SF_FORMAT_PAF,		"PAF (Ensoniq PARIS)", 			"paf"	},
	{	SF_FORMAT_RAW,		"RAW header-less",		 		"raw"	},
	/* Not ready for mainstream use yet.
	{	SF_FORMAT_SD2,		"SD2 (Sound Designer II)", 		"sd2"	},
	*/
	{	SF_FORMAT_IRCAM,	"SF (Berkeley/IRCAM/CARL)", 	"sf"	},
	{	SF_FORMAT_VOC,		"VOC (Creative Labs)",			"voc"	},
	{	SF_FORMAT_W64,		"W64 (SoundFoundry WAVE 64)",	"w64"	},
	{	SF_FORMAT_WAV,		"WAV (Microsoft)", 				"wav"	},
	{	SF_FORMAT_NIST,		"WAV (Sphere NIST)", 			"wav"	},

} ; /* major_formats */

int		
psf_get_format_major_count	(void)
{	return (sizeof (major_formats) / sizeof (SF_FORMAT_INFO)) ;
} /* psf_get_format_major_count */

int
psf_get_format_major (SF_FORMAT_INFO *data) 
{	int index ;

	if (data->format < 0 || data->format >= (SIGNED_SIZEOF (major_formats) / SIGNED_SIZEOF (SF_FORMAT_INFO)))
		return SFE_BAD_CONTROL_CMD ;

	index = data->format ;
	memcpy (data, &(major_formats [index]), SIGNED_SIZEOF (SF_FORMAT_INFO)) ;
	
	return 0 ;
} /* psf_get_format_major */

/*============================================================================
** Subtype format info.
*/

static SF_FORMAT_INFO subtype_formats [] =
{
	{	SF_FORMAT_PCM_S8,		"Signed 8 bit PCM",		NULL },
	{	SF_FORMAT_PCM_16,		"Signed 16 bit PCM",	NULL },
	{	SF_FORMAT_PCM_24,		"Signed 24 bit PCM",	NULL },
	{	SF_FORMAT_PCM_32,		"Signed 32 bit PCM",	NULL },

	{	SF_FORMAT_PCM_U8,		"Unsigned 8 bit PCM",	NULL },
	
	{	SF_FORMAT_FLOAT,		"32 bit float",			NULL },
	{	SF_FORMAT_DOUBLE,		"64 bit float",			NULL },
	
	{	SF_FORMAT_ULAW,			"U-Law",				NULL },
	{	SF_FORMAT_ALAW,			"A-Law",				NULL },
	{	SF_FORMAT_IMA_ADPCM,	"IMA ADPCM",			NULL },
	{	SF_FORMAT_MS_ADPCM,		"Microsoft ADPCM",		NULL },

	{	SF_FORMAT_GSM610,		"GSM 6.10",				NULL },

	{	SF_FORMAT_G721_32,		"32kbs G721 ADPCM",		NULL },
	{	SF_FORMAT_G723_24,		"24kbs G723 ADPCM",		NULL },

	{	SF_FORMAT_DWVW_12,		"12 bit DWVW",			NULL },
	{	SF_FORMAT_DWVW_16,		"16 bit DWVW",			NULL },
	{	SF_FORMAT_DWVW_24,		"24 bit DWVW",			NULL },

} ; /* subtype_formats */

int		
psf_get_format_subtype_count	(void)
{	return (sizeof (subtype_formats) / sizeof (SF_FORMAT_INFO)) ;
} /* psf_get_format_subtype_count */

int
psf_get_format_subtype (SF_FORMAT_INFO *data) 
{	int index ;

	if (data->format < 0 || data->format >= (SIGNED_SIZEOF (subtype_formats) / SIGNED_SIZEOF (SF_FORMAT_INFO)))
		return SFE_BAD_CONTROL_CMD ;

	index = data->format ;
	memcpy (data, &(subtype_formats [index]), sizeof (SF_FORMAT_INFO)) ;
	
	return 0 ;
} /* psf_get_format_subtype */

/*==============================================================================
*/

double
psf_calc_signal_max (SF_PRIVATE *psf)
{	sf_count_t	position ;
	double 		max_val = 0.0, temp, *data ;
	int			k, len, readcount, save_state ;

	/* If the file is not seekable, there is nothing we can do. */
	if (! psf->sf.seekable)
	{	psf->error = SFE_NOT_SEEKABLE ;
		return	0.0 ;
		} ;
		
	if (! psf->read_double)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return	0.0 ;
		} ;
		
	save_state = sf_command ((SNDFILE*) psf, SFC_GET_NORM_DOUBLE, NULL, 0) ;
	sf_command ((SNDFILE*) psf, SFC_SET_NORM_DOUBLE, NULL, SF_FALSE) ;
	
	/* Brute force. Read the whole file and find the biggest sample. */
	position = sf_seek ((SNDFILE*) psf, 0, SEEK_CUR) ; /* Get current position in file */
	sf_seek ((SNDFILE*) psf, 0, SEEK_SET) ;			/* Go to start of file. */
	
	len = sizeof (psf->buffer) / sizeof (double) ;
	
	data = (double*) psf->buffer ;
	
	readcount = len ;
	while (readcount == len)
	{	readcount = sf_read_double ((SNDFILE*) psf, data, len) ;
		for (k = 0 ; k < len ; k++)
		{	temp = fabs (data [k]) ;
			max_val  = temp > max_val ? temp : max_val ;
			} ;
		} ;
	
	sf_seek ((SNDFILE*) psf, position, SEEK_SET) ;		/* Return to original position. */
	
	sf_command ((SNDFILE*) psf, SFC_SET_NORM_DOUBLE, NULL, save_state) ;

	return	max_val ;
} /* psf_calc_signal_max */

