/*
** Copyright (C) 2001 Erik de Castro Lopo <erikd@zip.com.au>
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
#include	<stdarg.h>

#include	"sndfile.h"
#include	"config.h"
#include	"sfendian.h"
#include	"common.h"


/*------------------------------------------------------------------------------
 * Macros to handle big/little endian issues.
*/

#if (CPU_IS_LITTLE_ENDIAN == 1)
#	define	MAKE_MARKER(a,b,c,d)		((a)|((b)<<8)|((c)<<16)|((d)<<24))
#elif (CPU_IS_BIG_ENDIAN == 1)
#	define	MAKE_MARKER(a,b,c,d)		(((a)<<24)|((b)<<16)|((c)<<8)|(d))
#else
#	error "Cannot determine endian-ness of processor."
#endif

/*------------------------------------------------------------------------------
 * Typedefs for file chunks.
*/

enum
{	VOC_TYPE_TERMINATOR		= 0,
	VOC_TYPE_SOUND_DATA		= 1,
	VOC_TYPE_SOUND_CONTINUE = 2,
	VOC_TYPE_SILENCE		= 3,
	VOC_TYPE_MARKER			= 4,
	VOC_TYPE_ASCII			= 5,
	VOC_TYPE_REPEAT			= 6,
	VOC_TYPE_END_REPEAT		= 7,
	VOC_TYPE_EXTENDED		= 8,
	VOC_TYPE_9				= 9
} ;

/*------------------------------------------------------------------------------
 * Private static functions.
*/

static	int		voc_close	(SF_PRIVATE  *psf) ;

/*------------------------------------------------------------------------------
** Public functions.
*/

int
voc_open_read	(SF_PRIVATE *psf)
{	char	creative [20], type ;
	short	version ;

	/* Set position to start of file to begin reading header. */
	psf_binheader_readf (psf, "pB", 0, creative, sizeof (creative)) ;
		
	if (creative [sizeof (creative) - 1] != 0x1A)
		return SFE_VOC_NO_CREATIVE ;
		
	/* Terminate the string. */
	creative [sizeof (creative) - 1] = 0 ;
	
	if (strcmp ("Creative Voice File", creative))
		return SFE_VOC_NO_CREATIVE ;

	psf_log_printf (psf, "%s\n", creative) ;

	psf_binheader_readf (psf, "ww", &(psf->dataoffset), &version) ;

	psf_log_printf (psf, "dataoffset : %d\n", psf->dataoffset) ;
	psf_log_printf (psf, "version    : %X\n", version) ;

	if (version != 0x010A && version != 0x0114)
		return SFE_VOC_BAD_VERSION ;

	psf_binheader_readf (psf, "w", &version) ;
	psf_log_printf (psf, "version 2  : %X\n", version) ;
	
	while (1)
	{	psf_binheader_readf (psf, "b", &type) ;
	
		switch (type)
		{	case VOC_TYPE_TERMINATOR :
					psf_log_printf (psf, " Terminator\n") ;
					break ;

			case VOC_TYPE_SOUND_DATA :
					{	unsigned char rate_byte, compression ;
						int		size ;

						psf_binheader_readf (psf, "tbb", &size, &rate_byte, &compression) ;

						psf_log_printf (psf, " Sound Data : %d\n", size) ;
						psf_log_printf (psf, "  sr   : %d => %dHz\n", (rate_byte & 0xFF), 1000000 / (256 - rate_byte)) ;
						psf_log_printf (psf, "  comp : %d\n", compression) ;
	
						psf_binheader_readf (psf, "j", size) ;
						} ;
					break ;

			case VOC_TYPE_SOUND_CONTINUE :
					{	int		size ;

						psf_binheader_readf (psf, "t", &size) ;

						psf_log_printf (psf, " Sound Continue : %d\n", size) ;
	
						psf_binheader_readf (psf, "j", size) ;
						} ;
					break ;

			case VOC_TYPE_SILENCE :
					{	unsigned char rate_byte ;
						short length ;
						
						psf_log_printf (psf, " Silence\n") ;
						psf_binheader_readf (psf, "wb", &length, &rate_byte) ;
						psf_log_printf (psf, "  length : %d\n", length) ;
						psf_log_printf (psf, "  sr     : %d => %dHz\n", (rate_byte & 0xFF), 1000000 / (256 - rate_byte)) ;
		  				} ;
					break ;

			case VOC_TYPE_MARKER :
					{	int		size ;
						short	value ;

						psf_log_printf (psf, " Marker\n") ;

						psf_binheader_readf (psf, "tw", &size, &value) ;
						
						psf_log_printf (psf, "  size  : %d\n", size) ;
						psf_log_printf (psf, "  value : %d\n", value) ;
						} ;
					break ;

			case VOC_TYPE_ASCII :
					{	int		size ;

						psf_binheader_readf (psf, "t", &size) ;

						psf_log_printf (psf, " ASCII : %d\n", size) ;
	
						psf_binheader_readf (psf, "B", psf->header, size) ;
						psf->header [size] = 0 ;
						psf_log_printf (psf, "  text : %s\n", psf->header) ;
						} ;
					break ;

			case VOC_TYPE_REPEAT :
					{	int		size ;
						short	count ;

						psf_binheader_readf (psf, "tw", &size, &count) ;

						psf_log_printf (psf, " Marker : %d\n", size) ;
						psf_log_printf (psf, "  value : %d\n", count) ;
						} ;
					break ;

			case VOC_TYPE_END_REPEAT :
					psf_log_printf (psf, " End Repeat\n") ;
					break ;
						
			case VOC_TYPE_EXTENDED :
					{	unsigned char pack, mode ;
						short 	rate_short ;
						int		size, sample_rate ;

						psf_binheader_readf (psf, "t", &size) ;
						psf_log_printf (psf, " Extended : %d\n", size) ;
						
						psf_binheader_readf (psf, "wbb", &rate_short, &pack, &mode) ;
						psf_log_printf (psf, "  size : %d\n", size) ;
						psf_log_printf (psf, "  pack : %d\n", pack) ;
						psf_log_printf (psf, "  mode : %d\n", mode) ;
						
						if (mode)
							sample_rate = 128000000 / (65536 - rate_short) ;
						else
							sample_rate = 256000000 / (65536 - rate_short) ;

						psf_log_printf (psf, "  sr   : %d => %dHz\n", (rate_short & 0xFFFF), sample_rate) ;
						psf_binheader_readf (psf, "j", size) ;
						} ;
					break ;

			case VOC_TYPE_9 :
					{	unsigned char bitwidth, channels, byte6 ;
						int sample_rate, size, bytecount = 0 ;
						
						psf_binheader_readf (psf, "t", &size) ;
						psf_log_printf (psf, " Type 9 : %d\n", size) ;

						bytecount = psf_binheader_readf (psf, "lbbb", &sample_rate, &bitwidth, &channels, &byte6) ;

						psf_log_printf (psf, "  sample rate : %d\n", sample_rate) ;
						psf_log_printf (psf, "  bit width   : %d\n", bitwidth) ;
						psf_log_printf (psf, "  channels    : %d\n", channels) ;

						psf_binheader_readf (psf, "j", size - bytecount) ;
						} ;
					break ;

			default :
				psf_log_printf (psf, "Unknown type : %d\n", type & 0xFF) ;
				return SFE_VOC_BAD_MARKER ;
			} ;

		if (ftell (psf->file) >= psf->filelength)
			break ;
		} ;



	psf->sf.seekable = SF_TRUE ;

	psf->close = (func_close) voc_close ;

	return 0 ;
} /* voc_open_read */

int 	
voc_open_write	(SF_PRIVATE *psf)
{	

	return 0 ;
} /* voc_open_write */

/*------------------------------------------------------------------------------
*/

static int	
voc_close	(SF_PRIVATE  *psf)
{	
	if (psf->mode == SF_MODE_WRITE)
	{	/*  Now we know for certain the length of the file we can re-write 
		**	correct values for the FORM, 8SVX and BODY chunks.
		*/
                
		} ;

	if (psf->fdata)
		free (psf->fdata) ;
	psf->fdata = NULL ;
	
	return 0 ;
} /* voc_close */


/*------------------------------------------------------------------------------------

Creative Voice (VOC) file format
--------------------------------

~From: galt@dsd.es.com

(byte numbers are hex!)

    HEADER (bytes 00-19)
    Series of DATA BLOCKS (bytes 1A+) [Must end w/ Terminator Block]

- ---------------------------------------------------------------

HEADER:
=======
     byte #     Description
     ------     ------------------------------------------
     00-12      "Creative Voice File"
     13         1A (eof to abort printing of file)
     14-15      Offset of first datablock in .voc file (std 1A 00
                in Intel Notation)
     16-17      Version number (minor,major) (VOC-HDR puts 0A 01)
     18-19      1's Comp of Ver. # + 1234h (VOC-HDR puts 29 11)

- ---------------------------------------------------------------

DATA BLOCK:
===========

   Data Block:  TYPE(1-byte), SIZE(3-bytes), INFO(0+ bytes)
   NOTE: Terminator Block is an exception -- it has only the TYPE byte.

      TYPE   Description     Size (3-byte int)   Info
      ----   -----------     -----------------   -----------------------
      00     Terminator      (NONE)              (NONE)
      01     Sound data      2+length of data    *
      02     Sound continue  length of data      Voice Data
      03     Silence         3                   **
      04     Marker          2                   Marker# (2 bytes)
      05     ASCII           length of string    null terminated string
      06     Repeat          2                   Count# (2 bytes)
      07     End repeat      0                   (NONE)
      08     Extended        4                   ***

      *Sound Info Format:    
       --------------------- 
       00   Sample Rate      
       01   Compression Type 
       02+  Voice Data

      **Silence Info Format:
      ----------------------------
      00-01  Length of silence - 1
      02     Sample Rate
      

    ***Extended Info Format:
       ---------------------
       00-01  Time Constant: Mono: 65536 - (256000000/sample_rate)
                             Stereo: 65536 - (25600000/(2*sample_rate))
       02     Pack
       03     Mode: 0 = mono
                    1 = stereo


  Marker#           -- Driver keeps the most recent marker in a status byte
  Count#            -- Number of repetitions + 1
                         Count# may be 1 to FFFE for 0 - FFFD repetitions
                         or FFFF for endless repetitions
  Sample Rate       -- SR byte = 256-(1000000/sample_rate)
  Length of silence -- in units of sampling cycle
  Compression Type  -- of voice data
                         8-bits    = 0
                         4-bits    = 1
                         2.6-bits  = 2
                         2-bits    = 3
                         Multi DAC = 3+(# of channels) [interesting--
                                       this isn't in the developer's manual]


---------------------------------------------------------------------------------
Addendum submitted by Votis Kokavessis:

After some experimenting with .VOC files I found out that there is a Data Block 
Type 9, which is not covered in the VOC.TXT file. Here is what I was able to discover 
about this block type:
  
  
TYPE: 09
SIZE: 12 + length of data
INFO: 12 (twelve) bytes
  
INFO STRUCTURE:
  
Bytes 0-1: (Word) Sample Rate (e.g. 44100)
Bytes 2-3: zero (could be that bytes 0-3 are a DWord for Sample Rate)
Byte 4: Sample Size in bits (e.g. 16)
Byte 5: Number of channels (e.g. 1 for mono, 2 for stereo)
Byte 6: Unknown (equal to 4 in all files I examined)
Bytes 7-11: zero


-------------------------------------------------------------------------------------*/

