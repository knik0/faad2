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
endswap_short_array (short *ptr, sf_count_t len)
{	unsigned char *ucptr, temp ;

	ucptr = (unsigned char *) ptr ;
	while (len > 0)
	{	len -- ;
		temp = ucptr [2 * len] ;
		ucptr [2 * len] = ucptr [2 * len + 1] ;
		ucptr [2 * len + 1] = temp ;
		} ;
} /* endswap_short_array */

void
endswap_int_array (int *ptr, sf_count_t len)
{	unsigned char *ucptr, temp ;
	
	ucptr = (unsigned char *) ptr ;
	while (len > 0)
	{	len -- ;

		temp = ucptr [4 * len] ;
		ucptr [4 * len] = ucptr [4 * len + 3] ;
		ucptr [4 * len + 3] = temp ;

		temp = ucptr [4 * len + 1] ;
		ucptr [4 * len + 1] = ucptr [4 * len + 2] ;
		ucptr [4 * len + 2] = temp ;
		} ;
} /* endswap_int_array */

/*	This function assumes that sizeof (long) == 8, but works correctly even 
**	is sizeof (long) == 4.
*/
void
endswap_long_array (long *ptr, sf_count_t len)
{	unsigned char *ucptr, temp ;
	
	ucptr = (unsigned char *) ptr ;
	while (len > 0)
	{	len -- ;
	
		temp = ucptr [8 * len] ;
		ucptr [8 * len] = ucptr [8 * len + 7] ;
		ucptr [8 * len + 7] = temp ;

		temp = ucptr [8 * len + 1] ;
		ucptr [8 * len + 1] = ucptr [8 * len + 6] ;
		ucptr [8 * len + 6] = temp ;
		
		temp = ucptr [8 * len + 2] ;
		ucptr [8 * len + 2] = ucptr [8 * len + 5] ;
		ucptr [8 * len + 5] = temp ;
		
		temp = ucptr [8 * len + 3] ;
		ucptr [8 * len + 3] = ucptr [8 * len + 4] ;
		ucptr [8 * len + 4] = temp ;
		} ;
} /* endswap_long_array */

/*-----------------------------------------------------------------------------------------------
*/

int 
subformat_to_bytewidth (int format)
{
	switch (format)
	{	case SF_FORMAT_PCM_U8 :
		case SF_FORMAT_PCM_S8 :
				return 1 ;
		case SF_FORMAT_PCM_16 :
				return 2 ;
		case SF_FORMAT_PCM_24 :
				return 3 ;
		case SF_FORMAT_PCM_32 :
		case SF_FORMAT_FLOAT :
				return 4 ;
		case SF_FORMAT_DOUBLE :
				return 8 ;
		};
	
	return 0 ;
} /* subformat_to_bytewidth */

int 
s_bitwidth_to_subformat (int bits)
{	static int array [] = 
	{	SF_FORMAT_PCM_S8, SF_FORMAT_PCM_16, SF_FORMAT_PCM_24, SF_FORMAT_PCM_32 
		} ;
		
	if (bits < 8 || bits > 32)
		return 0 ;
		
	return array [((bits + 7) / 8) - 1] ;
} /* bitwidth_to_subformat */

int 
u_bitwidth_to_subformat (int bits)
{	static int array [] = 
	{	SF_FORMAT_PCM_U8, SF_FORMAT_PCM_16, SF_FORMAT_PCM_24, SF_FORMAT_PCM_32 
		} ;
		
	if (bits < 8 || bits > 32)
		return 0 ;
		
	return array [((bits + 7) / 8) - 1] ;
} /* bitwidth_to_subformat */

/*-----------------------------------------------------------------------------------------------
** psf_log_printf allows libsndfile internal functions to print to an internal logbuffer which
** can later be displayed. 
** The format specifiers are as for printf but without the field width and other modifiers.
** Printing is performed to the logbuffer char array of the SF_PRIVATE struct. 
** Printing is done in such a way as to guarantee that the log never overflows the end of the
** logbuffer array.  
*/

#define LOG_PUTCHAR(a,b)									\
			if ((a)->logindex < SF_BUFFER_LEN - 1)			\
			{	(a)->logbuffer [(a)->logindex++] = (b) ;	\
				(a)->logbuffer [(a)->logindex] = 0 ;		\
				}											\
			else											\
				break ;

void
psf_log_printf (SF_PRIVATE *psf, char *format, ...)
{	va_list			ap ;
	unsigned int	u ;
	int     		d, tens, shift ;
	char    		c, *strptr, istr [5] ;

	va_start(ap, format);
	
	/* printf ("psf_log_printf : %s\n", format) ; */
	
	while ((c = *format++))
	{	if (c != '%')
		{	LOG_PUTCHAR (psf, c) ;
			continue ;
			} ;
		
		switch((c = *format++)) 
		{	case 's': /* string */
					strptr = va_arg (ap, char *) ;
					while (*strptr)
						LOG_PUTCHAR (psf, *strptr++) ;
					break;
		    
			case 'd': /* int */
					d = va_arg (ap, int) ;

					if (d == 0)
					{	LOG_PUTCHAR (psf, '0') ;
						break ;
						} 
					if (d < 0)
					{	LOG_PUTCHAR (psf, '-') ;
						d = -d ;
						} ;
					tens = 1 ;
					while (d / tens >= 10) 
						tens *= 10 ;
					while (tens > 0)
					{	LOG_PUTCHAR (psf, '0' + d / tens) ;
						d %= tens ;
						tens /= 10 ;
						} ;
					break;
					
			case 'u': /* unsigned int */
					u = va_arg (ap, unsigned int) ;

					if (u == 0)
					{	LOG_PUTCHAR (psf, '0') ;
						break ;
						} 
					tens = 1 ;
					while (u / tens >= 10) 
						tens *= 10 ;
					while (tens > 0)
					{	LOG_PUTCHAR (psf, '0' + u / tens) ;
						u %= tens ;
						tens /= 10 ;
						} ;
					break;
					
			case 'c': /* char */
					c = va_arg (ap, int) & 0xFF ;
					LOG_PUTCHAR (psf, c);
					break;
					
			case 'X': /* hex */
					d = va_arg (ap, int) ;
					
					if (d == 0)
					{	LOG_PUTCHAR (psf, '0') ;
						break ;
						} ;
					shift = 28 ;
					while (! ((0xF << shift) & d))
						shift -= 4 ;
					while (shift >= 0)
					{	c = (d >> shift) & 0xF ;
						LOG_PUTCHAR (psf, (c > 9) ? c + 'A' - 10 : c + '0') ;
						shift -= 4 ;
						} ;
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
						LOG_PUTCHAR (psf, c) ;
						} ;
					break;
					
			case 'C': /* int2str */
					{	sf_count_t lld, tens ;
					
						lld = va_arg (ap, sf_count_t);

						if (lld == 0)
						{	LOG_PUTCHAR (psf, '0') ;
							break ;
							} 
						if (lld < 0)
						{	LOG_PUTCHAR (psf, '-') ;
							lld = -lld ;
							} ;
						tens = 1 ;
						while (lld / tens >= 10) 
							tens *= 10 ;
						while (tens > 0)
						{	LOG_PUTCHAR (psf, '0' + lld / tens) ;
							lld %= tens ;
							tens /= 10 ;
							} ;


						} ;
					break;
					
			default :
					LOG_PUTCHAR (psf, '*') ;
					LOG_PUTCHAR (psf, c) ;
					LOG_PUTCHAR (psf, '*') ;
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
	LSF_VSNPRINTF (start, maxlen, format, argptr) ;
	va_end (argptr) ;

	/* Make sure the string is properly terminated. */
	start [maxlen - 1] = 0 ;
	
	psf->headindex = strlen (psf->header) ;

	return ;
} /* psf_asciiheader_printf */

/*-----------------------------------------------------------------------------------------------
**  Binary header writing functions. Returns number of bytes written.
**
**  Format specifiers for psf_binheader_writef are as follows
**		m	- marker - four bytes - no endian manipulation
**
**		e   - all following numerical values will be little endian
**		E   - all following numerical values will be big endian
**
**		t   - all following O types will be truncated to 4 bytes
**		T   - switch off truncation of all following O types
**
**		1	- single byte value
**		2	- two byte value
**		3	- three byte value
**		4	- four byte value
**		8	- eight byte value (sometimes written as 4 bytes)
**
**		s   - string preceded by a little endian four byte length
**		f	- floating point data
**		d	- double precision floating point data
**		h	- 16 binary bytes value
**
**		b	- binary data (see below)
**		z   - zero bytes (se below)
**		h   - zero bytes (se below)
**
**	To write a word followed by an int (both little endian) use:
**		psf_binheader_writef ("e24", wordval, sf_count_tval) ;
**
**	To write binary data use:
**		psf_binheader_writef ("b", &bindata, sizeof (bindata)) ;
**
**	To write N zero bytes use:
**		psf_binheader_writef ("z", N) ;
*/

/* These macros may seem a bit messy but do prevent problems with processors which 
** seg. fault when asked to write an int or short to a non-int/short aligned address.
*/

#define	PUT_BYTE(psf,x)		if ((psf)->headindex < sizeof ((psf)->header) - 1)	\
							{	(psf)->header [(psf)->headindex++] = (x) ;   }

#if (CPU_IS_BIG_ENDIAN == 1)
#define	PUT_MARKER(psf,x)	if ((psf)->headindex < sizeof ((psf)->header) - 4)			\
							{	(psf)->header [(psf)->headindex++] = ((x) >> 24) ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 16) ;		\
								(psf)->header [(psf)->headindex++] = ((x) >>  8);		\
								(psf)->header [(psf)->headindex++] = (x) ;   }
								                                                                   
#elif (CPU_IS_LITTLE_ENDIAN == 1)
#define	PUT_MARKER(psf,x)	if ((psf)->headindex < sizeof ((psf)->header) - 4)			\
							{	(psf)->header [(psf)->headindex++] = (x) ;				\
								(psf)->header [(psf)->headindex++] = ((x) >>  8) ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 16) ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 24) ;   }

#else
#       error "Cannot determine endian-ness of processor."
#endif


#define	PUT_BE_SHORT(psf,x)	if ((psf)->headindex < sizeof ((psf)->header) - 2)			\
							{	(psf)->header [(psf)->headindex++] = ((x) >> 8) ;		\
								(psf)->header [(psf)->headindex++] = (x) ; 		}

#define	PUT_LE_SHORT(psf,x)	if ((psf)->headindex < sizeof ((psf)->header) - 2)			\
							{	(psf)->header [(psf)->headindex++] = (x) ;				\
								(psf)->header [(psf)->headindex++] = ((x) >> 8) ;	}

#define	PUT_BE_3BYTE(psf,x)	if ((psf)->headindex < sizeof ((psf)->header) - 3)			\
							{	(psf)->header [(psf)->headindex++] = ((x) >> 16) ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 8) ;		\
								(psf)->header [(psf)->headindex++] = (x) ;		}

#define	PUT_LE_3BYTE(psf,x)	if ((psf)->headindex < sizeof ((psf)->header) - 3)			\
							{	(psf)->header [(psf)->headindex++] = (x) ;				\
								(psf)->header [(psf)->headindex++] = ((x) >> 8) ;   	\
								(psf)->header [(psf)->headindex++] = ((x) >> 16) ;	}

#define	PUT_BE_INT(psf,x)	if ((psf)->headindex < sizeof ((psf)->header) - 4)			\
							{	(psf)->header [(psf)->headindex++] = ((x) >> 24) ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 16) ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 8) ;		\
								(psf)->header [(psf)->headindex++] = (x) ;		}

#define	PUT_LE_INT(psf,x)	if ((psf)->headindex < sizeof ((psf)->header) - 4)			\
							{	(psf)->header [(psf)->headindex++] = (x) ;				\
								(psf)->header [(psf)->headindex++] = ((x) >> 8) ;   	\
								(psf)->header [(psf)->headindex++] = ((x) >> 16) ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 24) ;	}

#if (SIZEOF_SF_COUNT_T == 4)
#define	PUT_BE_8BYTE(psf,x)	if ((psf)->headindex < sizeof ((psf)->header) - 8)			\
							{	(psf)->header [(psf)->headindex++] = 0 ;				\
								(psf)->header [(psf)->headindex++] = 0 ;				\
								(psf)->header [(psf)->headindex++] = 0 ;				\
								(psf)->header [(psf)->headindex++] = 0 ;				\
								(psf)->header [(psf)->headindex++] = ((x) >> 24) ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 16) ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 8) ;		\
								(psf)->header [(psf)->headindex++] = (x) ;		}

#define	PUT_LE_8BYTE(psf,x)	if ((psf)->headindex < sizeof ((psf)->header) - 8)			\
							{	(psf)->header [(psf)->headindex++] = (x) ;				\
								(psf)->header [(psf)->headindex++] = ((x) >> 8) ;   	\
								(psf)->header [(psf)->headindex++] = ((x) >> 16) ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 24) ;		\
								(psf)->header [(psf)->headindex++] = 0 ;				\
								(psf)->header [(psf)->headindex++] = 0 ;   				\
								(psf)->header [(psf)->headindex++] = 0 ;				\
								(psf)->header [(psf)->headindex++] = 0 ;	}

#elif  (SIZEOF_SF_COUNT_T == 8)
#define	PUT_BE_8BYTE(psf,x)	if ((psf)->headindex < sizeof ((psf)->header) - 8)			\
							{	(psf)->header [(psf)->headindex++] = ((x) >> 56) ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 48) ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 40) ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 32) ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 24) ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 16) ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 8) ;		\
								(psf)->header [(psf)->headindex++] = (x) ;		}

#define	PUT_LE_8BYTE(psf,x)	if ((psf)->headindex < sizeof ((psf)->header) - 8)			\
							{	(psf)->header [(psf)->headindex++] = (x) ;				\
								(psf)->header [(psf)->headindex++] = ((x) >> 8) ;   	\
								(psf)->header [(psf)->headindex++] = ((x) >> 16) ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 24) ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 32) ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 40) ;   	\
								(psf)->header [(psf)->headindex++] = ((x) >> 48) ;		\
								(psf)->header [(psf)->headindex++] = ((x) >> 56) ;	}
#else
#error "SIZEOF_SF_COUNT_T is not defined."
#endif
int
psf_binheader_writef (SF_PRIVATE *psf, char *format, ...)
{	va_list	argptr ;
	sf_count_t 		countdata ;
	unsigned long 	longdata ;
	unsigned int 	data ;
	float			floatdata ;
	void			*bindata ;
	size_t			size ;
	char    		c, *strptr ;
	int				count = 0, endian, trunc_8to4 ;
	
	endian = SF_ENDIAN_LITTLE ;
	trunc_8to4 = SF_FALSE ;
	
	va_start(argptr, format);
	
	while ((c = *format++))
	{	switch (c)
		{	case 'e' : /* All conversions are now from LE to host. */
					endian = SF_ENDIAN_LITTLE ;
					break ;

			case 'E' : /* All conversions are now from BE to host. */
					endian = SF_ENDIAN_BIG ;
					break ;

			case 't' : /* All 8 byte values now get written as 4 bytes. */
					trunc_8to4 = SF_TRUE ;
					break ;

			case 'T' : /* All 8 byte values now get written as 8 bytes. */
					trunc_8to4 = SF_FALSE ;
					break ;

			case 'm' : 
					data = va_arg (argptr, unsigned int) ;
					PUT_MARKER (psf, data) ;
					count += 4 ;
					break ;
					
			case '1' :
					data = va_arg (argptr, unsigned int) ;
					PUT_BYTE (psf, data) ;
					count += 1 ;
					break ;
					
			case '2' :
					data = va_arg (argptr, unsigned int) ;
					if (endian == SF_ENDIAN_BIG)
					{	PUT_BE_SHORT (psf, data) ;
						}
					else
					{	PUT_LE_SHORT (psf, data) ;
						} ;
					count += 2 ;
					break ;

			case '3' : /* tribyte */
					data = va_arg (argptr, unsigned int) ;
					if (endian == SF_ENDIAN_BIG)
					{	PUT_BE_3BYTE (psf, data) ;
						}
					else
					{	PUT_LE_3BYTE (psf, data) ;
						} ;
					count += 3 ;
					break ;

			case '4' :
					data = va_arg (argptr, unsigned int) ;
					if (endian == SF_ENDIAN_BIG)
					{	PUT_BE_INT (psf, data) ;
						}
					else
					{	PUT_LE_INT (psf, data) ;
						} ;
					count += 4 ;
					break ;

			case '8' :
					countdata = va_arg (argptr, sf_count_t) ;
					if (endian == SF_ENDIAN_BIG && trunc_8to4 == SF_FALSE)
					{	PUT_BE_8BYTE (psf, countdata) ;
						count += 8 ;
						}
					else if (endian == SF_ENDIAN_LITTLE && trunc_8to4 == SF_FALSE)
					{	PUT_LE_8BYTE (psf, countdata) ;
						count += 8 ;
						}
					else if (endian == SF_ENDIAN_BIG && trunc_8to4 == SF_TRUE)
					{	longdata = countdata ;
						PUT_BE_INT (psf, longdata) ;
						count += 4 ;
						}
					else if (endian == SF_ENDIAN_LITTLE && trunc_8to4 == SF_TRUE)
					{	longdata = countdata ;
						PUT_LE_INT (psf, longdata) ;
						count += 4 ;
						}
					break ;

			case 'f' :
					floatdata = (float) va_arg (argptr, double) ;
					float32_write (floatdata, (unsigned char *) &data) ;
					if (endian == SF_ENDIAN_BIG)
					{	PUT_BE_INT (psf, data) ;
						}
					else
					{	PUT_LE_INT (psf, data) ;
						} ;
					count += 4 ;
					break ;

			case 'd' :
					psf_log_printf (psf, "Must fix double conversion\n") ;
					/*
					doubledata = va_arg (argptr, double) ;
					double64_write (doubledata, (unsigned char *) &data) ;
					if (endian == SF_ENDIAN_BIG)
					{	PUT_BE_INT (psf, data) ;
						}
					else
					{	PUT_LE_INT (psf, data) ;
						} ;
					count += 4 ;
					*/
					break ;

			case 's' :
					strptr = va_arg (argptr, char *) ;
					size   = strlen (strptr) + 1 ;
					size  += (size & 1) ;
					if (endian == SF_ENDIAN_BIG)
					{	PUT_BE_INT (psf, size) ;
						}
					else
					{	PUT_LE_INT (psf, size) ;
						} ;
					memcpy (&(psf->header [psf->headindex]), strptr, size) ;
					psf->headindex += size ;
					count += 4 + size ;
					break ;
					
			case 'b' :
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
					
			case 'h' :
					bindata = va_arg (argptr, void *) ; ;
					memcpy (&(psf->header [psf->headindex]), bindata, 16) ;
					psf->headindex += 16 ;
					count += 16 ;
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
**		j   - jump a given number of bytes forward in the file.
**
**	If format is NULL, psf_binheader_readf returns the current offset.
*/

#define	GET_BYTE(psf)	( (psf)->header [0] )

#if (CPU_IS_BIG_ENDIAN == 1)
#define	GET_MARKER(psf)	( ((psf)->header [0] << 24) | ((psf)->header [1] << 16) |	\
						  ((psf)->header [2] <<  8) | ((psf)->header [3]) )

#elif (CPU_IS_LITTLE_ENDIAN == 1)
#define	GET_MARKER(psf)	( ((psf)->header [0]      ) | ((psf)->header [1] <<  8) |	\
						  ((psf)->header [2] << 16) | ((psf)->header [3] << 24) )

#else
#       error "Cannot determine endian-ness of processor."
#endif

#define	GET_LE_SHORT(psf)	( ((psf)->header [1] <<  8) | ((psf)->header [0]) )

#define	GET_BE_SHORT(psf)	( ((psf)->header [0] <<  8) | ((psf)->header [1]) )


#define	GET_LE_3BYTE(psf)	( 	((psf)->header [2] << 16) | ((psf)->header [1] << 8) |	\
								((psf)->header [0]) )

#define	GET_BE_3BYTE(psf)	( 	((psf)->header [0] << 16) | ((psf)->header [1] << 8) |	\
								((psf)->header [2]) )

#define	GET_LE_INT(psf)		( 	((psf)->header [3] << 24) | ((psf)->header [2] << 16) |	\
								((psf)->header [1] <<  8) | ((psf)->header [0]) )

#define	GET_BE_INT(psf)		( 	((psf)->header [0] << 24) | ((psf)->header [1] << 16) |	\
							 	((psf)->header [2] <<  8) | ((psf)->header [3]) )

#if (SIZEOF_LONG == 4)
#define	GET_LE_8BYTE(psf)	( 	((psf)->header [3] << 24) | ((psf)->header [2] << 16) |	\
							 	((psf)->header [1] <<  8) | ((psf)->header [0]) )

#define	GET_BE_8BYTE(psf)	( 	((psf)->header [4] << 24) | ((psf)->header [5] << 16) |	\
								((psf)->header [6] <<  8) | ((psf)->header [7]) )
#else
#define	GET_LE_8BYTE(psf)	( 	((psf)->header [7] << 56L) | ((psf)->header [6] << 48L) |	\
							 	((psf)->header [5] << 40L) | ((psf)->header [4] << 32L) |	\
							 	((psf)->header [3] << 24L) | ((psf)->header [2] << 16L) |	\
							 	((psf)->header [1] <<  8L) | ((psf)->header [0] ))

#define	GET_BE_8BYTE(psf)	( 	((psf)->header [0] << 56L) | ((psf)->header [1] << 48L) |	\
							 	((psf)->header [2] << 40L) | ((psf)->header [3] << 32L) |	\
							 	((psf)->header [4] << 24L) | ((psf)->header [5] << 16L) |	\
							 	((psf)->header [6] <<  8L) | ((psf)->header [7] ))

#endif

int
psf_binheader_readf (SF_PRIVATE *psf, char const *format, ...)
{	va_list			argptr ;
	sf_count_t		*countptr, countdata ;
	unsigned int 	*intptr, intdata ;
	unsigned short	*shortptr, shortdata ;
	char    		*charptr ;
	int				position ;
	float			*floatptr ;
	size_t			size ;
	char			c ;
	int				count = 0, endian = SF_ENDIAN_LITTLE ;
	
	if (! format)
		return psf_ftell (psf->filedes) ;
	
	va_start(argptr, format);
	
	while ((c = *format++))
	{	switch (c)
		{	case 'e' : /* All conversions are now from LE to host. */
					endian = SF_ENDIAN_LITTLE ;
					break ;

			case 'E' : /* All conversions are now from BE to host. */
					endian = SF_ENDIAN_BIG ;
					break ;

			case 'm' : 
					intptr = va_arg (argptr, unsigned int*) ;
					count += psf_fread (psf->header, 1, sizeof (int), psf->filedes) ;
					*intptr = GET_MARKER (psf) ;
					break ;
					
			case 'h' : 
					intptr = va_arg (argptr, unsigned int*) ;
					count += psf_fread (psf->header, 1, 16, psf->filedes) ;
					{	int k ;
						intdata = 0 ;
						for (k = 0 ; k < 16 ; k++)
							intdata ^= psf->header [k] << k ;
						}
					*intptr = intdata ;
					break ;
					
			case '1' :
					charptr = va_arg (argptr, char*) ;
					count += psf_fread (psf->header, 1, sizeof (char), psf->filedes) ;
					*charptr = GET_BYTE (psf) ;
					break ;
					
			case '2' :
					shortptr = va_arg (argptr, unsigned short*) ;
					count += psf_fread (psf->header, 1, sizeof (short), psf->filedes) ;
					if (endian == SF_ENDIAN_BIG)
						shortdata = GET_BE_SHORT (psf) ;
					else
						shortdata = GET_LE_SHORT (psf) ;
					*shortptr = shortdata ;
					break ;

			case '3' :
					intptr = va_arg (argptr, unsigned int*) ;
					count += psf_fread (psf->header, 1, 3, psf->filedes) ;
					if (endian == SF_ENDIAN_BIG)
						intdata = GET_BE_3BYTE (psf) ;
					else
						intdata = GET_LE_3BYTE (psf) ;
					*intptr = intdata ;
					break ;

			case '4' :
					intptr = va_arg (argptr, unsigned int*) ;
					count += psf_fread (psf->header, 1, sizeof (int), psf->filedes) ;
					if (endian == SF_ENDIAN_BIG)
						intdata = GET_BE_INT (psf) ;
					else
						intdata = GET_LE_INT (psf) ;
					*intptr = intdata ;
					break ;

			case '8' :
					countptr = va_arg (argptr, sf_count_t*) ;
					count += psf_fread (psf->header, 1, 8, psf->filedes) ;
					if (endian == SF_ENDIAN_BIG)
						countdata = GET_BE_8BYTE (psf) ;
					else
						countdata = GET_LE_8BYTE (psf) ;
					*countptr = countdata ;
					break ;

			case 'f' : /* Float conversion */
					floatptr = va_arg (argptr, float *) ;
					count += psf_fread (psf->header, 1, sizeof (float), psf->filedes) ;
					if (endian == SF_ENDIAN_BIG)
						intdata = GET_BE_INT (psf) ;
					else
						intdata = GET_LE_INT (psf) ;
					*floatptr = float32_read ((unsigned char*) &intdata) ;
					break ;

			case 'd' : /* double conversion */
					psf_log_printf (psf, "Must fix double conversion\n") ;
					/*
					doubleptr = va_arg (argptr, double *) ;
					count += psf_fread (psf->header, 1, sizeof (double), psf->filedes) ;
					if (endian == SF_ENDIAN_BIG)
						longdata = GET_BE_8BYTE (psf) ;
					else
						longdata = GET_LE_8BYTE (psf) ;
					*doubleptr = double32_read ((unsigned char*) &longdata) ;
					*/
					break ;

			case 's' :
					psf_log_printf (psf, "Format conversion 's' not implemented yet.\n") ;
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
					
			case 'b' :
					charptr = va_arg (argptr, char*) ;
					size = va_arg (argptr, size_t) ;
					if (size > 0)
					{	memset (charptr, 0, size) ;
						count += psf_fread (charptr, 1, size, psf->filedes) ;
						} ;
					break ;
					
			case 'z' :
					psf_log_printf (psf, "Format conversion 'z' not implemented yet.\n") ;
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
					psf_fseek (psf->filedes, position, SEEK_SET) ;
					count = 0 ;
					break ;

			case 'j' :
					/* Get the seek position first. */ 
					position = va_arg (argptr, int) ;
					psf_fseek (psf->filedes, position, SEEK_CUR) ;
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
	psf_log_printf (psf, " Samples     :   %C\n", psf->sf.samples) ;
	psf_log_printf (psf, " Channels    :   %d\n", psf->sf.channels) ;

	psf_log_printf (psf, " Format      :   0x%X\n", psf->sf.format) ;
	psf_log_printf (psf, " Sections    :   %d\n", psf->sf.sections) ;
	psf_log_printf (psf, " Seekable    :   %s\n", psf->sf.seekable ? "TRUE" : "FALSE") ;
	
	psf_log_printf (psf, "---------------------------------\n") ;
} /* psf_dump_SFINFO */ 

sf_count_t  
psf_default_seek (SF_PRIVATE *psf, int mode, sf_count_t samples_from_start)
{	sf_count_t position, retval ;

	if (! (psf->blockwidth && psf->dataoffset >= 0))
	{	psf->error = SFE_BAD_SEEK ;
		return	((sf_count_t) -1) ;
		} ;

	position = psf->dataoffset + psf->blockwidth * samples_from_start ;

	if ((retval = psf_fseek (psf->filedes, position, SEEK_SET)) != position)
	{	psf->error = SFE_SEEK_FAILED ;
		return ((sf_count_t) -1) ;
		} ;
		
	mode = mode ;

	return samples_from_start ;
} /* psf_default_seek */

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
				
		fmaxval   = maxval / SF_MAX_COUNT ;
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
