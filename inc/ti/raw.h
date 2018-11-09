/*
 * raw.h
 */
#ifndef TI_RAW_H_
#define TI_RAW_H_

#include <qpack.h>
#include <stdlib.h>
#include <stddef.h>

typedef qp_raw_t ti_raw_t;

ti_raw_t * ti_raw_new(const unsigned char * raw, size_t n);
static inline void ti_raw_free(ti_raw_t * raw);
ti_raw_t * ti_raw_dup(const ti_raw_t * raw);
ti_raw_t * ti_raw_from_packer(qp_packer_t * packer);
ti_raw_t * ti_raw_from_ti_string(const char * src, size_t n);
char * ti_raw_to_str(const ti_raw_t * raw);
static inline _Bool ti_raw_equal(const ti_raw_t * a, const ti_raw_t * b);
static inline _Bool ti_raw_equal_strn(
        const ti_raw_t * a,
        const char * s,
        size_t n);

static inline _Bool ti_raw_equal(const ti_raw_t * a, const ti_raw_t * b)
{
    return a->n == b->n && !memcmp(a->data, b->data, a->n);
}

static inline _Bool ti_raw_equal_strn(
        const ti_raw_t * a,
        const char * s,
        size_t n)
{
    return a->n == n && !memcmp(a->data, s, n);
}

static inline void ti_raw_free(ti_raw_t * raw)
{
    free(raw);
}

#endif /* TI_RAW_H_ */
