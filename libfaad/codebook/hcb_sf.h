/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2004 M. Bakker, Ahead Software AG, http://www.nero.com
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
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: hcb_sf.h,v 1.4 2004/01/20 18:42:51 menno Exp $
**/

/* Binary search huffman table HCB_SF */

ALIGN static const int8_t hcb_sf[][2] = {
    { -61, 1 }, { 2, 3 }, { -62, 4 }, { 5, 6 }, { -60, -63 }, { -59, 7 },
    { 8, 9 }, { -64, -58 }, { 10, 11 }, { 12, 13 }, { -65, -57 }, { -66, -56 },
    { 14, 15 }, { 16, 17 }, { -55, -67 }, { -54, 18 }, { 19, 20 }, { 21, 22 },
    { -68, -53 }, { -69, -52 }, { -70, 23 }, { 24, 25 }, { 26, 27 }, { -51, -71 },
    { -72, -50 }, { 28, 29 }, { 30, 31 }, { 32, 33 }, { -49, -73 }, { -48, -74 },
    { -47, -75 }, { 34, 35 }, { 36, 37 }, { 38, 39 }, { -45, -46 }, { -44, -43 },
    { -76, -78 }, { 40, 41 }, { 42, 43 }, { 44, 45 }, { -77, -42 }, { -79, -80 },
    { -41, -81 }, { 46, 47 }, { 48, 49 }, { 50, 51 }, { -40, -82 }, { -39, -83 },
    { -38, 52 }, { 53, 54 }, { 55, 56 }, { 57, 58 }, { -84, -86 }, { -36, -88 },
    { -85, -87 }, { -37, -89 }, { 59, 60 }, { 61, 62 }, { 63, 64 }, { -34, -32 },
    { -91, -90 }, { 65, 66 }, { 67, 68 }, { 69, 70 }, { 71, 72 }, { -35, -92 },
    { -95, -94 }, { -93, -97 }, { -33, 73 }, { 74, 75 }, { 76, 77 }, { 78, 79 },
    { 80, 81 }, { -96, -99 }, { -98, 82 }, { 83, 84 }, { 85, 86 }, { 87, 88 },
    { 89, 90 }, { 91, 92 }, { 93, 94 }, { 95, 96 }, { -31, -100 }, { -102, -118 },
    { -120, -119 }, { -121, 97 }, { 98, 99 }, { 100, 101 }, { 102, 103 },
    { 104, 105 }, { 106, 107 }, { 108, 109 }, { 110, 111 }, { 112, 113 },
    { 114, 115 }, { 116, 117 }, { 118, 119 }, { -23, -22 }, { -21, -20 },
    { -19, -4 }, { -24, -30 }, { -29, -28 }, { -27, -26 }, { -25, -17 },
    { -10, -9 }, { -8, -7 }, { -6, -5 }, { -11, -16 }, { -15, -14 }, { -13, -12 },
    { -3, -115 }, { -113, -112 }, { -111, -116 }, { -18, -1 }, { -2, -117 },
    { -114, -106 }, { -105, -103 }, { -101, -104 }, { -110, -109 }
};
