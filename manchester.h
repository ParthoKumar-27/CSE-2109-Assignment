#ifndef MANCHESTER_H
#define MANCHESTER_H

#include <stdint.h>
#include <stddef.h>

/*
 Packed Manchester encoder/decoder.

 Mapping:
  - data bit 1 -> encoded pair 1,0
  - data bit 0 -> encoded pair 0,1

 Each input byte (8 bits) => 16 encoded bits => 2 bytes.
 out_len (bytes) = in_len * 2

 Functions return malloc'd buffers. Caller must free.
*/

uint8_t* man_encode(const uint8_t *data, size_t len, size_t *out_len);
uint8_t* man_decode(const uint8_t *data, size_t len, size_t *out_len);

#endif