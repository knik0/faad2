/*
** Copyright (C) 2002 Erik de Castro Lopo <erikd@zip.com.au>
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include "sndfile.h"
#include "config.h"
#include "common.h"

#if (defined (WIN32) || defined (_WIN32))

	/* Win32 has a 64 file offset seek function:
	**		__int64 _lseeki64 (int handle, __int64 offset, int origin);
	*/
	#define	EXTRA_OPEN_FLAGS	O_BINARY
	#define	SF_SEEK				_lseeki64
	#define SF_FTELL(fd)		_telli64 (fd)

#elif (defined (SIZEOF_OFF_T) && SIZEOF_OFF_T == 8)
	
	/* If sizeof (off_t) == 8 everything should just work :-). 
	** This works if the libray is being compiled as a 64 bit user space
	** application or as a 32 bit user space library on *BSD, MacOSX.
	** It will hopefully be the default on everything RealSoonNow (tm).
	*/
	#define	EXTRA_OPEN_FLAGS	0
	#define	SF_SEEK				lseek
	#define SF_FTELL(fd)		lseek ((fd), 0, SEEK_CUR)

#else
	/* Fallback position : if sizeof (off_t) == 4 will be limited to files of 
	** 2^31-1 bytes. 
	*/
	#define	EXTRA_OPEN_FLAGS	0
	#define	SF_SEEK				lseek
	#define SF_FTELL(fd)		lseek ((fd), 0, SEEK_CUR)

#endif

/*------------------------------------------------------------------------------
*/
#if (defined (WIN32) || defined (_WIN32))
#include <direct.h>

#define	_IFMT		_S_IFMT
#define _IFREG		_S_IFREG

#define	S_ISREG(m)	((m) & _S_IFREG)

#define	S_IRWXU 	0000700	/* rwx, owner */
#define		S_IRUSR	0000400	/* read permission, owner */
#define		S_IWUSR	0000200	/* write permission, owner */
#define		S_IXUSR	0000100	/* execute/search permission, owner */
#define	S_IRWXG		0000070	/* rwx, group */
#define		S_IRGRP	0000040	/* read permission, group */
#define		S_IWGRP	0000020	/* write permission, grougroup */
#define		S_IXGRP	0000010	/* execute/search permission, group */
#define	S_IRWXO		0000007	/* rwx, other */
#define		S_IROTH	0000004	/* read permission, other */
#define		S_IWOTH	0000002	/* write permission, other */
#define		S_IXOTH	0000001	/* execute/search permission, other */

#endif

/*------------------------------------------------------------------------------
*/

#define	SENSIBLE_SIZE	(0x40000000)

int
psf_open (const char *pathname, int mode)
{	int fd = -1, flags ;

	switch (mode)
	{	case SFM_READ :
				flags = O_RDONLY | EXTRA_OPEN_FLAGS ;
				fd = open (pathname, flags) ;
				break ;

		case SFM_WRITE :
				flags = O_WRONLY | O_CREAT | O_TRUNC | EXTRA_OPEN_FLAGS ;
				fd = open (pathname, flags, S_IRUSR | S_IWUSR | S_IRGRP) ;
				break ;
			
		case SFM_RDWR :
				flags = O_RDWR | O_CREAT | EXTRA_OPEN_FLAGS ;
				fd = open (pathname, flags, S_IRUSR | S_IWUSR | S_IRGRP) ;
				break ;
			
		default :
				/* psf_log_printf (psf, "ad open mode.\n") ; */
				return -1 ;
		} ;
	
	if (fd < 0)
		perror ("psf_open") ;
	
	return fd ;
} /* psf_open */

sf_count_t 
psf_fseek (int fd, sf_count_t offset, int whence)
{
	/* Bypass weird Win32-ism if necessary. 
	** _lseeki64() returns an "invalid parameter" error is called with the
	** following offset and whence parameter. Instead, use the _telli64()
	** function.
	*/
	if (OS_IS_WIN32 && offset == 0 && whence == SEEK_CUR)
		return SF_FTELL (fd) ;

	return SF_SEEK (fd, offset, whence) ;
} /* psf_fseek */

sf_count_t
psf_fread (void *ptr, sf_count_t bytes, sf_count_t items, int fd)
{	sf_count_t total = 0 ;
	size_t	 count ;
	
	items *= bytes ;

	/* Do this check after the multiplication above. */	
	if (items <= 0)
		return 0 ;
		
	while (items > 0)
	{	/* Break the writes down to a sensible size. */
		count = (items > SENSIBLE_SIZE) ? SENSIBLE_SIZE : items ;
		
		count = read (fd, ((char*) ptr) + total, count) ;
		
		if (count == ((size_t) -1) && errno == EINTR)
			continue ;

		if (count <= 0)
			break ;
			
		total += count ;
		items -= count ;
		} ;

	return total / bytes ;
} /* psf_fread */

sf_count_t
psf_fwrite (void *ptr, sf_count_t bytes, sf_count_t items, int fd)
{	sf_count_t total = 0 ;
	size_t	 count ;
	
	items *= bytes ;
	
	/* Do this check after the multiplication above. */	
	if (items <= 0)
		return 0 ;
		
	while (items > 0)
	{	/* Break the writes down to a sensible size. */
		count = (items > SENSIBLE_SIZE) ? SENSIBLE_SIZE : items ;
		
		count = write (fd, ((char*) ptr) + total, count) ;
		
		if (count == ((size_t) -1) && errno == EINTR)
			continue ;

		if (count <= 0)
			break ;
			
		total += count ;
		items -= count ;
		} ;

	return total / bytes ;
} /* psf_fwrite */

sf_count_t
psf_ftell (int fd)
{	sf_count_t pos ;

	pos = SF_FTELL (fd) ;

	/*
	if (pos < 0)
		psf_log_printf (psf, "psf_ftell error %s\n", strerror (errno)) ;
	*/

	return pos ;
} /* psf_ftell */

int
psf_fclose (int fd)
{	return close (fd) ;
} /* psf_fclose */

sf_count_t 
psf_fgets (char *buffer, sf_count_t bufsize, int fd)
{	sf_count_t	k = 0 ;
	sf_count_t		count ;

	while (k < bufsize - 1)
	{	count = read (fd, &(buffer [k]), 1) ;

	    if (count == -1 && errno == EINTR)
			continue ;
		if (count <= 0)
			break ;
		
		if (buffer [k++] == '\n')
			break ;
		}

	buffer [k] = 0 ;
	
	return k ;
} /* psf_fgets */

int 
psf_ferror (int fd)
{	
	return 0 ;
} /* psf_ferror */

void 
psf_fclearerr (int fd)
{	errno = fd ? 0 : 0 ;
} /* psf_fclearerr */

sf_count_t
psf_get_filelen (int fd)
{	struct stat statbuf ;

	if (fstat (fd, &statbuf))
	{	perror ("fstat") ;
		return -1 ;
		} ;
		
	return statbuf.st_size ;
} /* psf_fclose */

