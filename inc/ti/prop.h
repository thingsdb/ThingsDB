/*
 * prop.h
 */
#ifndef TI_PROP_H_
#define TI_PROP_H_

typedef struct ti_prop_s  ti_prop_t;

#include <stdint.h>
#include <ti/name.h>
#include <ti/val.h>

ti_prop_t * ti_prop_create(ti_name_t * name, ti_val_t * val);
void ti_prop_destroy(ti_prop_t * prop);

struct ti_prop_s
{
    ti_name_t * name;   /* with reference */
    ti_val_t * val;     /* with reference */
};

#endif /* TI_PROP_H_ */
