/*
 * ti/field.h
 */
#ifndef TI_FIELD_H_
#define TI_FIELD_H_

typedef struct ti_field_s ti_field_t;

#include <ti/name.h>
#include <ti/raw.h>
#include <ti/val.h>
#include <inttypes.h>
#include <ex.h>

int ti_field_create(
        ti_name_t * name,
        ti_raw_t * spec_raw,
        ti_type_t * type,
        ex_t * e);
void ti_field_destroy(ti_field_t * field);
int ti_field_check_val(ti_field_t * field, ti_val_t * val, ex_t * e);
int ti_field_check_field(ti_field_t * to, ti_field_t * from, ex_t * e);

struct ti_field_s
{
    ti_name_t * name;
    ti_raw_t * spec_raw;
    ti_type_t * type;
    uint16_t spec;
    uint16_t nested_spec;       /* array/set have a nested specification */
};



#endif  /* TI_FIELD_H_ */
