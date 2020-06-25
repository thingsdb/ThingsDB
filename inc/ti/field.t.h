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

struct ti_field_s
{
    ti_name_t * name;
    ti_raw_t * spec_raw;
    ti_type_t * type;           /* parent type */
    uint16_t spec;
    uint16_t nested_spec;       /* array/set have a nested specification */
    uint32_t idx;               /* index of the field withing the type */
};

#endif  /* TI_FIELD_T_H_ */
