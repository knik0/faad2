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

#include	<stdarg.h>
#include	<string.h>
#include	<math.h>

#include	"sndfile.h"
#include	"sfendian.h"
#include	"common.h"

/*-----------------------------------------------------------------------------------------------
** Generic functions for performing endian swapping on short and int arrays.
*/

void
endswap_short_array (short *ptr, int len)
{	int k ;
	for (k = 0 ; k < len ; k++)
		ptr[k] = ((((ptr[k])>>8)&0xFF)|(((ptr[k])&0xFF)<<8)) ;
} /* endswap_short_array */

void
endswap_int_array (int *ptr, int len)
{	int k ;
	for (k = 0 ; k < len ; k++)
		ptr[k] = ((((ptr[k])>>24)&0xFF)|(((ptr[k])>>8)&0xFF00)|
					(((ptr[k])&0xFF00)<<8)|(((ptr[k])&0xFF)<<24)) ;		
} /* endswap_int_array */

/*-----------------------------------------------------------------------------------------------
** psf_log_printf allows libsndfile internal functions to print to an internal logbuffer which
** can later be displayed. 
** The format specifiers are as for printf but without the field width and other modifiers.
** Printing is performed to the logbuffer char array of the SF_PRIVATE struct. 
** Printing is done in such a way as to guarantee that the log never overflows the end of the
** logbuffer array.  
*/

#define psf_putchar(a,b)									\
			if ((a)->logindex < SF_BUFFER_LEN - 1)			\
			{	(a)->logbuffer [(a)->logindex++] = (b) ;	\
				(a)->logbuffer [(a)->logindex] = 0 ;		\
				} ;

void
psf_log_printf (SF_PRIVATE *psf, char *format, ...)
{	va_list	ap ;
	int     d, tens, shift ;
	char    c, *strptr, istr [5] ;

	va_start(ap, format);
	
	/* printf ("psf_log_printf : %s\n", format) ; */
	
	while ((c = *format++))
	{	if (c != '%')
		{	psf_putchar (psf, c) ;
			continue ;
			} ;
		
		switch((c = *format++)) 
		{	case 's': /* string */
					strptr = va_arg (ap, char *) ;
					while (*strptr)
						psf_putchar (psf, *strptr++) ;
					break;
		    
			case 'd': /* int */
					d = va_arg (ap, int) ;

					if (d == 0)
					{	psf_putchar (psf, '0') ;
						break ;
						} 
					if (d < 0)
					{	psf_putchar (psf, '-') ;
						d = -d ;
						} ;
					tens = 1 ;
					while (d / tens >= 10) 
						tens *= 10 ;
					while (tens > 0)
					{	psf_putchar (psf, '0' + d / tens) ;
						d %= tens ;
						tens /= 10 ;
						} ;
					break;
					
			case 'X': /* hex */
					d = va_arg (ap, int) ;
					
					if (d == 0)
					{	psf_putchar (psf, '0') ;
						break ;
						} ;
					shift = 28 ;
					while (! ((0xF << shift) & d))
						shift -= 4 ;
					while (shift >= 0)
					{	c = (d >> shift) & 0xF ;
						psf_putchar (psf, (c > 9) ? c + 'A' - 10 : c + '0') ;
						shift -= 4 ;
						} ;
					break;
					
			case 'c': /* char */
					c = va_arg (ap, int) & 0xFF ;
					psf_putchar (psf, c);
					break;
					
			case 'D': /* int2str */
					d = va_arg (ap, int);
					if (CPU_IS_LITTLE_ENDIAN)
					{	istr [0] = d & 0xFF ;
						istr [1] = (d >> 8) & 0xFF ;
						istr [2] = (d >> 16) & 0xFF ;
						istr [3] = (d >> 24) & 0xFF ;
						}
					else
					{	istr [3] = d & 0xFF ;
						istr [2] = (d >> 8) & 0xFF ;
						istr [1] = (d >> 16) & 0xFF ;
						istr [0] = (d >> 24) & 0xFF ;
						} ;
					istr [4] = 0 ;
					strptr = istr ;
					while (*strptr)
					{	c = *strptr++ ;
						psf_putchar (psf, c) ;
						} ;
					break;
					
			default :
					psf_putchar (psf, '*') ;
					psf_putchar (psf, c) ;
					psf_putchar (psf, '*') ;
					break ;
			} /* switch */
		} /* while */

	va_end(ap);
	return ;
} /* psf_log_printf */

/*-----------------------------------------------------------------------------------------------
**  ASCII header printf functions.
**  Some formats (ie NIST) use ascii text in their headers.
**  Format specifiers are the same as the standard printf specifiers (uses vsnprintf).
**  If this generates a compile error on any system, the author should be notified
**  so an alternative vsnprintf can be provided.
*/

void
psf_asciiheader_printf (SF_PRIVATE *psf, char *format, ...)
{	va_list	argptr ;
	int  maxlen ;
	char *start ;
	
	start  = (char*) psf->header + strlen ((char*) psf->header) ;
	maxlen = sizeof (psf->header) - strlen ((char*) psf->header) ;
	
	va_start (argptr, format) ;
	vsnprintf (start, maxlen, format, argptr) ;
	va_end (argptr) ;

	/* Make sure the string is properly terminated. */
	start [maxlen - 1] = 0 ;	

	return ;
} /* psf_asciiheader_printf */

/*-----------------------------------------------------------------------------------------------
**  Binary header writing functions. Returns number of bytes written.
**
**  Format specifiers for psf_binheader_writef are as follows
**		m	- marker - four bytes - no endian manipulation
**
**		b   - byte
**
**		w	- two byte value - little endian
**		W	- two byte value - big endian
**		l	- four byte value - little endian
**		L	- four byte value - big endian
**
**		s   - string preceded by a little endian four byte length
**		S   - string preceded by a big endian four byte length
**
**		f	- little endian 32 bit float
**		F   - big endian 32 bit float
**
**		B	- binary data (see below)
**		z   - zero bytes (se below)
**
**	To write a word followed by a long (both little endian) use:
**		psf_binheader_writef ("wl", wordval, longval) ;
**
**	To write binary data use:
**		psf_binheader_writef ("B", &bindata, sizeof (bindata)) ;
**
**	To write N zero bytes use:
**		psf_binheader_writef ("z", N) ;
*/

/* These macros may seem a bit messy but do prevent problems with processors which 
** seg. fault when asked to write an int or short to a non-int/short aligned address.
*/

#if (CPU_IS_BIG_ENDIAN == 1)
#define	PUT_INT(psf,x)		if ((psf)->headindex < sizeof ((psf)->header) - 4)					\
							{	(psf)->header [(psf)->headindex++] = ((x) >> 24) & 0xFF ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 16) & 0xFF ;		\
								(psf)->header [(psf)->headindex++] = ((x) >>  8) & 0xFF ;		\
								(psf)->header [(psf)->headindex++] = (x) & 0xFF ;   }
								                                                                   
#define	PUT_SHORT(psf,x)	if ((psf)->headindex < sizeof ((psf)->header) - 2)					\
							{	(psf)->header [(psf)->headindex++] = ((x) >> 8) & 0xFF ;		\
								(psf)->header [(psf)->headindex++] = (x) & 0xFF ;   }

#elif (CPU_IS_LITTLE_ENDIAN == 1)
#define	PUT_INT(psf,x)		if ((psf)->headindex < sizeof ((psf)->header) - 4)					\
							{	(psf)->header [(psf)->headindex++] = (x) & 0xFF ;				\
								(psf)->header [(psf)->headindex++] = ((x) >>  8) & 0xFF ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 16) & 0xFF ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 24) & 0xFF ;   }
                                                                        
#define	PUT_SHORT(psf,x)	if ((psf)->headindex < sizeof ((psf)->header) - 2)					\
							{	(psf)->header [(psf)->headindex++] = (x) & 0xFF ;				\
								(psf)->header [(psf)->headindex++] = ((x) >> 8) & 0xFF ;   }

#else
#       error "Cannot determine endian-ness of processor."
#endif

#define	PUT_BYTE(psf,x)		if ((psf)->headindex < sizeof ((psf)->header) - 1)					\
							{	(psf)->header [(psf)->headindex++] = (x) & 0xFF ;   }

int
psf_binheader_writef (SF_PRIVATE *psf, char *format, ...)
{	va_list	argptr ;
	unsigned int 	data ;
	float			floatdata ;
	void			*bindata ;
	size_t			size ;
	char    		c, *strptr ;
	int				count = 0 ;
	
	va_start(argptr, format);
	
	while ((c = *format++))
	{	switch (c)
		{	case 'm' : 
					data = va_arg (argptr, unsigned int) ;
					PUT_INT (psf, data) ;
					count += 4 ;
					break ;
					
			case 'b' :
					data = va_arg (argptr, unsigned int) ;
					PUT_BYTE (psf, data) ;
					count += 1 ;
					break ;
					
			case 'w' :
					data = va_arg (argptr, unsigned int) ;
					data = H2LE_SHORT (data) ;
					PUT_SHORT (psf, data) ;
					count += 2 ;
					break ;

			case 'W' :
					data = va_arg (argptr, unsigned int) ;
					data = H2BE_SHORT (data) ;
					PUT_SHORT (psf, data) ;
					count += 2 ;
					break ;
					
			case 'l' :
					data = va_arg (argptr, unsigned int) ;
					data = H2LE_INT (data) ;
					PUT_INT (psf, data) ;
					count += 4 ;
					break ;

			case 'L' :
					data = va_arg (argptr, unsigned int) ;
					data = H2BE_INT (data) ;
					PUT_INT (psf, data) ;
					count += 4 ;
					break ;

			case 'f' :
					floatdata = (float) va_arg (argptr, double) ;
					float32_write (floatdata, (unsigned char *) &data) ;
					data = H2LE_INT (data) ;
					PUT_INT (psf, data) ;
					count += 4 ;
					break ;

			case 'F' :
					floatdata = (float) va_arg (argptr, double) ;
					float32_write (floatdata, (unsigned char *) &data) ;
					data = H2BE_INT (data) ;
					PUT_INT (psf, data) ;
					count += 4 ;
					break ;

			case 's' :
					strptr = va_arg (argptr, char *) ;
					size   = strlen (strptr) + 1 ;
					size  += (size & 1) ;
					data = H2LE_INT (size) ;
					PUT_INT (psf, data) ;
					memcpy (&(psf->header [psf->headindex]), strptr, size) ;
					psf->headindex += size ;
					count += 4 + size ;
					break ;
					
			case 'S' :
					strptr = va_arg (argptr, char *) ;
					size   = strlen (strptr) + 1 ;
					size  += (size & 1) ;
					data = H2BE_INT (size) ;
					PUT_INT (psf, data) ;
					memcpy (&(psf->header [psf->headindex]), strptr, size) ;
					psf->headindex += size ;
					count += 4 + size ;
					break ;
					
			case 'B' :
					bindata = va_arg (argptr, void *) ;
					size    = va_arg (argptr, size_t) ;
					memcpy (&(psf->header [psf->headindex]), bindata, size) ;
					psf->headindex += size ;
					count += size ;
					break ;
					
			case 'z' :
					size    = va_arg (argptr, size_t) ;
					count += size ;
					while (size)
					{	psf->header [psf->headindex] = 0 ;
						psf->headindex ++ ;
						size -- ;
						} ;
					break ;
					
			default : 
				psf_log_printf (psf, "*** Invalid format specifier `%c'\n", c) ;
				psf->error = SFE_INTERNAL ; 
				break ;
			} ;
		} ;
	
	va_end(argptr);
	return count ;
} /* psf_binheader_writef */

/*-----------------------------------------------------------------------------------------------
**  Binary header reading functions. Returns number of bytes read.
**
**	Format specifiers are the same as for header write function above with the following
**	additions:
**
**		p   - jump a given number of position from start of file.
**
**	If format is NULL, psf_binheader_readf returns the current offset.
*/

#if (CPU_IS_BIG_ENDIAN == 1)
#define	GET_INT(psf)	( ((psf)->header [0] << 24) + ((psf)->header [1] << 16) +	\
						  ((psf)->header [2] <<  8) + ((psf)->header [3]) )

#define	GET_3BYTE(psf)	( ((psf)->header [0] << 16) + ((psf)->header [1] << 8) +	\
						  ((psf)->header [2]) )

#define	GET_SHORT(psf)	( ((psf)->header [0] <<  8) + ((psf)->header [1]) )

#elif (CPU_IS_LITTLE_ENDIAN == 1)
#define	GET_INT(psf)	( ((psf)->header [0]      ) + ((psf)->header [1] <<  8) +	\
						  ((psf)->header [2] << 16) + ((psf)->header [3] << 24) )

#define	GET_3BYTE(psf)	( ((psf)->header [0]      ) + ((psf)->header [1] <<  8) +	\
						  ((psf)->header [2] << 16) )

#define	GET_SHORT(psf)	( ((psf)->header [0]) + ((psf)->header [1] <<  8) )

#else
#       error "Cannot determine endian-ness of processor."
#endif

#define	GET_BYTE(psf)	( (psf)->header [0] )

int
psf_binheader_readf (SF_PRIVATE *psf, char const *format, ...)
{	va_list	argptr ;
	unsigned int 	*longptr, longdata ;
	unsigned short	*wordptr, worddata ;
	char    		*charptr ;
	int				position ;
	float			*floatptr ;
	size_t			size ;
	char			c ;
	int				count = 0 ;
	
	if (! format)
		return ftell (psf->file) ;
			
	va_start(argptr, format);
	
	while ((c = *format++))
	{	switch (c)
		{	case 'm' : 
					longptr = va_arg (argptr, unsigned int*) ;
					fread (psf->header, 1, sizeof (int), psf->file) ;
					*longptr = GET_INT (psf) ;
					count += 4 ;
					break ;
					
			case 'b' :
					charptr = va_arg (argptr, char*) ;
					fread (psf->header, 1, sizeof (char), psf->file) ;
					*charptr = GET_BYTE (psf) ;
					count += 1 ;
					break ;
					
			case 'w' :
					wordptr = va_arg (argptr, unsigned short*) ;
					fread (psf->header, 1, sizeof (short), psf->file) ;
					worddata = GET_SHORT (psf) ;
					*wordptr = H2LE_SHORT (worddata) ;
					count += 2 ;
					break ;

			case 'W' :
					wordptr = va_arg (argptr, unsigned short*) ;
					fread (psf->header, 1, sizeof (short), psf->file) ;
					worddata = GET_SHORT (psf) ;
					*wordptr = H2BE_SHORT (worddata) ;
					count += 2 ;
					break ;
					
			case 'l' :
					longptr = va_arg (argptr, unsigned int*) ;
					fread (psf->header, 1, sizeof (int), psf->file) ;
					longdata = GET_INT (psf) ;
					*longptr = H2LE_INT (longdata) ;
					count += 4 ;
					break ;

			case 'L' :
					longptr = va_arg (argptr, unsigned int*) ;
					fread (psf->header, 1, sizeof (int), psf->file) ;
					longdata = GET_INT (psf) ;
					*longptr = H2BE_INT (longdata) ;
					count += 4 ;
					break ;

			case 't' :
					longptr = va_arg (argptr, unsigned int*) ;
					fread (psf->header, 1, 3, psf->file) ;
					longdata = GET_3BYTE (psf) ;
					*longptr = H2LE_INT (longdata) ;
					count += 3 ;
					break ;

			case 'T' :
					longptr = va_arg (argptr, unsigned int*) ;
					fread (psf->header, 1, 3, psf->file) ;
					longdata = GET_3BYTE (psf) ;
					*longptr = H2BE_INT (longdata) ;
					count += 3 ;
					break ;

			case 'f' :
					floatptr = va_arg (argptr, float *) ;
					fread (psf->header, 1, sizeof (float), psf->file) ;
					longdata = GET_INT (psf) ;
					longdata = H2LE_INT (longdata) ;
					*floatptr = float32_read ((unsigned char*) &longdata) ;
					count += 4 ;
					break ;

			case 'F' :
					floatptr = va_arg (argptr, float *) ;
					fread (psf->header, 1, sizeof (float), psf->file) ;
					longdata = GET_INT (psf) ;
					longdata = H2BE_INT (longdata) ;
					*floatptr = float32_read ((unsigned char*) &longdata) ;
					count += 4 ;
					break ;

			case 's' :
					printf ("Format conversion not implemented yet.\n") ;
					/*
					strptr = va_arg (argptr, char *) ;
					size   = strlen (strptr) + 1 ;
					size  += (size & 1) ;
					longdata = H2LE_INT (size) ;
					get_int (psf, longdata) ;
					memcpy (&(psf->header [psf->headindex]), strptr, size) ;
					psf->headindex += size ;
					*/
					break ;
					
			case 'S' :
					printf ("Format conversion not implemented yet.\n") ;
					/*
					strptr = va_arg (argptr, char *) ;
					size   = strlen (strptr) + 1 ;
					size  += (size & 1) ;
					longdata = H2BE_INT (size) ;
					get_int (psf, longdata) ;
					memcpy (&(psf->header [psf->headindex]), strptr, size) ;
					psf->headindex += size ;
					*/
					break ;
					
			case 'B' :
					charptr = va_arg (argptr, char*) ;
					size = va_arg (argptr, size_t) ;
					if (size > 0)
					{	memset (charptr, 0, size) ;
						fread (charptr, 1, size, psf->file) ;
						count += size ;
						} ;
					break ;
					
			case 'z' :
					printf ("Format conversion not implemented yet.\n") ;
					/*
					size    = va_arg (argptr, size_t) ;
					while (size)
					{	psf->header [psf->headindex] = 0 ;
						psf->headindex ++ ;
						size -- ;
						} ;
					*/
					break ;
					
			case 'p' :
					/* Get the seek position first. */ 
					position = va_arg (argptr, int) ;
					fseek (psf->file, position, SEEK_SET) ;
					count = 0 ;
					break ;

			case 'j' :
					/* Get the seek position first. */ 
					position = va_arg (argptr, int) ;
					fseek (psf->file, position, SEEK_CUR) ;
					count = 0 ;
					break ;

			default :
				psf_log_printf (psf, "*** Invalid format specifier `%c'\n", c) ;
				psf->error = SFE_INTERNAL ; 
				break ;
			} ;
		} ;
	
	va_end (argptr);
	
	return count ;
} /* psf_binheader_readf */

/*-----------------------------------------------------------------------------------------------
*/

void
psf_log_SF_INFO (SF_PRIVATE *psf)
{	psf_log_printf (psf, "---------------------------------\n") ;

	psf_log_printf (psf, " Sample rate :   %d\n", psf->sf.samplerate) ;
	psf_log_printf (psf, " Samples     :   %d\n", psf->sf.samples) ;
	psf_log_printf (psf, " Channels    :   %d\n", psf->sf.channels) ;

	psf_log_printf (psf, " Bit width   :   %d\n", psf->sf.pcmbitwidth) ;
	psf_log_printf (psf, " Format      :   %X\n", psf->sf.format) ;
	psf_log_printf (psf, " Sections    :   %d\n", psf->sf.sections) ;
	psf_log_printf (psf, " Seekable    :   %s\n", psf->sf.seekable ? "TRUE" : "FALSE") ;
	
	psf_log_printf (psf, "---------------------------------\n") ;
} /* psf_dump_SFINFO */ 

/*========================================================================================
**	Functions used in the write function for updating the peak chunk. 
*/

/*-void	
peak_update_short	(SF_PRIVATE *psf, short *ptr, size_t items)
{	int		chan, k, position ;
	short	maxval ;
	float	fmaxval ;
	
	for (chan = 0 ; chan < psf->sf.channels ; chan++)
	{	maxval = abs (ptr [chan]) ;
		position = 0 ;
		for (k = chan ; k < items ; k += psf->sf.channels)
			if (maxval < abs (ptr [k]))
			{	maxval = abs (ptr [k]) ;
				position = k ;
				} ;
				
		fmaxval   = maxval / 32767.0 ;
		position /= psf->sf.channels ;
		
		if (fmaxval > psf->peak.peak[chan].value)
		{	psf->peak.peak[chan].value = fmaxval ;
			psf->peak.peak[chan].position = psf->current - position ;
			} ;
		} ;
	
	return ;		
} /+* peak_update_short *+/

void	
peak_update_int		(SF_PRIVATE *psf, int *ptr, size_t items)
{	int		chan, k, position ;
	int		maxval ;
	float	fmaxval ;
	
	for (chan = 0 ; chan < psf->sf.channels ; chan++)
	{	maxval = abs (ptr [chan]) ;
		position = 0 ;
		for (k = chan ; k < items ; k += psf->sf.channels)
			if (maxval < abs (ptr [k]))
			{	maxval = abs (ptr [k]) ;
				position = k ;
				} ;
				
		fmaxval   = maxval / 0x7FFFFFFF ;
		position /= psf->sf.channels ;
		
		if (fmaxval > psf->peak.peak[chan].value)
		{	psf->peak.peak[chan].value = fmaxval ;
			psf->peak.peak[chan].position = psf->current - position ;
			} ;
		} ;
	
	return ;		
} /+* peak_update_int *+/

void	
peak_update_double	(SF_PRIVATE *psf, double *ptr, size_t items)
{	int		chan, k, position ;
	double	fmaxval ;
	
	for (chan = 0 ; chan < psf->sf.channels ; chan++)
	{	fmaxval = fabs (ptr [chan]) ;
		position = 0 ;
		for (k = chan ; k < items ; k += psf->sf.channels)
			if (fmaxval < fabs (ptr [k]))
			{	fmaxval = fabs (ptr [k]) ;
				position = k ;
				} ;

		position /= psf->sf.channels ;
		
		if (fmaxval > psf->peak.peak[chan].value)
		{	psf->peak.peak[chan].value = fmaxval ;
			psf->peak.peak[chan].position = psf->current - position ;
			} ;
		} ;
	
	return ;		
} /+* peak_update_double *+/
-*/
