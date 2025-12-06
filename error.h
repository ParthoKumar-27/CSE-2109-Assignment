#ifndef ERROR_H
#define ERROR_H

#include <stdlib.h>

void flip_bit(uint8_t *buf, size_t pos)
{
    buf[pos/8] ^= (1 << (pos % 8));
}

void inject_single(uint8_t *buf, size_t bits)
{
    flip_bit(buf, rand() % bits);
}

void inject_two(uint8_t *buf, size_t bits)
{
    size_t b1 = rand() % bits;
    size_t b2 = rand() % bits;
    while (b1 == b2)
        b2 = rand() % bits;

    flip_bit(buf, b1);
    flip_bit(buf, b2);
}

void inject_odd(uint8_t *buf, size_t bits, int n)
{
    for (int i = 0; i < n; i++)
        flip_bit(buf, rand() % bits);
}

void inject_burst(uint8_t *buf, size_t bits, int size)
{
    size_t start = rand() % (bits - size);
    for (int i = 0; i < size; i++)
        flip_bit(buf, start + i);
}

#endif
