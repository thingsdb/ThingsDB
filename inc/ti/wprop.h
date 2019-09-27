/*
 * ti/wprop.h
 */
#ifndef TI_WPROP_H_
#define TI_WPROP_H_

typedef struct ti_wprop_s  ti_wprop_t;

#include <ti/name.h>
#include <ti/val.h>

struct ti_wprop_s
{
    ti_name_t * name;   /* without reference */
    ti_val_t * val;     /* without reference */
};

#endif /* TI_WPROP_H_ */
