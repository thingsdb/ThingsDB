/*
 * ti/item.c
 */
#include <stdlib.h>
#include <ti.h>
#include <ti/raw.h>
#include <ti/item.h>
#include <ti/val.inline.h>

ti_item_t * ti_item_create(ti_raw_t * raw, ti_val_t * val)
{
    ti_item_t * item = malloc(sizeof(ti_item_t));
    if (!item)
        return NULL;

    item->val = val;
    item->key = raw;

    return item;
}

/*
 * Returns a duplicate of the item with a new reference for the raw and value.
 */
ti_item_t * ti_item_dup(ti_item_t * item)
{
    ti_item_t * dup = malloc(sizeof(ti_item_t));
    if (!item)
        return NULL;

    memcpy(dup, item, sizeof(ti_item_t));
    ti_incref(dup->key);
    ti_incref(dup->val);

    return dup;
}

void ti_item_destroy(ti_item_t * item)
{
    if (!item)
        return;
    ti_val_unsafe_drop((ti_val_t *) item->key);
    ti_val_unsafe_gc_drop(item->val);
    free(item);
}
