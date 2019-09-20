/*
 * ti/field.h
 */
#ifndef TI_FIELD_H_
#define TI_FIELD_H_

typedef struct ti_field_s ti_field_t;

#include <ti/name.h>
#include <inttypes.h>

struct ti_field_s
{
    ti_name_t * name;
    uint16_t sid;
    char * spec;
};


#endif  /* TI_FIELD_H_ */
