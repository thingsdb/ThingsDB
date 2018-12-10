/*
 * raw.c
 */
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ti/raw.h>

ti_raw_t * ti_raw_create(const unsigned char * raw, size_t n)
{
    ti_raw_t * r = malloc(sizeof(ti_raw_t) + n);
    if (!r)
        return NULL;
    r->n = n;
    r->ref = 1;
    memcpy(r->data, raw, n);
    return r;
}

void ti_raw_drop(ti_raw_t * raw)
{
    if (raw && !--raw->ref)
        free(raw);
}

ti_raw_t * ti_raw_from_packer(qp_packer_t * packer)
{
    size_t sz = sizeof(ti_raw_t) + packer->len;
    ti_raw_t * r = malloc(sz);
    if (!r)
        return NULL;
    r->n = packer->len;
    r->ref = 1;
    memcpy(r->data, packer->buffer, packer->len);
    return r;
}

ti_raw_t * ti_raw_from_ti_string(const char * src, size_t n)
{
    assert (n >= 2);  /* at least "" or '' */

    size_t i = 0;
    size_t sz = n - 2;
    ti_raw_t * r = malloc(sizeof(ti_raw_t) + sz);
    if (!r)
        return NULL;

    /* take the first character, this is " or ' */
    char chr = *src;

    /* we need to loop till len-2 so take 2 */
    for (n -= 2; i < n; i++)
    {
        src++;
        if (*src == chr)
        {
            /* this is the character, skip one and take the next */
            src++;
            n--;
        }
        r->data[i] = *src;
    }

    r->ref = 1;
    r->n = i;

    if (r->n < sz)
    {
        ti_raw_t * tmp = realloc(r, sizeof(ti_raw_t) + r->n);
        if (tmp)
            r = tmp;
    }

    return r;
}

ti_raw_t * ti_raw_from_fmt(const char * fmt, ...)
{
    ti_raw_t * r;
    int sz;
    va_list args0, args1;
    va_start(args0, fmt);
    va_copy(args1, args0);

    sz = vsnprintf(NULL, 0, fmt, args0);
    if (sz < 0)
        return NULL;

    va_end(args0);

    r = malloc(sizeof(ti_raw_t) + sz);
    if (!r)
        goto done;

    r->ref = 1;
    r->n = (uint32_t) sz;

    (void) vsnprintf((char *) r->data, r->n, fmt, args1);

done:
    va_end(args1);
    return r;
}

ti_raw_t * ti_raw_upper(ti_raw_t * raw)
{
    char * to, * from = (char *) raw->data;
    uint32_t i = 0, n = raw->n;
    ti_raw_t * r = malloc(sizeof(ti_raw_t) + n);
    if (!r)
        return NULL;
    r->n = n;
    r->ref = 1;
    to = (char *) r->data;

    for (; i < n; ++i, ++from, ++to)
        *to = (char) toupper(*from);

    return r;
}

ti_raw_t * ti_raw_lower(ti_raw_t * raw)
{
    char * to, * from = (char *) raw->data;
    uint32_t i = 0, n = raw->n;
    ti_raw_t * r = malloc(sizeof(ti_raw_t) + n);
    if (!r)
        return NULL;
    r->n = n;
    r->ref = 1;
    to = (char *) r->data;

    for (; i < n; ++i, ++from, ++to)
        *to = (char) tolower(*from);

    return r;
}

char * ti_raw_to_str(const ti_raw_t * raw)
{
    char * str = malloc(raw->n + 1);
    if (!str)
        return NULL;
    memcpy(str, raw->data, raw->n);
    str[raw->n] = '\0';
    return str;
}

int ti_raw_cmp(const ti_raw_t * a, const ti_raw_t * b)
{
    int rc = memcmp(a->data, b->data, a->n < b->n ? a->n : b->n);
    return (rc || a->n == b->n) ? rc : (a->n > b->n
            ? (int) a->data[b->n]
            : -((int) b->data[a->n])
    );
}

int ti_raw_cmp_strn(const ti_raw_t * a, const char * s, size_t n)
{
    int rc = memcmp(a->data, s, a->n < n ? a->n : n);
    return (rc || a->n == n) ? rc : (a->n > n
            ? (int) a->data[n]
            : -((int) ((unsigned char) s[a->n]))
    );
}

ti_raw_t * ti_raw_cat(const ti_raw_t * a, const ti_raw_t * b)
{
    size_t n = a->n + b->n;
    ti_raw_t * r = malloc(sizeof(ti_raw_t) + n);
    if (!r)
        return NULL;
    r->n = n;
    r->ref = 1;
    memcpy(r->data, a->data, a->n);
    memcpy(r->data + a->n, b->data, b->n);
    return r;
}

ti_raw_t * ti_raw_cat_strn(const ti_raw_t * a, const char * s, size_t n)
{
    size_t nn = a->n + n;
    ti_raw_t * r = malloc(sizeof(ti_raw_t) + nn);
    if (!r)
        return NULL;
    r->n = nn;
    r->ref = 1;
    memcpy(r->data, a->data, a->n);
    memcpy(r->data + a->n, s, n);
    return r;
}

ti_raw_t * ti_raw_icat_strn(const ti_raw_t * b, const char * s, size_t n)
{
    size_t nn = n + b->n;
    ti_raw_t * r = malloc(sizeof(ti_raw_t) + nn);
    if (!r)
        return NULL;
    r->n = nn;
    r->ref = 1;
    memcpy(r->data, s, n);
    memcpy(r->data + n, b->data, b->n);
    return r;
}

ti_raw_t * ti_raw_cat_strn_strn(
        const char * as,
        size_t an,
        const char * bs,
        size_t bn)
{
    size_t nn = an + bn;
    ti_raw_t * r = malloc(sizeof(ti_raw_t) + nn);
    if (!r)
        return NULL;
    r->n = nn;
    r->ref = 1;
    memcpy(r->data, as, an);
    memcpy(r->data + an, bs, bn);
    return r;
}
