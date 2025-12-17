// manchester.c

#include "manchester.h"
#include <stdlib.h>
#include <string.h>

static void set_bit(uint8_t *buf, size_t bit_index, int value)
{
    size_t byte_idx = bit_index / 8;
    int bit_pos = 7 - (int)(bit_index % 8); // MSB-first
    if (value)
        buf[byte_idx] |= (uint8_t)(1u << bit_pos);
    else
        buf[byte_idx] &= (uint8_t)~(1u << bit_pos);
}

static int get_bit(const uint8_t *buf, size_t bit_index)
{
    size_t byte_idx = bit_index / 8;
    int bit_pos = 7 - (int)(bit_index % 8);
    return (buf[byte_idx] >> bit_pos) & 1;
}

uint8_t* man_encode(const uint8_t *data, size_t len, size_t *out_len)
{
    if (!data || len == 0) {
        *out_len = 0;
        return NULL;
    }

    *out_len = len * 2;
    uint8_t *out = malloc(*out_len);
    if (!out) return NULL;
    memset(out, 0, *out_len);

    size_t out_bit = 0;
    for (size_t i = 0; i < len; i++) {
        for (int b = 7; b >= 0; b--) {
            int bit = (data[i] >> b) & 1;
            if (bit) {
                // 1 -> 1,0
                set_bit(out, out_bit++, 1);
                set_bit(out, out_bit++, 0);
            } else {
                // 0 -> 0,1
                set_bit(out, out_bit++, 0);
                set_bit(out, out_bit++, 1);
            }
        }
    }
    return out;
}

uint8_t* man_decode(const uint8_t *data, size_t len, size_t *out_len)
{
    if (!data || len == 0) {
        *out_len = 0;
        return NULL;
    }

    *out_len = len / 2; // floor
    uint8_t *out = malloc(*out_len);
    if (!out) return NULL;
    memset(out, 0, *out_len);

    for (size_t i = 0; i < *out_len; i++) {
        uint8_t byte = 0;
        size_t base_bit = i * 16;
        int invalid = 0;
        for (int b = 0; b < 8; b++) {
            int first = get_bit(data, base_bit + b * 2);
            int second = get_bit(data, base_bit + b * 2 + 1);

            int bit;
            if (first == 1 && second == 0) bit = 1;
            else if (first == 0 && second == 1) bit = 0;
            else { invalid = 1; break; }

            byte |= (uint8_t)(bit << (7 - b));
        }

        if (invalid) out[i] = 0xFF; // mark invalid byte to cause CRC mismatch
        else out[i] = byte;
    }

    return out;
}