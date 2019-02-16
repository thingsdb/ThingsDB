/*
 * ti/regex.h
 */
#ifndef TI_REGEX_H_
#define TI_REGEX_H_

#define PCRE2_CODE_UNIT_WIDTH 8

typedef struct ti_regex_s ti_regex_t;

#include <qpack.h>
#include <pcre2.h>
#include <stddef.h>
#include <ti/raw.h>
#include <ti/ex.h>

ti_regex_t * ti_regex_from_strn(const char * str, size_t n, ex_t * e);
void ti_regex_destroy(ti_regex_t * regex);
static inline int ti_regex_to_packer(ti_regex_t * regex, qp_packer_t ** packer);
static inline int ti_regex_to_file(ti_regex_t * regex, FILE * f);
static inline _Bool ti_regex_test(ti_regex_t * regex, ti_raw_t * raw);

struct ti_regex_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _pad8;
    uint16_t _pad16;

    pcre2_code * code;
    pcre2_match_data * match_data;
    ti_raw_t * pattern;
};

static inline int ti_regex_to_packer(ti_regex_t * regex, qp_packer_t ** packer)
{
    return -(
        qp_add_map(packer) ||
        qp_add_raw(*packer, (const uchar * ) "*", 1) ||
        qp_add_raw(*packer, regex->pattern->data, regex->pattern->n) ||
        qp_close_map(*packer)
    );
}

static inline int ti_regex_to_file(ti_regex_t * regex, FILE * f)
{
    return -(
        qp_fadd_type(f, QP_MAP1) ||
        qp_fadd_raw(f, (const uchar * ) "*", 1) ||
        qp_fadd_raw(f, regex->pattern->data, regex->pattern->n)
    );
}

static inline _Bool ti_regex_test(ti_regex_t * regex, ti_raw_t * raw)
{
    return pcre2_match(
            regex->code,
            (PCRE2_SPTR8) raw->data,
            raw->n,
            0,                     // start looking at this point
            0,                     // OPTIONS
            regex->match_data,
            NULL) >= 0;
}

#endif  /* TI_REGEX_H_ */
