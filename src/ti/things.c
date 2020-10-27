/*
 * ti/things.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/enum.h>
#include <ti/field.h>
#include <ti/names.h>
#include <ti/prop.h>
#include <ti/thing.inline.h>
#include <ti/things.h>
#include <ti/types.inline.h>
#include <ti/val.h>
#include <ti/val.inline.h>
#include <ti/varr.h>
#include <ti/vset.h>
#include <ti/watch.h>
#include <ti/wrap.h>
#include <util/logger.h>

static void things__gc_mark_thing(ti_thing_t * thing);


static void things__gc_mark_varr(ti_varr_t * varr)
{
    for (vec_each(varr->vec, ti_val_t, val))
    {
        switch(val->tp)
        {
        case TI_VAL_THING:
        {
            ti_thing_t * thing = (ti_thing_t *) val;
            if (thing->flags & TI_VFLAG_THING_SWEEP)
                things__gc_mark_thing(thing);
            continue;
        }
        case TI_VAL_WRAP:
        {
            ti_thing_t * thing = ((ti_wrap_t *) val)->thing;
            if (thing->flags & TI_VFLAG_THING_SWEEP)
                things__gc_mark_thing(thing);
            continue;
        }
        case TI_VAL_ARR:
        {
            ti_varr_t * varr = (ti_varr_t *) val;
            if (ti_varr_may_have_things(varr))
                things__gc_mark_varr(varr);
            continue;
        }
        }
    }
}

static inline int things__set_cb(ti_thing_t * thing, void * UNUSED(arg))
{
    if (thing->flags & TI_VFLAG_THING_SWEEP)
        things__gc_mark_thing(thing);
    return 0;
}

static inline void things__gc_val(ti_val_t * val)
{
    switch(val->tp)
    {
    case TI_VAL_THING:
    {
        ti_thing_t * thing = (ti_thing_t *) val;
        if (thing->flags & TI_VFLAG_THING_SWEEP)
            things__gc_mark_thing(thing);
        return;
    }
    case TI_VAL_WRAP:
    {
        ti_thing_t * thing = ((ti_wrap_t *) val)->thing;
        if (thing->flags & TI_VFLAG_THING_SWEEP)
            things__gc_mark_thing(thing);
        return;
    }
    case TI_VAL_ARR:
    {
        ti_varr_t * varr = (ti_varr_t *) val;
        if (ti_varr_may_have_things(varr))
            things__gc_mark_varr(varr);
        return;
    }
    case TI_VAL_SET:
    {
        ti_vset_t * vset = (ti_vset_t *) val;
        (void) imap_walk(vset->imap, (imap_cb) things__set_cb, NULL);
        return;
    }
    }
}

ti_thing_t * ti_things_create_thing_o(
        uint64_t id,
        size_t init_sz,
        ti_collection_t * collection)
{
    assert (id);
    ti_thing_t * thing = ti_thing_o_create(id, init_sz, collection);
    if (!thing || ti_thing_to_map(thing))
    {
        ti_val_drop((ti_val_t *) thing);
        return NULL;
    }
    return thing;
}

ti_thing_t * ti_things_create_thing_t(
        uint64_t id,
        ti_type_t * type,
        ti_collection_t * collection)
{
    assert (id);
    ti_thing_t * thing = ti_thing_t_create(id, type, collection);
    if (!thing || ti_thing_to_map(thing))
    {
        ti_val_drop((ti_val_t *) thing);
        return NULL;
    }
    return thing;
}

/* Returns a thing with a new reference or NULL in case of an error */
ti_thing_t * ti_things_thing_o_from_vup(
        ti_vup_t * vup,
        uint64_t thing_id,
        size_t sz,
        ex_t * e)
{
    ti_thing_t * thing;
    thing = imap_get(vup->collection->things, thing_id);
    if (thing)
    {
        if (sz != 1)
        {
            ex_set(e, EX_BAD_DATA,
                    "cannot directly assign properties to "TI_THING_ID" "
                    "by adding properties to the data; "
                    "change the thing to {\""TI_KIND_S_THING"\": %"PRIu64"}",
                    thing_id, thing_id);
            return NULL;
        }
        ti_incref(thing);
        return thing;
    }

    if (vup->isclient)
    {
        /*
         * If not unpacking from an event, then new things should be created
         * without an id.
         */
        ex_set(e, EX_LOOKUP_ERROR,
                "thing "TI_THING_ID" not found; "
                "if you want to create a new thing then remove the id (`#`) "
                "and try again",
                thing_id);
        return NULL;
    }

    --sz;  /* decrease one to unpack the remaining properties */

    thing = ti_things_create_thing_o(thing_id, sz, vup->collection);
    if (!thing)
    {
        ex_set_mem(e);
        return NULL;
    }

    ti_update_next_thing_id(thing_id);

    if (ti_thing_props_from_vup(thing, vup, sz, e))
    {
        ti_val_unsafe_drop((ti_val_t *) thing);
        return NULL;
    }
    return thing;
}

/* Returns a thing with a new reference or NULL in case of an error */
ti_thing_t * ti_things_thing_t_from_vup(ti_vup_t * vup, ex_t * e)
{
    ti_thing_t * thing;
    ti_type_t * type;
    mp_obj_t obj, mp_thing_id, mp_type_id;

    if (vup->isclient)
    {
        ex_set(e, EX_BAD_DATA, "cannot unpack a type from a client request");
        return NULL;
    }

    if (mp_next(vup->up, &mp_type_id) != MP_U64 ||
        mp_skip(vup->up) != MP_STR ||   /* `#` */
        mp_next(vup->up, &mp_thing_id) != MP_U64 ||
        mp_skip(vup->up) != MP_STR ||   /* `` */
        mp_next(vup->up, &obj) != MP_ARR)
    {
        ex_set(e, EX_BAD_DATA,
                "invalid type data; "
                "expecting an type_id, things_id and array with values");
        return NULL;
    }

    type = ti_types_by_id(vup->collection->types, mp_type_id.via.u64);
    if (!type)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "invalid type data; type id %"PRIu64" not found",
                mp_type_id.via.u64);
        return NULL;
    }

    if (obj.via.sz != type->fields->n)
    {
        ex_set(e, EX_BAD_DATA,
                "invalid type data; "
                "expecting %"PRIu32" values for type `%s` but got only %u",
                type->fields->n, type->name, obj.via.sz);
        return NULL;
    }

    thing = ti_things_create_thing_t(
            mp_thing_id.via.u64,
            type,
            vup->collection);

    if (!thing)
    {
        /* No need to check for garbage collected things */
        if (imap_get(vup->collection->things, mp_thing_id.via.u64))
            ex_set(e, EX_LOOKUP_ERROR,
                    "error while loading type `%s`; "
                    "thing "TI_THING_ID" already exists",
                    type->name, mp_thing_id.via.u64);
        else
            ex_set_mem(e);
        return NULL;
    }

    /* Update the next thing id if required */
    ti_update_next_thing_id(mp_thing_id.via.u64);

    for (vec_each(type->fields, ti_field_t, field))
    {
        ti_val_t * val = ti_val_from_vup_e(vup, e);

        if (!val || ti_field_make_assignable(field, &val, thing, e))
        {
            ex_append(e, "; error while loading field `%s` for type `%s`",
                    field->name->str,
                    type->name);
            ti_val_drop(val);
            ti_val_unsafe_drop((ti_val_t *) thing);
            return NULL;
        }

        VEC_push(thing->items, val);
    }

    return thing;
}

static int things__mark_enum_cb(ti_enum_t * enum_, void * UNUSED(data))
{
    if (enum_->enum_tp == TI_ENUM_THING)
    {
        for (vec_each(enum_->members, ti_member_t, member))
        {
            ti_thing_t * thing = (ti_thing_t *) VMEMBER(member);
            if (thing->flags & TI_VFLAG_THING_SWEEP)
                things__gc_mark_thing(thing);
        }
    }
    return 0;
}

int ti_things_gc(imap_t * things, ti_thing_t * root)
{
    size_t n = 0;
    vec_t * things_vec;
    struct timespec start, stop;
    double duration;

    (void) clock_gettime(TI_CLOCK_MONOTONIC, &start);

    things_vec = imap_vec(things);
    if (!things_vec)
        return -1;

    (void) ti_sleep(2);  /* sleeps are here to allow thread switching */

    if (root)
    {
        imap_walk(
                root->collection->enums->imap,
                (imap_cb) things__mark_enum_cb,
                NULL);
        things__gc_mark_thing(root);
    }

    (void) ti_sleep(2);

    for (vec_each(things_vec, ti_thing_t, thing))
        if (thing->flags & TI_VFLAG_THING_SWEEP)
            thing->ref = 0;

    (void) ti_sleep(2);

    for (vec_each(things_vec, ti_thing_t, thing))
        if (thing->flags & TI_VFLAG_THING_SWEEP)
            ti_thing_clear(thing);

    (void) ti_sleep(2);

    for (vec_each(things_vec, ti_thing_t, thing))
    {
        if (thing->flags & TI_VFLAG_THING_SWEEP)
        {
            ++n;
            ti_thing_destroy(thing);
            continue;
        }
        thing->flags |= TI_VFLAG_THING_SWEEP;
    }

    free(things_vec);

    ti.counters->garbage_collected += n;

    (void) clock_gettime(TI_CLOCK_MONOTONIC, &stop);
    duration = util_time_diff(&start, &stop);

    log_debug("garbage collection took %f seconds and cleaned: %zu thing(s)",
            duration, n);

    return 0;
}

static void things__gc_mark_thing(ti_thing_t * thing)
{
    thing->flags &= ~TI_VFLAG_THING_SWEEP;

    if (ti_thing_is_object(thing))
        for (vec_each(thing->items, ti_prop_t, prop))
            things__gc_val(prop->val);
    else
        for (vec_each(thing->items, ti_val_t, val))
            things__gc_val(val);
}

