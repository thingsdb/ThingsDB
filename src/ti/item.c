/*
 * item.c
 */
#include <stdlib.h>
#include <ti/item.h>
ti_item_t * ti_item_create(ti_prop_t * prop, ti_val_e tp, void * v)
{
    ti_item_t * item = malloc(sizeof(ti_item_t));
    if (!item || ti_val_set(&item->val, tp, v))
    {
        ti_item_destroy(item);
        return NULL;
    }
    item->prop = ti_prop_grab(prop);
    return item;
}

ti_item_t * ti_item_weak_create(ti_prop_t * prop, ti_val_e tp, void * v)
{
    ti_item_t * item = malloc(sizeof(ti_item_t));
    if (!item)
        return NULL;
    ti_val_weak_set(&item->val, tp, v);
    item->prop = ti_prop_grab(prop);
    return item;
}

void ti_item_destroy(ti_item_t * item)
{
    if (!item)
        return;
    ti_prop_drop(item->prop);
    ti_val_clear(&item->val);
    free(item);
}
