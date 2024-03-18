/*
 * ti/regex.t.h
 */
#ifndef TI_REGEX_T_H_
#define TI_REGEX_T_H_

#define PCRE2_CODE_UNIT_WIDTH 8

typedef struct ti_regex_s ti_regex_t;

#include <pcre2.h>
#include <stddef.h>
#include <ti/raw.h>
#include <ti/raw.t.h>
#include <util/mpack.h>
#include <ex.h>

#define _TI_PCRE2_STRINGIFY_(num) #num
#define _TI_PCRE2_VERSION_STR_(major,minor) \
    _TI_PCRE2_STRINGIFY_(major) "." \
    _TI_PCRE2_STRINGIFY_(minor)

#define TI_PCRE2_VERSION _TI_PCRE2_VERSION_STR_( \
        PCRE2_MAJOR, \
        PCRE2_MINOR) \
        PCRE2_PRERELEASE

struct ti_regex_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _flags;
    uint16_t _pad16;

    pcre2_code * code;
    pcre2_match_data * match_data;
    ti_raw_t * pattern;
};

static inline int ti_regex_to_client_pk(ti_regex_t * re, msgpack_packer * pk)
{
    return mp_pack_strn(pk, re->pattern->data, re->pattern->n);
}

static inline int ti_regex_to_store_pk(ti_regex_t * re, msgpack_packer * pk)
{
    return mp_pack_ext(pk, MPACK_EXT_REGEX, re->pattern->data, re->pattern->n);
}

static inline _Bool ti_regex_test(ti_regex_t * regex, ti_raw_t * raw)
{
    return pcre2_match(
            regex->code,
            (PCRE2_SPTR8) raw->data,
            raw->n,
            0,                     /* start looking at this point */
            0,                     /* OPTIONS */
            regex->match_data,
            NULL) >= 0;
}

static inline _Bool ti_regex_test_or_empty(ti_regex_t * regex, ti_raw_t * raw)
{
    return raw->n == 0 || pcre2_match(
            regex->code,
            (PCRE2_SPTR8) raw->data,
            raw->n,
            0,                     /* start looking at this point */
            0,                     /* OPTIONS */
            regex->match_data,
            NULL) >= 0;
}

static inline _Bool ti_regex_eq(ti_regex_t * ra, ti_regex_t * rb)
{
    return ti_raw_eq(ra->pattern, rb->pattern);
}

#endif  /* TI_REGEX_T_H_ */
