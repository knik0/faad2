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
** $Id: sbr_huff.h,v 1.3 2002/09/29 22:19:48 menno Exp $
**/

#ifndef __SBR_HUFF_H__
#define __SBR_HUFF_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint32_t codeword;
    uint16_t index;
} sbr_huff_tab;


uint16_t sbr_huff_dec(bitfile *ld, sbr_huff_tab *t_huff);

sbr_huff_tab t_huffman_env_1_5dB[];
sbr_huff_tab f_huffman_env_1_5dB[];
sbr_huff_tab t_huffman_env_bal_1_5dB[];
sbr_huff_tab f_huffman_env_bal_1_5dB[];
sbr_huff_tab t_huffman_env_3_0dB[];
sbr_huff_tab f_huffman_env_3_0dB[];
sbr_huff_tab t_huffman_env_bal_3_0dB[];
sbr_huff_tab f_huffman_env_bal_3_0dB[];
sbr_huff_tab t_huffman_noise_3_0dB[];
sbr_huff_tab *f_huffman_noise_3_0dB;
sbr_huff_tab t_huffman_noise_bal_3_0dB[];
sbr_huff_tab *f_huffman_noise_bal_3_0dB;

#ifdef __cplusplus
}
#endif
#endif

