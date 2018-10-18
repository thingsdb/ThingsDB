/*
 * thing.c
 */
#include <stdlib.h>
#include <ti/thing.h>
#include <util/logger.h>

ti_thing_t * ti_thing_create(uint64_t id)
{
    ti_thing_t * thing = malloc(sizeof(ti_thing_t));
    if (!thing)
        return NULL;

    thing->ref = 1;
    thing->id = id;
    thing->items = vec_new(0);
    thing->flags = TI_ELEM_FLAG_SWEEP;
    if (!thing->items)
    {
        ti_thing_drop(thing);
        return NULL;
    }
    return thing;
}

ti_thing_t * ti_thing_grab(ti_thing_t * thing)
{
    thing->ref++;
    return thing;
}

void ti_thing_drop(ti_thing_t * thing)
{
    if (thing && !--thing->ref)
    {
        (void *) imap_pop(things, thing->id);
        vec_destroy(thing->items, (vec_destroy_cb) ti_item_destroy);
        free(thing);
    }
}

int ti_thing_set(ti_thing_t * thing, ti_prop_t * prop, ti_val_e tp, void * v)
{
    for (vec_each(thing->items, ti_item_t, item))
    {
        if (item->prop == prop)
        {
            ti_val_clear(&item->val);
            return ti_val_set(&item->val, tp, v);
        }
    }
    ti_item_t * item = ti_item_create(prop, tp, v);
    if (!item || vec_push(&thing->items, item))
    {
        ti_item_destroy(item);
        return -1;
    }

    return 0;
}

int ti_thing_weak_set(
        ti_thing_t * thing,
        ti_prop_t * prop,
        ti_val_e tp,
        void * v)
{
    for (vec_each(thing->items, ti_item_t, item))
    {
        if (item->prop == prop)
        {
            ti_val_clear(&item->val);
            ti_val_weak_set(&item->val, tp, v);
            return 0;
        }
    }
    ti_item_t * item = ti_item_weak_create(prop, tp, v);
    if (!item || vec_push(&thing->items, item))
    {
        ti_item_destroy(item);
        return -1;
    }
    return 0;
}

int ti_thing_to_packer(ti_thing_t * thing, qp_packer_t * packer)
{
    if (    qp_add_map(&packer) ||
            qp_add_raw(packer, (const unsigned char *) TI_API_ID, 2) ||
            qp_add_int64(packer, (int64_t) thing->id))
        return -1;

    for (vec_each(thing->items, ti_item_t, item))
    {
        if (    qp_add_raw_from_str(packer, item->prop->name) ||
                ti_val_to_packer(&item->val, packer))
            return -1;
    }
    return qp_close_map(packer);
}
