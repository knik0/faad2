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

#ifndef SF_COMMAND_H_INCLUDED
#define SF_COMMAND_H_INCLUDED

/* Command values for sf_command (). These are obtained using the Python
** script sf_command.py in the top level directory of the libsndfile sources.
*/
enum
{
	SFC_GET_LIB_VERSION	= 0x124A0C,	/* "get-lib-version" */
	SFC_GET_LOG_INFO	= 0x248A3,	/* "get-log-info" */
	SFC_GET_NORM_DOUBLE	= 0x12692D,	/* "get-norm-double" */
	SFC_GET_NORM_FLOAT	= 0x934EA,	/* "get-norm-float" */
	SFC_SET_NORM_DOUBLE	= 0x17692D,	/* "set-norm-double" */
	SFC_SET_NORM_FLOAT	= 0xBB4EA,	/* "set-norm-float" */
	SFC_GET_SIMPLE_COUNT	= 0x12145914,	/* "get-simple-format-count" */
	SFC_GET_SIMPLE_FORMAT	= 0x485162,	/* "get-simple-format" */
	SFC_TRUE	= 0x2E7,	/* "true" */
	SFC_FALSE	= 0x45B		/* "false" */
} ;

#endif /* SF_COMMAND_H_INCLUDED */


