#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "neaacdec.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  NeAACDecHandle decoder = NeAACDecOpen();

  unsigned char* non_const_data = (unsigned char *)malloc(size);
  memcpy(non_const_data, data, size);

  uint64_t sample_rate;
  unsigned char num_channels;
  int res =
      NeAACDecInit(decoder, non_const_data, size, &sample_rate, &num_channels);
  if (res != 0) {
    NeAACDecClose(decoder);
    free(non_const_data);
    return 0;
  }

  NeAACDecFrameInfo faad_info;
  NeAACDecDecode(decoder, &faad_info, non_const_data, size);
  NeAACDecClose(decoder);
  free(non_const_data);

  return 0;
}
