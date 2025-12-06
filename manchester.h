#ifndef MANCHESTER_H
#define MANCHESTER_H

#include <stdint.h>
#include <stdlib.h>

// 1 → 10
// 0 → 01

uint8_t* man_encode(uint8_t *data, size_t len, size_t *out_len)
{
    *out_len = len * 16;  // 8 bits -> 16 manchester bits
    uint8_t *out = malloc(*out_len);

    for (size_t i = 0; i < len; i++)
    {
        for (int b = 7; b >= 0; b--)
        {
            int bit = (data[i] >> b) & 1;
            int pos = i * 16 + (7 - b) * 2;

            if (bit) { out[pos] = 1; out[pos+1] = 0; }
            else     { out[pos] = 0; out[pos+1] = 1; }
        }
    }
    return out;
}

uint8_t* man_decode(uint8_t *data, size_t len, size_t *out_len)
{
    *out_len = len / 16;
    uint8_t *out = malloc(*out_len);

    for (size_t i = 0; i < *out_len; i++)
    {
        uint8_t byte = 0;

        for (int b = 0; b < 8; b++)
        {
            int pos = i * 16 + b * 2;

            int first  = data[pos];
            int second = data[pos + 1];

            int bit;

            if (first == 1 && second == 0) bit = 1;
            else if (first == 0 && second == 1) bit = 0;
            else {
                // INVALID PAIR → FORCE BIT FLIP TO GUARANTEE CRC FAILS
                bit = 2;  
            }

            if (bit == 2) {  
                // Inject an impossible value so CRC definitely mismatches
                out[i] = 0xFF; 
                byte = out[i];
                break;
            }

            byte |= (bit << (7 - b));
        }

        out[i] = byte;
    }

    return out;
}


#endif
