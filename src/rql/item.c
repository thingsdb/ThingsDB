/*
 * item.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <rql/item.h>

rql_item_t * rql_item_create(rql_prop_t * prop, rql_val_e tp, void * v)
{
    rql_item_t * item = (rql_item_t *) malloc(sizeof(rql_item_t));
    if (!item || rql_val_set(&item->val, tp, v))
    {
        rql_item_destroy(item);
        return NULL;
    }
    item->prop = rql_prop_grab(prop);
    return item;
}

rql_item_t * rql_item_weak_create(rql_prop_t * prop, rql_val_e tp, void * v)
{
    rql_item_t * item = (rql_item_t *) malloc(sizeof(rql_item_t));
    if (!item) return NULL;
    rql_val_weak_set(&item->val, tp, v);
    item->prop = rql_prop_grab(prop);
    return item;
}

void rql_item_destroy(rql_item_t * item)
{
    if (!item) return;
    rql_prop_drop(item->prop);
    rql_val_clear(&item->val);
    free(item);
}
