/*
 * ti/raw.c
 */
#define _GNU_SOURCE
#include <assert.h>
#include <ctype.h>
#include <doc.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ti/name.h>
#include <ti/raw.h>
#include <ti/val.h>
#include <util/logger.h>

static const int base64__idx[256] = {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0, 62, 63, 62, 62, 63, 52, 53, 54, 55, 56, 57,
        58, 59, 60, 61,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,
        7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
        25,  0, 0,  0,  0, 63,  0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
        37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
};

ti_raw_t * ti_raw_create(uint8_t tp, const void * raw, size_t n)
{
    ti_raw_t * r = malloc(sizeof(ti_raw_t) + n);
    if (!r)
        return NULL;
    r->ref = 1;
    r->tp = tp;
    r->n = n;
    memcpy(r->data, raw, n);
    return r;
}

void ti_raw_init(ti_raw_t * raw, uint8_t tp, size_t total_n)
{
    raw->ref = 1;
    raw->tp = tp;
    raw->n = total_n - sizeof(ti_raw_t);
}

ti_raw_t * ti_bytes_from_base64(const void * data, size_t n)
{
    const unsigned char * p = data;
    char * out;
    int pad = n > 0 && (n % 4 || p[n - 1] == '=');
    size_t i, j, L = ((n + 3) / 4 - pad) * 4;
    ti_raw_t * r = malloc(sizeof(ti_raw_t) + (L / 4 * 3) + (pad
            ? (n > 1 && (n % 4 == 3 || p[n - 2] != '=')) + 1
            : 0));
    if (!r)
        return NULL;

    out = (char *) r->data;

    for (i = 0, j = 0; i < L; i += 4)
    {
        int nn = base64__idx[p[i]] << 18 | \
                base64__idx[p[i + 1]] << 12 | \
                base64__idx[p[i + 2]] << 6 | \
                base64__idx[p[i + 3]];
        out[j++] = nn >> 16;
        out[j++] = nn >> 8 & 0xFF;
        out[j++] = nn & 0xFF;
    }

    if (pad)
    {
        int nn = base64__idx[p[L]] << 18 | base64__idx[p[L + 1]] << 12;
        out[j++] = nn >> 16;

        if (n > L + 2 && p[L + 2] != '=')
        {
            nn |= base64__idx[p[L + 2]] << 6;
            out[j++] = nn >> 8 & 0xFF;
        }
    }

    r->ref = 1;
    r->tp = TI_VAL_BYTES;
    r->n = j;

    return r;
}

ti_raw_t * ti_str_from_ti_string(const char * src, size_t n)
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
    r->tp = TI_VAL_STR;
    r->n = i;

    if (r->n < sz)
    {
        ti_raw_t * tmp = realloc(r, sizeof(ti_raw_t) + r->n);
        if (tmp)
            r = tmp;
    }

    return r;
}

ti_raw_t * ti_str_from_fmt(const char * fmt, ...)
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

    r = malloc(sizeof(ti_raw_t) + sz + 1);
    if (!r)
        goto done;

    r->ref = 1;
    r->tp = TI_VAL_STR;
    r->n = (uint32_t) sz;

    (void) vsnprintf((char *) r->data, r->n + 1, fmt, args1);

done:
    va_end(args1);
    return r;
}

ti_raw_t * ti_raw_from_slice(
        ti_raw_t * source,
        ssize_t start,
        ssize_t stop,
        ssize_t step)
{
    ti_raw_t * raw;
    uchar * dest;
    uchar * from;
    ssize_t n = stop - start;

    n = n / step + !!(n % step);

    if (n <= 0)
        return (ti_raw_t *) ti_val_empty_str();

    raw = malloc(sizeof(ti_raw_t) + n);
    if (!raw)
        return NULL;

    raw->ref = 1;
    raw->tp = source->tp;
    raw->n = n;

    dest = raw->data;
    from = source->data + start;

    *dest = *from;

    while (--n)
        *(++dest) = *(from += step);

    return raw;
}

ti_raw_t * ti_str_upper(ti_raw_t * raw)
{
    char * to, * from = (char *) raw->data;
    uint32_t i = 0, n = raw->n;
    ti_raw_t * r = malloc(sizeof(ti_raw_t) + n);
    if (!r)
        return NULL;

    r->ref = 1;
    r->tp = TI_VAL_STR;
    r->n = n;
    to = (char *) r->data;

    for (; i < n; ++i, ++from, ++to)
        *to = (char) toupper(*from);

    return r;
}

ti_raw_t * ti_str_lower(ti_raw_t * raw)
{
    char * to, * from = (char *) raw->data;
    uint32_t i = 0, n = raw->n;
    ti_raw_t * r = malloc(sizeof(ti_raw_t) + n);
    if (!r)
        return NULL;

    r->ref = 1;
    r->tp = TI_VAL_STR;
    r->n = n;
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

void ti_raw_to_e(const ti_raw_t * r, ex_t * e, ex_enum code)
{
    assert(r->n < EX_MAX_SZ);
    e->n = r->n;
    e->nr = code;
    memcpy(e->msg, r->data, r->n);
    e->msg[e->n] = '\0';
}

ti_raw_t * ti_raw_cat(const ti_raw_t * a, const ti_raw_t * b)
{
    size_t n = a->n + b->n;
    ti_raw_t * r = malloc(sizeof(ti_raw_t) + n);
    if (!r)
        return NULL;

    r->ref = 1;
    r->tp = a->tp == TI_VAL_BYTES || b->tp == TI_VAL_BYTES
            ? TI_VAL_BYTES
            : TI_VAL_STR;
    r->n = n;
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

    r->ref = 1;
    r->tp = a->tp;
    r->n = nn;
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

    r->ref = 1;
    r->tp = b->tp;
    r->n = nn;
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

    r->ref = 1;
    r->tp = TI_VAL_STR;
    r->n = nn;
    memcpy(r->data, as, an);
    memcpy(r->data + an, bs, bn);
    return r;
}

_Bool ti_raw_contains(ti_raw_t * a, ti_raw_t * b)
{
    /* memmem requires _GNU_SOURCE */
    return !!memmem(a->data, a->n, b->data, b->n);
}

int ti_raw_check_valid_name(ti_raw_t * raw, const char * s, ex_t * e)
{
    if (!ti_name_is_valid_strn((const char *) raw->data, raw->n))
        ex_set(e, EX_VALUE_ERROR,
                "%s name must follow the naming rules"DOC_NAMES,
                s);
    return e->nr;
}

int ti_raw_err_not_found(ti_raw_t * raw, const char * s, ex_t * e)
{
    if (!ti_raw_check_valid_name(raw, s, e))
        ex_set(e, EX_LOOKUP_ERROR,
                "%s `%.*s` not found",
                s, (int) raw->n, (const char *) raw->data);
    return e->nr;
}

