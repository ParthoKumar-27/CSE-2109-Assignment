#ifndef ERROR_H
#define ERROR_H

#include <stdint.h>
#include <stddef.h>

/*
 Error injection API for packed Manchester stream.

 The 'bits' parameter is the total number of encoded bits (enc_len * 8).
 Functions flip bits and optionally write flipped bit indices into out_positions (0-based).
 Return value: number of bits flipped (also number of entries written to out_positions up to out_max).
*/

size_t inject_single(uint8_t *buf, size_t bits, size_t *out_positions, size_t out_max);
size_t inject_two(uint8_t *buf, size_t bits, size_t *out_positions, size_t out_max);
size_t inject_odd(uint8_t *buf, size_t bits, int n, size_t *out_positions, size_t out_max);
size_t inject_burst(uint8_t *buf, size_t bits, int size, size_t *out_positions, size_t out_max);

#endif