/*
 * item.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <rql/item.h>

void rql_item_destroy(rql_item_t * item)
{
    if (!item) return;
    rql_prop_drop(item->prop);
    rql_val_clear(&item->val);
    free(item);
}
