#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "neaacdec.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  unsigned char* non_const_data = (unsigned char *)malloc(size);
  memcpy(non_const_data, data, size);
  mp4AudioSpecificConfig mp4ASC;

  NeAACDecAudioSpecificConfig(non_const_data, (unsigned long) size, &mp4ASC);
  free(non_const_data);

  return 0;
}
