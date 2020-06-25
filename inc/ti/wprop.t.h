/*
 * ti/wprop.t.h
 */
#ifndef TI_WPROP_T_H_
#define TI_WPROP_T_H_

typedef struct ti_wprop_s  ti_wprop_t;

#include <ti/name.t.h>
#include <ti/val.t.h>

struct ti_wprop_s
{
    ti_name_t * name;   /* without reference */
    ti_val_t ** val;    /* without reference */
};

#endif /* TI_WPROP_T_H_ */
