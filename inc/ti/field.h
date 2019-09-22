/*
 * ti/field.h
 */
#ifndef TI_FIELD_H_
#define TI_FIELD_H_

typedef struct ti_field_s ti_field_t;

#include <ti/name.h>
#include <ti/val.h>
#include <inttypes.h>
#include <ex.h>

typedef enum
{
    TI_FIELD_FLAG_NULLABLE       =1<<0,
    TI_FIELD_FLAG_UTF8           =1<<1,     /* raw type must be valid utf8 */
    TI_FIELD_FLAG_ANY            =1<<2,
} ti_field_flag_enum_t;

ti_field_t * ti_create_field(ti_name_t * name, ti_raw_t * spec, ex_t * e);
void ti_field_destroy(ti_field_t * field);
int ti_field_check(ti_field_t * field, ti_val_t * val, ex_t * e);

struct ti_field_s
{
    ti_name_t * name;
    ti_raw_t * spec;
    ti_val_t * vdefault;
    ti_field_flag_enum_t flags;
};



#endif  /* TI_FIELD_H_ */
