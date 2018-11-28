/*
 * ti/things.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/prop.h>
#include <ti/things.h>
#include <util/logger.h>

static void things__gc_mark(ti_thing_t * thing);

ti_thing_t * ti_things_create_thing(imap_t * things, uint64_t id)
{
    assert (id);
    ti_thing_t * thing = ti_thing_create(id, things);
    if (!thing || ti_thing_to_map(thing))
    {
        ti_thing_drop(thing);
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
        things__gc_mark(root);

    (void) ti_sleep(100);

    for (vec_each(things_vec, ti_thing_t, thing))
        if (thing->flags & TI_THING_FLAG_SWEEP)
            thing->ref = 0;

    (void) ti_sleep(100);

    for (vec_each(things_vec, ti_thing_t, thing))
    {
        if (thing->flags & TI_THING_FLAG_SWEEP)
        {
            ++n;
            vec_destroy(thing->props, (vec_destroy_cb) ti_prop_destroy);
            (void *) imap_pop(things, thing->id);
            free(thing);
            continue;
        }
        thing->flags |= TI_THING_FLAG_SWEEP;
    }

    free(things_vec);

    ti()->counters->garbage_collected += n;

    log_debug("garbage collection cleaned: %zd thing(s)", n);

    return 0;
}


static void things__gc_mark(ti_thing_t * thing)
{
    thing->flags &= ~TI_THING_FLAG_SWEEP;
    for (vec_each(thing->props, ti_prop_t, prop))
    {
        switch (prop->val.tp)
        {
        case TI_VAL_THING:
            if (prop->val.via.thing->flags & TI_THING_FLAG_SWEEP)
                things__gc_mark(prop->val.via.thing);
            continue;
        case TI_VAL_THINGS:
            for (vec_each(prop->val.via.things, ti_thing_t, thing))
                if (thing->flags & TI_THING_FLAG_SWEEP)
                    things__gc_mark(thing);
            continue;
        default:
            continue;
        }
    }
}


