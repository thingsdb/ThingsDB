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
void ti_regex_drop(ti_regex_t * regex);
static inline int ti_regex_to_packer(ti_regex_t * regex, qp_packer_t ** packer);

struct ti_regex_s
{
    uint32_t ref;
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

#endif  /* TI_REGEX_H_ */
