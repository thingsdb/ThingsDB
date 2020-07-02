/*
 * ti/field.t.h
 */
#ifndef TI_FIELD_T_H_
#define TI_FIELD_T_H_

typedef struct ti_field_s ti_field_t;

#include <stdint.h>
#include <ti/name.t.h>
#include <ti/raw.t.h>
#include <ti/type.t.h>

typedef struct ti_match_re_s ti_match_re_t;
typedef struct ti_match_srange_s ti_match_srange_t;
typedef struct ti_match_irange_s ti_match_irange_t;
typedef struct ti_match_drange_s ti_match_drange_t;

struct ti_match_re_s
{
    pcre2_code * code;
    pcre2_match_data * match_data;
};

struct ti_match_srange_s
{
    size_t mi;
    size_t ma;
};

struct ti_match_irange_s
{
    int64_t mi;
    int64_t ma;
};

struct ti_match_drange_s
{
    double mi;
    double ma;
};

typedef union
{
    ti_match_re_t * re;             /* str, utf8 */
    ti_match_srange_t * srange;     /* str, utf8 */
    ti_match_irange_t * irange;     /* int, float */
    ti_match_drange_t * drange;     /* int, float */
} ti_match_via_t;


struct ti_field_s
{
    ti_name_t * name;
    ti_raw_t * spec_raw;
    ti_type_t * type;           /* parent type */
    uint16_t spec;
    uint16_t nested_spec;       /* array/set have a nested specification */
    uint32_t idx;               /* index of the field within the type */
    ti_match_via_t via;
};

#endif  /* TI_FIELD_T_H_ */
