/*
** FAAD - Freeware Advanced Audio Decoder
** Copyright (C) 2002 M. Bakker
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** $Id: common.c,v 1.2 2002/08/27 10:24:54 menno Exp $
**/

/* just some common functions that could be used anywhere */

#include "common.h"

#define LOG2 0.30102999566398

#if 0
uint32_t int_log2(uint32_t val)
{
    return (uint32_t)(log((real_t)val)/LOG2 + 0.5);
}
#endif

