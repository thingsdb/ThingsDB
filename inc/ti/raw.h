/*
 * ti/raw.h
 */
#ifndef TI_RAW_H_
#define TI_RAW_H_

#include <ex.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ti/raw.t.h>
#include <tiinc.h>
#include <util/vec.h>

ti_raw_t * ti_raw_create(uint8_t tp, const void * raw, size_t n);
ti_raw_t * ti_bytes_from_base64(const void * data, size_t n);
ti_raw_t * ti_str_base64_from_raw(ti_raw_t * src);
ti_raw_t * ti_str_from_ti_string(const char * src, size_t n);
ti_raw_t * ti_str_from_fmt(const char * fmt, ...);
ti_raw_t * ti_str_from_slice(
        ti_raw_t * source,
        ssize_t start,
        ssize_t stop,
        ssize_t step);
ti_raw_t * ti_bytes_from_slice(
        ti_raw_t * source,
        ssize_t start,
        ssize_t stop,
        ssize_t step);
ti_raw_t * ti_str_from_vec(vec_t * vec);
ti_raw_t * ti_str_trim(ti_raw_t * raw);
ti_raw_t * ti_str_trim_left(ti_raw_t * raw);
ti_raw_t * ti_str_trim_right(ti_raw_t * raw);
ti_raw_t * ti_str_upper(ti_raw_t * raw);
ti_raw_t * ti_str_lower(ti_raw_t * raw);
ti_raw_t * ti_str_dval_n(size_t n);
char * ti_raw_to_str(const ti_raw_t * raw);
int ti_raw_cmp(const ti_raw_t * a, const ti_raw_t * b);
int ti_raw_cmp_strn(const ti_raw_t * a, const char * s, size_t n);
void ti_raw_set_e(ex_t * e, const ti_raw_t * r, ex_enum code);
ti_raw_t * ti_raw_cat(const ti_raw_t * a, const ti_raw_t * b);
ti_raw_t * ti_raw_cat_strn(const ti_raw_t * a, const char * s, size_t n);
ti_raw_t * ti_raw_icat_strn(const ti_raw_t * b, const char * s, size_t n);
ti_raw_t * ti_raw_cat_strn_strn(
        const char * as,
        size_t an,
        const char * bs,
        size_t bn);
_Bool ti_raw_contains(ti_raw_t * a, ti_raw_t * b);
int ti_raw_check_valid_name(ti_raw_t * raw, const char * s, ex_t * e);
int ti_raw_err_not_found(ti_raw_t * raw, const char * s, ex_t * e);
int ti_raw_printable_not_found(ti_raw_t * raw, const char * s, ex_t * e);
const char * ti_raw_as_printable_str(ti_raw_t * raw);
static inline _Bool ti_raw_startswith(ti_raw_t * a, ti_raw_t * b);
static inline _Bool ti_raw_endswith(ti_raw_t * a, ti_raw_t * b);
static inline _Bool ti_raw_eq(const ti_raw_t * a, const ti_raw_t * b);
static inline _Bool ti_raw_eq_strn(
        const ti_raw_t * a,
        const char * s,
        size_t n);


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

#endif /* TI_RAW_H_ */
