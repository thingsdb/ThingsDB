/*
 * ti/things.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/names.h>
#include <ti/prop.h>
#include <ti/things.h>
#include <ti/val.h>
#include <ti/varr.h>
#include <ti/vset.h>
#include <ti/watch.h>
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

inline static int things__set_cb(ti_thing_t * thing, void * UNUSED(arg))
{
    if (thing->flags & TI_VFLAG_THING_SWEEP)
        things__gc_mark_thing(thing);
    return 0;
}

static void things__gc_mark_thing(ti_thing_t * thing)
{
    thing->flags &= ~TI_VFLAG_THING_SWEEP;
    for (vec_each(thing->props, ti_prop_t, prop))
    {
        switch(prop->val->tp)
        {
        case TI_VAL_THING:
        {
            ti_thing_t * thing = (ti_thing_t *) prop->val;
            if (thing->flags & TI_VFLAG_THING_SWEEP)
                things__gc_mark_thing(thing);
            continue;
        }

        case TI_VAL_ARR:
        {
            ti_varr_t * varr = (ti_varr_t *) prop->val;
            if (ti_varr_may_have_things(varr))
                things__gc_mark_varr(varr);
            continue;
        }

        case TI_VAL_SET:
        {
            ti_vset_t * vset = (ti_vset_t *) prop->val;
            (void) imap_walk(vset->imap, (imap_cb) things__set_cb, NULL);
            continue;

        }
        }
    }
}

ti_thing_t * ti_things_create_thing(imap_t * things, uint64_t id)
{
    assert (id);
    ti_thing_t * thing = ti_thing_create(id, things);
    if (!thing || ti_thing_to_map(thing))
    {
        ti_val_drop((ti_val_t *) thing);
        return NULL;
    }
    return thing;
}

/* Returns a thing with a new reference or NULL in case of an error */
ti_thing_t * ti_things_thing_from_unp(
        imap_t * things,
        uint64_t thing_id,
        qp_unpacker_t * unp,
        ssize_t sz,
        ex_t * e)
{
    ti_thing_t * thing;
    thing = imap_get(things, thing_id);
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

    if (unp->flags & TI_VAL_UNP_FROM_CLIENT)
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

    thing = ti_things_create_thing(things, thing_id);
    if (!thing)
    {
        ex_set_mem(e);
        return NULL;
    }

    if (thing_id >= ti()->node->next_thing_id)
        ti()->node->next_thing_id = thing_id + 1;

    --sz;  /* decrease one to unpack the remaining properties */
    if (ti_thing_props_from_unp(thing, things, unp, sz, e))
    {
        ti_val_drop((ti_val_t *) thing);
        return NULL;
    }
    return thing;
}

int ti_things_gc(imap_t * things, ti_thing_t * root)
{
    size_t n = 0;
    vec_t * things_vec = imap_vec_pop(things);
    if (!things_vec)
        return -1;

    (void) ti_sleep(100);

    if (root)
        things__gc_mark_thing(root);

    (void) ti_sleep(100);

    for (vec_each(things_vec, ti_thing_t, thing))
        if (thing->flags & TI_VFLAG_THING_SWEEP)
            thing->ref = 0;

    (void) ti_sleep(100);

    for (vec_each(things_vec, ti_thing_t, thing))
        if (thing->flags & TI_VFLAG_THING_SWEEP)
            ti_thing_clear(thing);

    (void) ti_sleep(100);

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

    ti()->counters->garbage_collected += n;

    log_debug("garbage collection cleaned: %zd thing(s)", n);

    return 0;
}
