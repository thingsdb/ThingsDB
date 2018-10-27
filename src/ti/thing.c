/*
 * thing.c
 */
#include <stdlib.h>
#include <ti/thing.h>
#include <util/logger.h>
#include <ti/prop.h>

ti_thing_t * ti_thing_create(uint64_t id, imap_t * things)
{
    ti_thing_t * thing = malloc(sizeof(ti_thing_t));
    if (!thing)
        return NULL;

    thing->ref = 1;
    thing->id = id;
    thing->things = things;
    thing->props = vec_new(0);
    thing->flags = TI_ELEM_FLAG_SWEEP;
    if (!thing->props)
    {
        ti_thing_drop(thing);
        return NULL;
    }
    return thing;
}

void ti_thing_drop(ti_thing_t * thing)
{
    if (thing && !--thing->ref)
    {
        (void *) imap_pop(thing->things, thing->id);
        vec_destroy(thing->props, (vec_destroy_cb) ti_prop_destroy);
        free(thing);
    }
}

int ti_thing_set(ti_thing_t * thing, ti_name_t * name, ti_val_e tp, void * v)
{
    for (vec_each(thing->props, ti_prop_t, prop))
    {
        if (prop->name == name)
        {
            ti_val_clear(&prop->val);
            return ti_val_set(&prop->val, tp, v);
        }
    }
    ti_prop_t * prop = ti_prop_create(name, tp, v);
    if (!prop || vec_push(&thing->props, prop))
    {
        ti_prop_destroy(prop);
        return -1;
    }

    return 0;
}

int ti_thing_weak_set(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_val_e tp,
        void * v)
{
    for (vec_each(thing->props, ti_prop_t, prop))
    {
        if (prop->name == name)
        {
            ti_val_clear(&prop->val);
            ti_val_weak_set(&prop->val, tp, v);
            return 0;
        }
    }
    ti_prop_t * prop = ti_prop_weak_create(name, tp, v);
    if (!prop || vec_push(&thing->props, prop))
    {
        ti_prop_destroy(prop);
        return -1;
    }
    return 0;
}

int ti_thing_to_packer(ti_thing_t * thing, qp_packer_t ** packer)
{
    if (    qp_add_map(packer) ||
            qp_add_raw(*packer, (const unsigned char *) TI_API_ID, 2) ||
            qp_add_int64(*packer, (int64_t) thing->id))
        return -1;

    for (vec_each(thing->props, ti_prop_t, prop))
    {
        if (    qp_add_raw_from_str(*packer, prop->name->str) ||
                ti_val_to_packer(&prop->val, packer))
            return -1;
    }
    return qp_close_map(packer);
}
