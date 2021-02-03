/*
 * ti/collection.c
 */
#include <assert.h>
#include <doc.h>
#include <stdlib.h>
#include <string.h>
#include <ti.h>
#include <ti/auth.h>
#include <ti/collection.h>
#include <ti/collection.inline.h>
#include <ti/enums.h>
#include <ti/gc.h>
#include <ti/name.h>
#include <ti/name.h>
#include <ti/names.h>
#include <ti/member.t.h>
#include <ti/procedure.h>
#include <ti/raw.inline.h>
#include <ti/thing.h>
#include <ti/wrap.h>
#include <ti/things.h>
#include <ti/types.h>
#include <ti/val.inline.h>
#include <util/fx.h>
#include <util/strx.h>

static void collection__gc_mark_thing(ti_thing_t * thing);

static const size_t ti_collection_min_name = 1;
static const size_t ti_collection_max_name = 128;

ti_collection_t * ti_collection_create(
        guid_t * guid,
        const char * name,
        size_t n,
        ti_tz_t * tz,
        uint64_t created_at)
{
    ti_collection_t * collection = malloc(sizeof(ti_collection_t));
    if (!collection)
        return NULL;

    collection->ref = 1;
    collection->root = NULL;
    collection->name = ti_str_create(name, n);
    collection->things = imap_create();
    collection->gc = queue_new(20);
    collection->access = vec_new(1);
    collection->procedures = smap_create();
    collection->types = ti_types_create(collection);
    collection->enums = ti_enums_create(collection);
    collection->lock = malloc(sizeof(uv_mutex_t));
    collection->created_at = created_at;
    collection->tz = tz;
    collection->futures = vec_new(4);

    memcpy(&collection->guid, guid, sizeof(guid_t));

    if (!collection->name || !collection->things || !collection->gc ||
        !collection->access || !collection->procedures || !collection->lock ||
        !collection->types || !collection->enums || !collection->futures ||
        uv_mutex_init(collection->lock))
    {
        ti_collection_drop(collection);
        return NULL;
    }

    return collection;
}

void ti_collection_destroy(ti_collection_t * collection)
{
    if (!collection)
        return;

    assert (collection->things->n == 0);
    assert (collection->gc->n == 0);

    imap_destroy(collection->things, NULL);
    queue_destroy(collection->gc, NULL);
    ti_val_drop((ti_val_t *) collection->name);
    vec_destroy(collection->access, (vec_destroy_cb) ti_auth_destroy);
    smap_destroy(collection->procedures, (smap_destroy_cb) ti_procedure_destroy);
    ti_types_destroy(collection->types);
    ti_enums_destroy(collection->enums);
    uv_mutex_destroy(collection->lock);
    free(collection->futures);
    free(collection->lock);
    free(collection);
}

void ti_collection_drop(ti_collection_t * collection)
{
    if (!collection || --collection->ref)
        return;

    ti_val_drop((ti_val_t *) collection->root);

    if (!collection->things->n && !collection->gc->n)
    {
        ti_collection_destroy(collection);
        return;
    }

    if (ti_collections_add_for_collect(collection))
        log_critical(EX_MEMORY_S);
}

_Bool ti_collection_name_check(const char * name, size_t n, ex_t * e)
{
    if (n < ti_collection_min_name || n >= ti_collection_max_name)
    {
        ex_set(e, EX_VALUE_ERROR,
                "collection name must be between %u and %u characters",
                ti_collection_min_name,
                ti_collection_max_name);
        return false;
    }

    if (!ti_name_is_valid_strn(name, n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "collection name must follow the naming rules"
                DOC_NAMES);
        return false;
    }

    return true;
}

int ti_collection_rename(
        ti_collection_t * collection,
        ti_raw_t * rname,
        ex_t * e)
{
    if (!ti_name_is_valid_strn((const char *) rname->data, rname->n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "collection name must follow the naming rules"
                DOC_NAMES);
        return -1;
    }

    if (ti_collections_get_by_strn((const char *) rname->data, rname->n))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "collection `%.*s` already exists",
                (int) rname->n, (const char *) rname->data);
        return -1;
    }

    ti_val_unsafe_drop((ti_val_t *) collection->name);
    collection->name = ti_grab(rname);

    return 0;
}

ti_val_t * ti_collection_as_mpval(ti_collection_t * collection)
{
    ti_raw_t * raw;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_raw_t), sizeof(ti_raw_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (ti_collection_to_pk(collection, &pk))
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    raw = (ti_raw_t *) buffer.data;
    ti_raw_init(raw, TI_VAL_MP, buffer.size);

    return (ti_val_t *) raw;
}

/*
 * Usually make a call to `ti_collection_find_thing` which will first look in
 * the collection and only if not found will look at the garbage collection.
 */
ti_thing_t * ti_collection_thing_restore_gc(
        ti_collection_t * collection,
        uint64_t thing_id)
{
    size_t idx = 0;
    ti_thing_t * thing;

    /* look if the thing is marked for garbage collection */
    for (queue_each(collection->gc, ti_gc_t, gc), ++idx)
    {
        if ((thing = gc->thing)->id == thing_id)
        {
            assert (thing->flags & TI_THING_FLAG_SWEEP);
            assert (thing->ref);

            log_info("restoring "TI_THING_ID" from garbage", thing->id);

            if (imap_add(collection->things, thing_id, thing))
                ti_panic("unable to restore from garbage collection");

            ti_decref(thing);
            /*
             * The references of thing may be 0 at this point but even if this
             * is the case we still should not drop the thing since it will
             * soon get a new reference by the one calling this function.
             */
            free(queue_remove(collection->gc, idx));
            return thing;
        }
    }

    return NULL;
}

static void collection__gc_mark_varr(ti_varr_t * varr)
{
    for (vec_each(varr->vec, ti_val_t, val))
    {
        switch(val->tp)
        {
        case TI_VAL_THING:
        {
            ti_thing_t * thing = (ti_thing_t *) val;
            if (thing->flags & TI_THING_FLAG_SWEEP)
                collection__gc_mark_thing(thing);
            continue;
        }
        case TI_VAL_WRAP:
        {
            ti_thing_t * thing = ((ti_wrap_t *) val)->thing;
            if (thing->flags & TI_THING_FLAG_SWEEP)
                collection__gc_mark_thing(thing);
            continue;
        }
        case TI_VAL_ARR:
        {
            ti_varr_t * varr = (ti_varr_t *) val;
            if (ti_varr_may_have_things(varr))
                collection__gc_mark_varr(varr);
            continue;
        }
        }
    }
}

static inline int colection__set_cb(ti_thing_t * thing, void * UNUSED(arg))
{
    if (thing->flags & TI_THING_FLAG_SWEEP)
        collection__gc_mark_thing(thing);
    return 0;
}

static inline void collection__gc_val(ti_val_t * val)
{
    switch(val->tp)
    {
    case TI_VAL_THING:
    {
        ti_thing_t * thing = (ti_thing_t *) val;
        if (thing->flags & TI_THING_FLAG_SWEEP)
            collection__gc_mark_thing(thing);
        return;
    }
    case TI_VAL_WRAP:
    {
        ti_thing_t * thing = ((ti_wrap_t *) val)->thing;
        if (thing->flags & TI_THING_FLAG_SWEEP)
            collection__gc_mark_thing(thing);
        return;
    }
    case TI_VAL_ARR:
    {
        ti_varr_t * varr = (ti_varr_t *) val;
        if (ti_varr_may_have_things(varr))
            collection__gc_mark_varr(varr);
        return;
    }
    case TI_VAL_SET:
    {
        ti_vset_t * vset = (ti_vset_t *) val;
        (void) imap_walk(vset->imap, (imap_cb) colection__set_cb, NULL);
        return;
    }
    }
}

static int collection__mark_enum_cb(ti_enum_t * enum_, void * UNUSED(data))
{
    if (enum_->enum_tp == TI_ENUM_THING)
    {
        for (vec_each(enum_->members, ti_member_t, member))
        {
            ti_thing_t * thing = (ti_thing_t *) VMEMBER(member);
            if (thing->flags & TI_THING_FLAG_SWEEP)
                collection__gc_mark_thing(thing);
        }
    }
    return 0;
}

static int collection__gc_i_cb(ti_item_t * item, void * UNUSED(arg))
{
    collection__gc_val(item->val);
    return 0;
}

static void collection__gc_mark_thing(ti_thing_t * thing)
{
    thing->flags &= ~TI_THING_FLAG_SWEEP;

    if (ti_thing_is_object(thing))
        if (ti_thing_is_dict(thing))
            (void) smap_values(
                    thing->items.smap,
                    (smap_val_cb) collection__gc_i_cb,
                    NULL);
        else
            for (vec_each(thing->items.vec, ti_prop_t, prop))
                collection__gc_val(prop->val);
    else
        for (vec_each(thing->items.vec, ti_val_t, val))
            collection__gc_val(val);
}

void ti_collection_gc_clear(ti_collection_t * collection)
{
    ti_gc_t * gc;

    for (queue_each(collection->gc, ti_gc_t, gc))
        gc->thing->ref = 0;

    for (queue_each(collection->gc, ti_gc_t, gc))
        ti_thing_clear(gc->thing);

    while ((gc = queue_pop(collection->gc)))
    {
        ti_thing_destroy(gc->thing);
        free(gc);
    }
}

typedef struct
{
    ti_collection_t * collection;
    uint64_t cevid;
} collection__gc_t;

static int collection__gc_thing(ti_thing_t * thing, collection__gc_t * w)
{
    if (thing->flags & TI_THING_FLAG_SWEEP)
    {
        ti_gc_t * gc = ti_gc_create(w->cevid, thing);

        /*
         * Things are removed from the collection after the walk because we
         * cannot modify the collection at this point.
         */
        if (gc && queue_push(&w->collection->gc, gc) == 0)
            return 0;

        ti_gc_destroy(gc);

        log_error(
            "cannot mark "TI_THING_ID" for garbage collection",
            thing->id);
    }

    thing->flags |= TI_THING_FLAG_SWEEP;

    /*
     * Return success, also when marking has failed to make sure at least
     * all thing flags are restored
     */
    return 0;
}

int ti_collection_gc(ti_collection_t * collection, _Bool do_mark_things)
{
    size_t n = 0, m = 0, idx = 0;
    uint64_t sevid = ti.global_stored_event_id;
    struct timespec start, stop;
    double duration;
    collection__gc_t w = {
            .collection = collection,
            .cevid = ti.node ? ti.node->cevid : 0,
    };


    (void) clock_gettime(TI_CLOCK_MONOTONIC, &start);

    if (do_mark_things)
    {
        imap_walk(
                collection->enums->imap,
                (imap_cb) collection__mark_enum_cb,
                NULL);
        collection__gc_mark_thing(collection->root);

        (void) ti_sleep(2);
    }

    for (queue_each(collection->gc, ti_gc_t, gc))
    {
        if (gc->event_id > sevid)
        {
            /*
             * For all collected things above the stored event id need to
             * have the `TI_THING_FLAG_SWEEP` which might be removed by the
             * earlier markings.
             */
            gc->thing->flags |= TI_THING_FLAG_SWEEP;
            continue;
        }

        ++m;

        if (gc->thing->flags & TI_THING_FLAG_SWEEP)
        {
            /*
             * Clear all the properties which is safe because the garbage
             * collector still hold at least one reference.
             */
            ti_thing_clear(gc->thing);
        }
    }

    (void) ti_sleep(2);

    while (m--)
    {
        ti_gc_t * gc = queue_shift(collection->gc);
        ti_thing_t * thing = gc->thing;
        free(gc);

        if (thing->flags & TI_THING_FLAG_SWEEP)
        {
            ++n;
            ti_thing_destroy(thing);
            continue;
        }

        log_debug("restoring "TI_THING_ID" from garbage collection", thing->id);

        /*
         * The garbage collector has a reference and since the
         */
        assert (thing->ref > 1);
        ti_decref(thing);

        if (imap_add(collection->things, thing->id, thing))
            ti_panic("unable to restore from garbage collection");
        /*
         * Do not re-set the SWEEP flag since this will be done while
         * walking the things collection below.
         */
    }

    (void) ti_sleep(2);

    /*
     * Take the current garbage size to see which things are assigned by the
     * walk below.
     */
    m = collection->gc->n;

    (void) imap_walk(collection->things, (imap_cb) collection__gc_thing, &w);

    /*
     * Remove the marked things from the collection. This is done after the
     * walk to prevent changes to the collection map.
     */
    for (queue_each(collection->gc, ti_gc_t, gc), ++idx)
        if (idx >= m)
            (void) imap_pop(collection->things, gc->thing->id);

    (void) ti_sleep(2);

    ti_counters_add_garbage_collected(n);

    (void) clock_gettime(TI_CLOCK_MONOTONIC, &stop);
    duration = util_time_diff(&start, &stop);

    log_info(
        "garbage collection took %f seconds; "
        "%zu things(s) are marked as garbage and %zu thing(s) are cleaned",
        duration, idx-m, n);

    return 0;
}
