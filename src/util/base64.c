#include <stdlib.h>

static const int base64__idx[256] = {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0, 62, 63, 62, 62, 63, 52, 53, 54, 55, 56, 57,
        58, 59, 60, 61,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,
        7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
        25,  0, 0,  0,  0, 63,  0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
        37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
};

char * base64_decode(const void * data, const size_t len)
{
    const unsigned char * p = data;
    int pad = len > 0 && (len % 4 || p[len - 1] == '=');
    size_t i, j;
    const size_t L = ((len + 3) / 4 - pad) * 4;
    char * str = malloc(L / 4 * 3 + pad);
    if (!str)
        return NULL;

    for (i = 0, j = 0; i < L; i += 4)
    {
        int n = base64__idx[p[i]] << 18 | \
                base64__idx[p[i + 1]] << 12 | \
                base64__idx[p[i + 2]] << 6 | \
                base64__idx[p[i + 3]];
        str[j++] = n >> 16;
        str[j++] = n >> 8 & 0xFF;
        str[j++] = n & 0xFF;
    }

    if (pad)
    {
        int n = base64__idx[p[L]] << 18 | base64__idx[p[L + 1]] << 12;
        str[j++] = n >> 16;

        if (len > L + 2 && p[L + 2] != '=')
        {
            n |= base64__idx[p[L + 2]] << 6;
            str[j++] = n >> 8 & 0xFF;
        }
    }

    str[j] = '\0';
    return str;
}

static const unsigned char base64__table[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

unsigned char * base64_encode(const unsigned char * src, const size_t len, size_t * n)
{
    unsigned char * out, * pos;
    const unsigned char * end, * in;
    size_t olen = 4 * ((len + 2) / 3); /* 3-byte blocks to 4-byte */

    if (olen < len || !(out = malloc(olen)))
        /* integer overflow or allocation error */
        return NULL;

    end = src + len;
    in = src;
    pos = out;

    while (end - in >= 3)
    {
        *pos++ = base64__table[in[0] >> 2];
        *pos++ = base64__table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
        *pos++ = base64__table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
        *pos++ = base64__table[in[2] & 0x3f];
        in += 3;
    }

    if (end - in)
    {
        *pos++ = base64__table[in[0] >> 2];
        if (end - in == 1)
        {
            *pos++ = base64__table[(in[0] & 0x03) << 4];
            *pos++ = '=';
        }
        else
        {
            *pos++ = base64__table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
            *pos++ = base64__table[(in[1] & 0x0f) << 2];
        }
        *pos++ = '=';
    }

    *n = pos - out;
    return out;
}
