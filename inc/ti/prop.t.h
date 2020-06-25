/*
 * ti/prop.t.h
 */
#ifndef TI_PROP_T_H_
#define TI_PROP_T_H_

typedef struct ti_prop_s  ti_prop_t;

#include <ti/name.t.h>
#include <ti/val.t.h>

struct ti_prop_s
{
    ti_name_t * name;   /* with reference */
    ti_val_t * val;     /* with reference */
};

#endif /* TI_PROP_T_H_ */
