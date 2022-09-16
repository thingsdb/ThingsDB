/*
 * ti/field.t.h
 */
#ifndef TI_FIELD_T_H_
#define TI_FIELD_T_H_

typedef struct ti_field_map_s ti_field_map_t;
typedef struct ti_field_s ti_field_t;

#include <stdint.h>
#include <ti/condition.t.h>
#include <ti/flags.h>
#include <ti/name.t.h>
#include <ti/raw.t.h>
#include <ti/type.t.h>
#include <ti/val.t.h>

typedef ti_val_t *  (*ti_field_dval_cb) (ti_field_t *);

enum
{
    TI_FIELD_FLAG_SAME_DEEP=1,
    TI_FIELD_FLAG_MIN_DEEP=2,
    TI_FIELD_FLAG_MAX_DEEP=4,
    TI_FIELD_FLAG_DEEP=8,
    TI_FIELD_FLAG_NO_IDS=TI_FLAGS_NO_IDS,
};

#define TI_FIELD_MIN_MAX (TI_FIELD_FLAG_MIN_DEEP|TI_FIELD_FLAG_MAX_DEEP)

struct ti_field_map_s
{
    char name[9];   /* MAX_WORD_LENGTH+1 */
    uint16_t n;
    uint16_t spec;
    ti_field_dval_cb dval_cb;
};

struct ti_field_s
{
    ti_name_t * name;
    ti_raw_t * spec_raw;
    ti_type_t * type;           /* parent type */
    uint16_t spec;
    uint16_t nested_spec;       /* array/set have a nested definition */
    uint32_t idx;               /* index of the field within the type */
    ti_field_dval_cb dval_cb;
    ti_condition_via_t condition;
    int flags;
};

#endif  /* TI_FIELD_T_H_ */
