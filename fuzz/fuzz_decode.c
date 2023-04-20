/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2005 M. Bakker, Nero AG, http://www.nero.com
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
** The "appropriate copyright message" mentioned in section 2c of the GPLv2
** must read: "Code from FAAD2 is copyright (c) Nero AG, www.nero.com"
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Nero AG through Mpeg4AAClicense@nero.com.
**/

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "neaacdec.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < 2) return 0;
  size_t first_part_size = data[0] | (data[1] << 8);
  data += 2;
  size -= 2;
  first_part_size = (first_part_size > size) ? size : first_part_size;
  size_t second_part_size = size - first_part_size;

  NeAACDecHandle decoder = NeAACDecOpen();

  unsigned char* first_part = (unsigned char *)malloc(first_part_size);
  memcpy(first_part, data, first_part_size);

  uint64_t sample_rate;
  unsigned char num_channels;
  int res =
      NeAACDecInit(decoder, first_part, first_part_size, &sample_rate, &num_channels);
  free(first_part);
  if (res != 0) {
    NeAACDecClose(decoder);
    free(first_part);
    return 0;
  }

  unsigned char* second_part = (unsigned char *)malloc(second_part_size);
  memcpy(second_part, data + first_part_size, second_part_size);

  NeAACDecFrameInfo faad_info;
  NeAACDecDecode(decoder, &faad_info, second_part, second_part_size);
  NeAACDecClose(decoder);
  free(first_part);
  free(second_part);

  return 0;
}
