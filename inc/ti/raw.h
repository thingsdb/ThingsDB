/*
 * ti/raw.h
 */
#ifndef TI_RAW_H_
#define TI_RAW_H_

#include <qpack.h>
#include <stdlib.h>
#include <stddef.h>
#include <ti/name.h>

typedef struct ti_raw_s ti_raw_t;
typedef unsigned char uchar;

ti_raw_t * ti_raw_create(const unsigned char * raw, size_t n);
ti_raw_t * ti_raw_from_packer(qp_packer_t * packer);
ti_raw_t * ti_raw_from_ti_string(const char * src, size_t n);
ti_raw_t * ti_raw_from_fmt(const char * fmt, ...);
ti_raw_t * ti_raw_from_strn(const char * str, size_t n);
ti_raw_t * ti_raw_upper(ti_raw_t * raw);
ti_raw_t * ti_raw_lower(ti_raw_t * raw);
char * ti_raw_to_str(const ti_raw_t * raw);
int ti_raw_cmp(const ti_raw_t * a, const ti_raw_t * b);
int ti_raw_cmp_strn(const ti_raw_t * a, const char * s, size_t n);
ti_raw_t * ti_raw_cat(const ti_raw_t * a, const ti_raw_t * b);
ti_raw_t * ti_raw_cat_strn(const ti_raw_t * a, const char * s, size_t n);
ti_raw_t * ti_raw_icat_strn(const ti_raw_t * b, const char * s, size_t n);
ti_raw_t * ti_raw_cat_strn_strn(
        const char * as,
        size_t an,
        const char * bs,
        size_t bn);
_Bool ti_raw_contains(ti_raw_t * a, ti_raw_t * b);
int ti_raw_err_name_not_found(ti_raw_t * raw, const char * s, ex_t * e);
static inline _Bool ti_raw_startswith(ti_raw_t * a, ti_raw_t * b);
static inline _Bool ti_raw_endswith(ti_raw_t * a, ti_raw_t * b);
static inline _Bool ti_raw_eq(const ti_raw_t * a, const ti_raw_t * b);
static inline _Bool ti_raw_eq_strn(
        const ti_raw_t * a,
        const char * s,
        size_t n);
static inline int ti_raw_err_not_found(
        ti_raw_t * raw,
        const char * s,
        ex_t * e);

struct ti_raw_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _flags;
    uint16_t _pad0;
    uint32_t n;
    uint32_t _pad1;
    unsigned char data[];
};

static inline _Bool ti_raw_eq(const ti_raw_t * a, const ti_raw_t * b)
{
    return a == b || (a->n == b->n && !memcmp(a->data, b->data, a->n));
}

static inline _Bool ti_raw_eq_strn(
        const ti_raw_t * a,
        const char * s,
        size_t n)
{
    return a->n == n && !memcmp(a->data, s, n);
}

static inline _Bool ti_raw_startswith(ti_raw_t * a, ti_raw_t * b)
{
    return a->n >= b->n && memcmp(a->data, b->data, b->n) == 0;
}

static inline _Bool ti_raw_endswith(ti_raw_t * a, ti_raw_t * b)
{
    return a->n >= b->n && memcmp(a->data + a->n - b->n, b->data, b->n) == 0;
}

static inline int ti_raw_err_not_found(
        ti_raw_t * raw,
        const char * s,
        ex_t * e)
{
    if (ti_name_is_valid_strn((const char *) raw->data, raw->n))
        ex_set(e, EX_INDEX_ERROR,
                "%s `%.*s` not found",
                s, (int) raw->n, (const char *) raw->data);
    else
        ex_set(e, EX_BAD_DATA,
                "%s name must follow the naming rules"
                TI_SEE_DOC("#names"),
                s);
    return e->nr;
}

#endif /* TI_RAW_H_ */
