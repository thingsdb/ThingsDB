/*
 * ti/cast.h
 */
#ifndef TI_CAST_H_
#define TI_CAST_H_

typedef struct ti_cast_s ti_cast_t;

#include <ti/name.h>
#include <ti/raw.h>
#include <ti/val.h>
#include <inttypes.h>
#include <ex.h>

int ti_field_create(
        ti_name_t * name,
        ti_raw_t * spec_raw,
        ti_type_t * type,
        ti_types_t * types,
        ex_t * e);
void ti_field_destroy(ti_field_t * field);
int ti_field_check_val(ti_field_t * field, ti_val_t * val, ex_t * e);

struct ti_cast_s
{
    vec_t * p;
    ti_field_t * field;
    size_t * idx;
};



#endif  /* TI_CAST_H_ */
