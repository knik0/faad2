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

/* On Intel Pentium processors (especially PIII and probably P4), converting
** from float to int is very slow. To meet the C specs, the code produced by 
** most C compilers targeting Pentium needs to change the FPU rounding mode 
** before the float to int conversion is performed. 
**
** Changing the FPU rounding mode causes the FPU pipeline to be flushed. It 
** is this flushing of the pipeline which is so slow.
*/


/* These macros are place holders for inline functions which will replace 
** them in the near future.
*/

#define	FLOAT_TO_INT(x)		((int)(x))
#define	FLOAT_TO_SHORT(x)	((short)(x))

#define	DOUBLE_TO_INT(x)	((int)(x))
#define	DOUBLE_TO_SHORT(x)	((short)(x))
