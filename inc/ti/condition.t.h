/*
 * ti/condition.t.h
 */
#ifndef TI_CONDITION_T_H_
#define TI_CONDITION_T_H_


typedef struct ti_condition_s ti_condition_t;
typedef struct ti_condition_re_s ti_condition_re_t;
typedef struct ti_condition_srange_s ti_condition_srange_t;
typedef struct ti_condition_irange_s ti_condition_irange_t;
typedef struct ti_condition_drange_s ti_condition_drange_t;
typedef struct ti_condition_rel_s ti_condition_rel_t;

#include <stdint.h>
#include <stdlib.h>
#include <ti/raw.t.h>
#include <ti/regex.h>
#include <ti/val.t.h>
#include <ti/field.t.h>

struct ti_condition_s
{
    ti_val_t * dval;
};

struct ti_condition_re_s
{
    ti_val_t * dval;
    ti_regex_t * regex;
};

struct ti_condition_srange_s
{
    ti_val_t * dval;
    size_t mi;
    size_t ma;
};

struct ti_condition_irange_s
{
    ti_val_t * dval;
    int64_t mi;
    int64_t ma;
};

struct ti_condition_drange_s
{
    ti_val_t * dval;
    double mi;
    double ma;
};

struct ti_condition_rel_s
{
    ti_field_t * field;
};

typedef union
{
    ti_condition_re_t * re;             /* str, utf8 */
    ti_condition_srange_t * srange;     /* str, utf8 */
    ti_condition_irange_t * irange;     /* int, float */
    ti_condition_drange_t * drange;     /* int, float */
    ti_condition_rel_t * rel;           /* relation */
    ti_condition_t * none;              /* NULL */
} ti_condition_via_t;


#endif  /* TI_CONDITION_T_H_ */
