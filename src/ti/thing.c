/*
 * thing.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/prop.h>
#include <ti/thing.h>
#include <ti/val.h>
#include <util/logger.h>

ti_thing_t * ti_thing_create(uint64_t id, imap_t * things)
{
    ti_thing_t * thing = malloc(sizeof(ti_thing_t));
    if (!thing)
        return NULL;

    thing->ref = 1;
    thing->id = id;
    thing->things = things;
    thing->props = vec_new(0);
    thing->attrs = NULL;
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
        if (thing->id)
            (void *) imap_pop(thing->things, thing->id);
        vec_destroy(thing->props, (vec_destroy_cb) ti_prop_destroy);
        vec_destroy(thing->attrs, (vec_destroy_cb) ti_prop_destroy);
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

void * ti_thing_attr_get(ti_thing_t * thing, ti_name_t * name)
{
    for (vec_each(thing->attrs, ti_prop_t, attr))
        if (attr->name == name)
            return attr;
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
        ti_prop_weak_destroy(prop);
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
        ti_prop_weak_destroy(prop);
        return -1;
    }
    return 0;
}

int ti_thing_attr_weak_setv(
        ti_thing_t * thing,
        ti_name_t * name,
        ti_val_t * val)
{
    ti_prop_t * prop;

    if (!thing->attrs)
    {
        thing->attrs = vec_new(1);
        if (!thing->attrs)
            return -1;
    }
    else for (vec_each(thing->attrs, ti_prop_t, prop))
    {
        if (prop->name == name)
        {
            ti_val_clear(&prop->val);
            ti_val_weak_copy(&prop->val, val);
            return 0;
        }
    }

    prop = ti_prop_weak_createv(name, val);
    if (!prop || vec_push(&thing->attrs, prop))
    {
        ti_prop_weak_destroy(prop);
        return -1;
    }
    return 0;
}

int ti_thing_gen_id(ti_thing_t * thing)
{
    assert (!thing->id);

    thing->id = ti_next_thing_id();
    ti_thing_mark_new(thing);

    if (ti_thing_to_map(thing))
        return -1;

    for (vec_each(thing->props, ti_prop_t, prop))
    {
        ti_val_t * val = &prop->val;

        if (val->tp == TI_VAL_THING)
        {
            if (val->via.thing->id)
            {
                ti_thing_unmark_new(val->via.thing);
                continue;
            }

            if (ti_thing_gen_id(val->via.thing))
                return -1;

            continue;
        }

        if (val->tp != TI_VAL_THINGS)
            continue;

        for (vec_each(val->via.things, ti_thing_t, tthing))
        {
            if (tthing->id)
            {
                ti_thing_unmark_new(tthing);
                continue;
            }
            if (ti_thing_gen_id(tthing))
                return -1;
        }
    }
    return 0;
}

int ti_thing_to_packer(ti_thing_t * thing, qp_packer_t ** packer, int pack)
{
    assert (pack == TI_VAL_PACK_FETCH || pack == TI_VAL_PACK_NEW);

    if (    qp_add_map(packer) ||
            qp_add_raw(*packer, (const uchar *) "#", 1) ||
            qp_add_int64(*packer, (int64_t) thing->id))
        return -1;

    for (vec_each(thing->props, ti_prop_t, prop))
    {
        if (    qp_add_raw_from_str(*packer, prop->name->str) ||
                ti_val_to_packer(&prop->val, packer, pack))
            return -1;
    }
    return qp_close_map(*packer);
}
