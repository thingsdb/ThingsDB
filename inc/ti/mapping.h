/*
 * ti/mapping.h
 */
#ifndef TI_MAPPING_H_
#define TI_MAPPING_H_

typedef struct ti_mapping_s ti_mapping_t;

#include <ti/field.h>

ti_mapping_t * ti_mapping_new(ti_field_t * t_field, ti_field_t * f_field);

struct ti_mapping_s
{
    ti_field_t * t_field;  /* weak reference */
    ti_field_t * f_field;  /* weak reference */
};

#endif  /* TI_MAPPING_H_ */
