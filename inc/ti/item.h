/*
 * item.h
 */
#ifndef TI_ITEM_H_
#define TI_ITEM_H_

typedef struct ti_item_s  ti_item_t;

#include <stdint.h>
#include <ti/prop.h>
#include <ti/val.h>

ti_item_t * ti_item_create(ti_prop_t * prop, ti_val_e tp, void * v);
ti_item_t * ti_item_weak_create(ti_prop_t * prop, ti_val_e tp, void * v);
void ti_item_destroy(ti_item_t * item);

struct ti_item_s
{
    ti_prop_t * prop;
    ti_val_t val;
};

#endif /* TI_ITEM_H_ */
