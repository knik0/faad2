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
** $Id: data.c,v 1.2 2002/02/18 10:01:05 menno Exp $
**/

#include "common.h"
#include "data.h"

extern uint8_t num_swb_long_window[] =
{
    41, 41, 47, 49, 49, 51, 47, 47, 43, 43, 43, 40
};

extern uint8_t num_swb_short_window[] =
{
    12, 12, 12, 14, 14, 14, 15, 15, 15, 15, 15, 15
};

static uint16_t swb_offset_long_96[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56,
    64, 72, 80, 88, 96, 108, 120, 132, 144, 156, 172, 188, 212, 240,
    276, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960, 1024
};

static uint16_t swb_offset_short_96[] =
{
    0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128
};

static uint16_t swb_offset_long_64[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56,
    64, 72, 80, 88, 100, 112, 124, 140, 156, 172, 192, 216, 240, 268,
    304, 344, 384, 424, 464, 504, 544, 584, 624, 664, 704, 744, 784, 824,
    864, 904, 944, 984, 1024
};

static uint16_t swb_offset_short_64[] =
{
    0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128
};


static uint16_t swb_offset_long_48[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72,
    80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292,
    320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736,
    768, 800, 832, 864, 896, 928, 1024
};

static uint16_t swb_offset_short_48[] =
{
    0, 4, 8, 12, 16, 20, 28, 36, 44, 56, 68, 80, 96, 112, 128
};

static uint16_t swb_offset_long_32[] =
{
    0, 4,  8,  12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72,
    80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292,
    320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736,
    768, 800, 832, 864, 896, 928, 960, 992, 1024
};

static uint16_t swb_offset_long_24[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 52, 60, 68,
    76, 84, 92, 100, 108, 116, 124, 136, 148, 160, 172, 188, 204, 220,
    240, 260, 284, 308, 336, 364, 396, 432, 468, 508, 552, 600, 652, 704,
    768, 832, 896, 960, 1024
};

static uint16_t swb_offset_short_24[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 108, 128
};

static uint16_t swb_offset_long_16[] =
{
    0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 100, 112, 124,
    136, 148, 160, 172, 184, 196, 212, 228, 244, 260, 280, 300, 320, 344,
    368, 396, 424, 456, 492, 532, 572, 616, 664, 716, 772, 832, 896, 960, 1024
};

static uint16_t swb_offset_short_16[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 60, 72, 88, 108, 128
};

static uint16_t swb_offset_long_8[] =
{
    0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 172,
    188, 204, 220, 236, 252, 268, 288, 308, 328, 348, 372, 396, 420, 448,
    476, 508, 544, 580, 620, 664, 712, 764, 820, 880, 944, 1024
};

static uint16_t swb_offset_short_8[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 60, 72, 88, 108, 128
};

extern uint16_t *swb_offset_long_window[] =
{
    swb_offset_long_96,      /* 96000 */
    swb_offset_long_96,      /* 88200 */
    swb_offset_long_64,      /* 64000 */
    swb_offset_long_48,      /* 48000 */
    swb_offset_long_48,      /* 44100 */
    swb_offset_long_32,      /* 32000 */
    swb_offset_long_24,      /* 24000 */
    swb_offset_long_24,      /* 22050 */
    swb_offset_long_16,      /* 16000 */
    swb_offset_long_16,      /* 12000 */
    swb_offset_long_16,      /* 11025 */
    swb_offset_long_8        /* 8000  */
};

extern uint16_t *swb_offset_short_window[] =
{
    swb_offset_short_96,      /* 96000 */
    swb_offset_short_96,      /* 88200 */
    swb_offset_short_64,      /* 64000 */
    swb_offset_short_48,      /* 48000 */
    swb_offset_short_48,      /* 44100 */
    swb_offset_short_48,      /* 32000 */
    swb_offset_short_24,      /* 24000 */
    swb_offset_short_24,      /* 22050 */
    swb_offset_short_16,      /* 16000 */
    swb_offset_short_16,      /* 12000 */
    swb_offset_short_16,      /* 11025 */
    swb_offset_short_8        /* 8000  */
};

extern uint8_t pred_sfb_max[] =
{
    33,     /* 96000 */
    33,     /* 88200 */
    38,     /* 64000 */
    40,     /* 48000 */
    40,     /* 44100 */
    40,     /* 32000 */
    41,     /* 24000 */
    41,     /* 22050 */
    37,     /* 16000 */
    37,     /* 12000 */
    37,     /* 11025 */
    34      /* 8000  */
};

extern uint32_t sample_rates[] =
{
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000,
    12000, 11025, 8000
};