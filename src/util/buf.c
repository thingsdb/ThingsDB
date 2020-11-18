/*
 * util/buf.h
 */
#include <util/logger.h>
#include <util/buf.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void buf_init(buf_t * buf)
{
    buf->cap = 0;
    buf->len = 0;
    buf->data = NULL;
}

int buf_append(buf_t * buf, const char * s, size_t n)
{
    size_t rsize = buf->len + n;

    if (rsize > buf->cap)
    {
        char * tmp;
        size_t nsize = buf->cap ? buf->cap << 1 : 8192;

        while(rsize > nsize)
            nsize <<= 1;

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

int buf_write(buf_t * buf, const char c)
{
    if (buf->len == buf->cap)
    {
        char * tmp;
        size_t nsize = buf->cap ? buf->cap << 1 : 8192;

        tmp = realloc(buf->data, nsize);
        if (!tmp)
            return -1;

        buf->data = tmp;
        buf->cap = nsize;
    }

    buf->data[buf->len++] = c;
    return 0;
}

int buf_append_fmt(buf_t * buf, const char * fmt, ...)
{
    int rc;
    int nchars;
    va_list args, args_cp;
    size_t cap = buf->cap - buf->len;

    va_start(args, fmt);
    va_copy(args_cp, args);

    nchars = vsnprintf(buf->data + buf->len, cap, fmt, args);

    /*
     * Must test for >= cap because vsnprintf() will always write the \0
     * character to end the string, also when not enough space exists in the
     * buffer. Therefore the buffer must have enough space to also contain the
     * last terminator character.
     */
    rc = nchars < 0 || (size_t) nchars >= cap;  /* 0 (false) = success */

    if (rc)
    {
        char * tmp;
        size_t nsize = buf->cap ? buf->cap << 1 : 8192;

        while(buf->len + (size_t) nchars > nsize)
            nsize <<= 1;

        tmp = realloc(buf->data, nsize);
        if (!tmp)
            goto done;

        buf->data = tmp;
        buf->cap = nsize;

        cap = buf->cap - buf->len;
        nchars = vsnprintf(buf->data + buf->len, cap, fmt, args_cp);

        rc = nchars < 0 || (size_t) nchars >= cap;  /* 0 (false) = success */
    }

    if (rc == 0)
        buf->len += nchars;

done:
    va_end(args);
    va_end(args_cp);

    return rc;
}
