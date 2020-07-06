/*
 * ti/condition.t.h
 */
#ifndef TI_CONDITION_T_H_
#define TI_CONDITION_T_H_

#define PCRE2_CODE_UNIT_WIDTH 8

typedef struct ti_condition_re_s ti_condition_re_t;
typedef struct ti_condition_srange_s ti_condition_srange_t;
typedef struct ti_condition_irange_s ti_condition_irange_t;
typedef struct ti_condition_drange_s ti_condition_drange_t;

#include <pcre2.h>
#include <stdint.h>
#include <stdlib.h>
#include <ti/raw.t.h>
#include <ti/vint.h>
#include <ti/vfloat.h>

struct ti_condition_re_s
{
    pcre2_code * code;
    pcre2_match_data * match_data;
};

struct ti_condition_srange_s
{
    size_t mi;
    size_t ma;
    ti_raw_t * dval;
};

struct ti_condition_irange_s
{
    int64_t mi;
    int64_t ma;
    ti_vint_t * dval;
};

struct ti_condition_drange_s
{
    double mi;
    double ma;
    ti_vfloat_t * dval;
};

typedef union
{
    ti_condition_re_t * re;             /* str, utf8 */
    ti_condition_srange_t * srange;     /* str, utf8 */
    ti_condition_irange_t * irange;     /* int, float */
    ti_condition_drange_t * drange;     /* int, float */
    void * none;                        /* NULL */
} ti_condition_via_t;


#endif  /* TI_CONDITION_T_H_ */
