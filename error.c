// error.c
// Bit-level error injection for packed Manchester stream

#include "error.h"
#include <stdlib.h>
#include <stdint.h>

static void flip_encoded_bit(uint8_t *buf, size_t bitpos)
{
    size_t byte_idx = bitpos / 8;
    int bit_pos = 7 - (int)(bitpos % 8);
    buf[byte_idx] ^= (uint8_t)(1u << bit_pos);
}

size_t inject_single(uint8_t *buf, size_t bits, size_t *out_positions, size_t out_max)
{
    if (bits == 0) return 0;
    size_t p = (size_t)rand() % bits;
    flip_encoded_bit(buf, p);
    if (out_positions && out_max > 0) out_positions[0] = p;
    return 1;
}

size_t inject_two(uint8_t *buf, size_t bits, size_t *out_positions, size_t out_max)
{
    if (bits == 0) return 0;
    if (bits == 1) {
        flip_encoded_bit(buf, 0);
        if (out_positions && out_max > 0) out_positions[0] = 0;
        return 1;
    }
    size_t b1 = (size_t)rand() % bits;
    size_t b2 = (size_t)rand() % bits;
    while (b2 == b1) b2 = (size_t)rand() % bits;
    flip_encoded_bit(buf, b1);
    flip_encoded_bit(buf, b2);
    if (out_positions && out_max > 0) out_positions[0] = b1;
    if (out_positions && out_max > 1) out_positions[1] = b2;
    return 2;
}

size_t inject_odd(uint8_t *buf, size_t bits, int n, size_t *out_positions, size_t out_max)
{
    if (bits == 0 || n <= 0) return 0;
    size_t written = 0;
    for (int i = 0; i < n; i++) {
        size_t p = (size_t)rand() % bits;
        flip_encoded_bit(buf, p);
        if (out_positions && written < out_max) out_positions[written] = p;
        written++;
    }
    return written;
}

size_t inject_burst(uint8_t *buf, size_t bits, int size, size_t *out_positions, size_t out_max)
{
    if (bits == 0 || size <= 0) return 0;
    if ((size_t)size >= bits) {
        size_t written = 0;
        for (size_t i = 0; i < bits; i++) {
            flip_encoded_bit(buf, i);
            if (out_positions && written < out_max) out_positions[written] = i;
            written++;
        }
        return written;
    }
    size_t start = (size_t)rand() % (bits - (size_t)size + 1);
    size_t written = 0;
    for (int i = 0; i < size; i++) {
        size_t p = start + (size_t)i;
        flip_encoded_bit(buf, p);
        if (out_positions && written < out_max) out_positions[written] = p;
        written++;
    }
    return written;
}