/*
 * util/rbuf.h
 *
 * Write data in reverse order to a buffer.
 */
#include <util/logger.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <util/rbuf.h>

void rbuf_init(rbuf_t * buf)
{
    buf->pos = 0;
    buf->cap = 0;
    buf->data = NULL;
}

int rbuf_append(rbuf_t * buf, const char * s, size_t n)
{
    if (n > buf->pos)
    {
        char * tmp;
        size_t nsize = buf->cap ? buf->cap << 1 : 8192;
        size_t len = rbuf_len(buf);
        size_t rsize = len + n;

        while(rsize > nsize)
            nsize <<= 1;

        tmp = malloc(nsize);
        if (!tmp)
            return -1;

        buf->pos = nsize - len;
        buf->cap = nsize;

        memcpy(tmp + buf->pos, buf->data, len);

        free(buf->data);
        buf->data = tmp;
    }

    buf->pos -= n;
    memcpy(buf->data + buf->pos, s, n);

    return 0;
}

int rbuf_write(rbuf_t * buf, const char c)
{
    if (!buf->pos)
    {
        char * tmp;
        size_t nsize = buf->cap ? buf->cap << 1 : 8192;
        size_t len = rbuf_len(buf);

        /* this used to be a re-allocation but we need new memory, bug #253 */
        tmp = malloc(nsize);
        if (!tmp)
            return -1;

        buf->pos = nsize - len;
        buf->cap = nsize;

        memcpy(tmp + buf->pos, buf->data, len);

        free(buf->data);
        buf->data = tmp;
    }

    buf->data[--buf->pos] = c;
    return 0;
}


