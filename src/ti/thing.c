/*
 * thing.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/thing.h>
#include <util/logger.h>
#include <ti/prop.h>

ti_thing_t * ti_thing_create(uint64_t id, imap_t * things)
{
    assert (( !id && !things ) || ( id && things ));

    ti_thing_t * thing = malloc(sizeof(ti_thing_t));
    if (!thing)
        return NULL;

    thing->ref = 1;
    thing->id = id;
    thing->things = things;
    thing->props = vec_new(0);
    thing->flags = TI_THING_FLAG_SWEEP;
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
        if (thing->things)
            (void *) imap_pop(thing->things, thing->id);
        vec_destroy(thing->props, (vec_destroy_cb) ti_prop_destroy);
        free(thing);
    }
}

ti_val_t * ti_thing_get(ti_thing_t * thing, ti_name_t * name)
{
    for (vec_each(thing->props, ti_prop_t, prop))
        if (prop->name == name)
            return &prop->val;
    return NULL;
}

int ti_thing_set(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_val_enum tp,
        void * v)
{
    ti_prop_t * prop;

    for (vec_each(thing->props, ti_prop_t, prop))
    {
        if (prop->name == name)
        {
            ti_val_clear(&prop->val);
            return ti_val_set(&prop->val, tp, v);
        }
    }

    prop = ti_prop_create(name, tp, v);
    if (!prop || vec_push(&thing->props, prop))
    {
        ti_prop_destroy(prop);
        return -1;
    }

    return 0;
}

int ti_thing_setv(ti_thing_t * thing, ti_name_t * name, ti_val_t * val)
{
    ti_prop_t * prop;

    for (vec_each(thing->props, ti_prop_t, prop))
    {
        if (prop->name == name)
        {
            ti_val_clear(&prop->val);
            return ti_val_copy(&prop->val, val);
        }
    }

    prop = ti_prop_createv(name, val);
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
        ti_val_enum tp,
        void * v)
{
    ti_prop_t * prop;

    for (vec_each(thing->props, ti_prop_t, prop))
    {
        if (prop->name == name)
        {
            ti_val_clear(&prop->val);
            ti_val_weak_set(&prop->val, tp, v);
            return 0;
        }
    }

    prop = ti_prop_weak_create(name, tp, v);
    if (!prop || vec_push(&thing->props, prop))
    {
        ti_prop_destroy(prop);
        return -1;
    }
    return 0;
}

int ti_thing_weak_setv(ti_thing_t * thing, ti_name_t * name, ti_val_t * val)
{
    ti_prop_t * prop;

    for (vec_each(thing->props, ti_prop_t, prop))
    {
        if (prop->name == name)
        {
            ti_val_clear(&prop->val);
            ti_val_weak_copy(&prop->val, val);
            return 0;
        }
    }

    prop = ti_prop_weak_createv(name, val);
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
            qp_add_raw(*packer, (const unsigned char *) "$id", 3) ||
            qp_add_int64(*packer, (int64_t) thing->id))
        return -1;

    for (vec_each(thing->props, ti_prop_t, prop))
    {
        if (    qp_add_raw_from_str(*packer, prop->name->str) ||
                ti_val_to_packer(&prop->val, packer))
            return -1;
    }
    return qp_close_map(*packer);
}
