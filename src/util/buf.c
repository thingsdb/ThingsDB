/*
 * util/buf.h
 */
#include <util/logger.h>
#include <util/buf.h>
#include <stdlib.h>
#include <string.h>

void buf_init(buf_t * buf)
{
    buf->cap = 0;
    buf->len = 0;
    buf->data = NULL;
}

int buf_append(buf_t * buf, const char * s, size_t n)
{
    if (buf->len + n > buf->cap)
    {
        char * tmp;
        size_t nsize = buf->cap ? buf->cap * 2 : 8192;

        while(nsize < buf->cap + n)
            nsize *= 2;


        tmp = realloc(buf->data, nsize);
        if (!tmp)
            return -1;
        buf->data = tmp;
        buf->cap = nsize;
    }

    memcpy(buf->data + buf->len, s, n);
    buf->len += n;

    return 0;
}
