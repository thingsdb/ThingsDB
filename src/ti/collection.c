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
#include <ti/ctask.h>
#include <ti/enums.h>
#include <ti/future.h>
#include <ti/gc.h>
#include <ti/member.t.h>
#include <ti/name.h>
#include <ti/name.h>
#include <ti/names.h>
#include <ti/procedure.h>
#include <ti/raw.inline.h>
#include <ti/room.h>
#include <ti/thing.h>
#include <ti/things.h>
#include <ti/vtask.inline.h>
#include <ti/types.h>
#include <ti/val.inline.h>
#include <ti/whitelist.h>
#include <ti/wrap.h>
#include <util/fx.h>
#include <util/strx.h>

static void collection__gc_mark_thing(ti_thing_t * thing);

static const size_t ti_collection_min_name = 1;
static const size_t ti_collection_max_name = 128;

ti_collection_t * ti_collection_create(
        uint64_t collection_id,
        uint64_t next_free_id,
        guid_t * guid,
        const char * name,
        size_t n,
        uint64_t created_at,
        ti_tz_t * tz,
        uint8_t deep)
{
    /* we need the `next_free_id` for compatibility with older versions */
    ti_collection_t * collection = malloc(sizeof(ti_collection_t));
    if (!collection)
        return NULL;

    collection->ref = 1;
    collection->deep = deep;
    collection->root = NULL;
    collection->id = collection_id;
    collection->next_free_id = next_free_id;
    collection->name = ti_str_create(name, n);
    collection->things = imap_create();
    collection->rooms = imap_create();
    collection->gc = queue_new(20);
    collection->access = vec_new(1);
    collection->procedures = smap_create();
    collection->types = ti_types_create(collection);
    collection->enums = ti_enums_create(collection);
    collection->lock = malloc(sizeof(uv_mutex_t));
    collection->created_at = created_at;
    collection->tz = tz;
    collection->futures = vec_new(4);
    collection->vtasks = vec_new(4);
    collection->named_rooms = smap_create();

    memcpy(&collection->guid, guid, sizeof(guid_t));

    if (!collection->name || !collection->things || !collection->gc ||
        !collection->access || !collection->procedures || !collection->lock ||
        !collection->types || !collection->enums || !collection->futures ||
        !collection->rooms || !collection->named_rooms ||
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

    assert(collection->things->n == 0);
    assert(collection->rooms->n == 0);
    assert(collection->gc->n == 0);

    imap_destroy(collection->things, NULL);
    imap_destroy(collection->rooms, NULL);
    queue_destroy(collection->gc, NULL);
    ti_val_drop((ti_val_t *) collection->name);
    vec_destroy(collection->access, (vec_destroy_cb) ti_auth_destroy);
    vec_destroy(collection->vtasks, (vec_destroy_cb) ti_vtask_drop);
    smap_destroy(collection->procedures, (smap_destroy_cb) ti_procedure_destroy);
    smap_destroy(collection->named_rooms, NULL);
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

    if (!collection->things->n && !collection->gc->n && !collection->futures->n)
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
                rname->n, (const char *) rname->data);
        return -1;
    }

    ti_val_unsafe_drop((ti_val_t *) collection->name);
    collection->name = ti_grab(rname);

    return 0;
}

/*
 * Check if a collection is really empty. Note that futures aren't checked,
 * this is not required but keep in mind that they might exist and want to
 * modify the collection afterwards. Allowing futures allows for a module to
 * load an export and use import to restore the collection.
 */
#define TCCENM collection->name->n, (const char *) collection->name->data
int ti_collection_check_empty(ti_collection_t * collection, ex_t * e)
{
    const char * pf = "collection not empty;";

    if (collection->things->n != 1)
        ex_set(e, EX_OPERATION,
            "%s collection `%.*s` contains things", pf, TCCENM);
    else if (collection->types->imap->n)
        ex_set(e, EX_OPERATION,
            "%s collection `%.*s` contains types", pf, TCCENM);
    else if (ti_thing_n(collection->root))
        ex_set(e, EX_OPERATION,
            "%s collection `%.*s` contains properties", pf, TCCENM);
    else if (collection->enums->imap->n)
        ex_set(e, EX_OPERATION,
            "%s collection `%.*s` contains enumerators", pf, TCCENM);
    else if (collection->procedures->n)
        ex_set(e, EX_OPERATION,
            "%s collection `%.*s` contains procedures", pf, TCCENM);
    else if (collection->vtasks->n)
        ex_set(e, EX_OPERATION,
            "%s collection `%.*s` contains tasks", pf, TCCENM);
    else if (collection->root->ref > 1)
        ex_set(e, EX_OPERATION,
            "collection `%.*s` is being used", TCCENM);

    return e->nr;
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
    ti_raw_init(raw, TI_VAL_MPDATA, buffer.size);

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
            assert(thing->flags & TI_THING_FLAG_SWEEP);
            assert(thing->ref);

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
    uint64_t ccid;
} collection__gc_t;

static int collection__gc_thing(ti_thing_t * thing, collection__gc_t * w)
{
    if (thing->flags & TI_THING_FLAG_SWEEP)
    {
        ti_gc_t * gc = ti_gc_create(w->ccid, thing);

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

void ti_collection_stop_futures(ti_collection_t * collection)
{
    ti_future_t * future;

    /*
     * First take a reference, otherwise the stop future call might adjust the
     * vector.
     */
    for (vec_each(collection->futures, ti_future_t, future))
        ti_incref(future);

    for (vec_each(collection->futures, ti_future_t, future))
        ti_future_stop(future);

    while ((future = vec_pop(collection->futures)))
        ti_val_unsafe_drop((ti_val_t *) future);
}

int ti_collection_gc(ti_collection_t * collection, _Bool do_mark_things)
{
    size_t n = 0, m = 0, idx = 0;
    uint64_t scid = ti.global_stored_change_id;
    struct timespec start, stop;
    double duration;
    collection__gc_t w = {
            .collection = collection,
            .ccid = ti.node ? ti.node->ccid : 0,
    };

    (void) clock_gettime(TI_CLOCK_MONOTONIC, &start);

    if (do_mark_things)
    {
        assert(collection->futures->n == 0 && "Futures must be cancelled");

        /* Take a lock because flags are not atomic and might be changed */
        uv_mutex_lock(collection->lock);

        for (vec_each(collection->vtasks, ti_vtask_t, vtask))
            for (vec_each(vtask->args, ti_val_t, val))
                collection__gc_val(val);

        imap_walk(
                collection->enums->imap,
                (imap_cb) collection__mark_enum_cb,
                NULL);
        collection__gc_mark_thing(collection->root);

        /* Release the lock */
        uv_mutex_unlock(collection->lock);

        (void) ti_sleep(5);
    }

    uv_mutex_lock(collection->lock);

    for (queue_each(collection->gc, ti_gc_t, gc))
    {
        if (gc->change_id > scid)
        {
            /*
             * All collected things after the stored change id need the
             * `TI_THING_FLAG_SWEEP` flag which might be removed by the
             * earlier mark function.
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
        assert(thing->ref > 1);
        ti_decref(thing);

        if (imap_add(collection->things, thing->id, thing))
            ti_panic("unable to restore from garbage collection");
        /*
         * Do not re-set the SWEEP flag since this will be done while
         * walking the things collection below.
         */
    }

    /* Release the lock and let the thread sleep some time */
    uv_mutex_unlock(collection->lock);

    (void) ti_sleep(5);

    /* Take a new lock */
    uv_mutex_lock(collection->lock);

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


    /* Finished, release the collection lock */
    uv_mutex_unlock(collection->lock);

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

ti_pkg_t * ti_collection_join_rooms(
        ti_collection_t * collection,
        ti_stream_t * stream,
        ti_pkg_t * pkg,
        ex_t * e)
{
    ti_pkg_t * resp;
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    size_t i, nargs, size = pkg->n + sizeof(ti_pkg_t);
    mp_unp_t up;
    mp_obj_t obj = {0}, mp_id;
    ti_room_t * room;
    vec_t * whitelist = stream->via.user->whitelists[TI_WHITELIST_ROOMS];

    mp_unp_init(&up, pkg->data, pkg->n);

    mp_next(&up, &obj);     /* array with at least size 1 */
    mp_skip(&up);           /* scope */

    nargs = obj.via.sz - 1;

    if (mp_sbuffer_alloc_init(&buffer, size, sizeof(ti_pkg_t)))
    {
        ex_set_mem(e);
        return NULL;
    }

    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);
    msgpack_pack_array(&pk, nargs);

    uv_mutex_lock(collection->lock);

    for (i = 0; i < nargs; ++i)
    {
        if (mp_next(&up, &mp_id) == MP_STR)
        {
            room = ti_collection_room_by_strn(
                collection,
                mp_id.via.str.data,
                mp_id.via.str.n);
        }
        else if (mp_cast_u64(&mp_id) == 0)
        {
            room = ti_collection_room_by_id(collection, mp_id.via.u64);
        }
        else
        {
            ex_set(e, EX_BAD_DATA,
                "join request only accepts integer room id's "
                "or string room names"DOC_LISTENING);
            break;
        }

        if (room &&
            ti_whitelist_check(whitelist, room->name) == 0 &&
            ti_room_join(room, stream) == 0)
            msgpack_pack_uint64(&pk, room->id);
        else
        {
            if (room && nargs == 1)
            {
                if (room->name)
                    ex_set(e, EX_FORBIDDEN,
                        "no match in whitelist for room `%.*s`",
                        room->name->n,
                        room->name->str);

                else
                    ex_set(e, EX_FORBIDDEN,
                        "no match in whitelist for "TI_ROOM_ID,
                        room->id);
            }
            else
                /* do not return with an error with multiple rooms to join */
                msgpack_pack_nil(&pk);
        }
    }

    uv_mutex_unlock(collection->lock);

    if (e->nr)
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    resp = (ti_pkg_t *) buffer.data;
    pkg_init(resp, pkg->id, TI_PROTO_CLIENT_RES_DATA, buffer.size);

    return resp;
}

ti_pkg_t * ti_collection_leave_rooms(
        ti_collection_t * collection,
        ti_stream_t * stream,
        ti_pkg_t * pkg,
        ex_t * e)
{
    ti_pkg_t * resp;
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    size_t i, nargs, size = pkg->n + sizeof(ti_pkg_t);
    mp_unp_t up;
    mp_obj_t obj = {0}, mp_id;
    ti_room_t * room;

    mp_unp_init(&up, pkg->data, pkg->n);

    mp_next(&up, &obj);     /* array with at least size 1 */
    mp_skip(&up);           /* scope */

    nargs = obj.via.sz - 1;

    if (mp_sbuffer_alloc_init(&buffer, size, sizeof(ti_pkg_t)))
    {
        ex_set_mem(e);
        return NULL;
    }

    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);
    msgpack_pack_array(&pk, nargs);

    uv_mutex_lock(collection->lock);

    for (i = 0; i < nargs; ++i)
    {
        if (mp_next(&up, &mp_id) == MP_STR)
        {
            room = ti_collection_room_by_strn(
                collection,
                mp_id.via.str.data,
                mp_id.via.str.n);
        }
        else if (mp_cast_u64(&mp_id) == 0)
        {
            room = ti_collection_room_by_id(collection, mp_id.via.u64);
        }
        else
        {
            ex_set(e, EX_BAD_DATA,
                "leave request only accepts integer room id's "
                "or string room names"DOC_LISTENING);
            break;
        }

        if (room && ti_room_leave(room, stream) == 0)
            msgpack_pack_uint64(&pk, room->id);
        else
            msgpack_pack_nil(&pk);
    }

    uv_mutex_unlock(collection->lock);

    if (e->nr)
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    resp = (ti_pkg_t *) buffer.data;
    pkg_init(resp, pkg->id, TI_PROTO_CLIENT_RES_DATA, buffer.size);

    return resp;
}

/*
 * When the return value (and thus e->nr) is EX_BAD_DATA or EX_SUCCESS, the
 * collection is changed and therefore the same changes must be applied to all
 * nodes. Any other error indicates no changes are made.
 */
int ti_collection_unpack(
        ti_collection_t * collection,
        mp_unp_t * up,
        ex_t * e)
{
    mp_obj_t obj, mp_schema, mp_version;
    char * version = NULL;
    size_t i;

    if (mp_next(up, &obj) != MP_ARR || obj.via.sz == 0 ||
        mp_next(up, &mp_schema) != MP_U64 ||
        mp_next(up, &mp_version) != MP_STR)
    {
        ex_set(e, EX_VALUE_ERROR,
                "no valid import data; "
                "make sure the bytes are created using the `export` function "
                "with the `dump` option enabled");
        return e->nr;
    }

    if (mp_schema.via.u64 != TI_EXPORT_SCHEMA_V1500)
    {
        ex_set(e, EX_VALUE_ERROR,
                "export with unknown schema `%"PRIu64"`",
                mp_schema.via.u64);
        return e->nr;
    }

    version = mp_strdup(&mp_version);
    if (!version)
    {
        ex_set_mem(e);
        return e->nr;
    }

    if (ti_version_cmp(version, TI_VERSION) > 0)
    {
        ex_set(e, EX_VALUE_ERROR,
                "export is created using ThingsDB version `%s` which is newer "
                "than the running version `"TI_VERSION"`",
                version);
    }
    else for (i = obj.via.sz-2; i--;)
    {
        if (ti_ctask_run(collection->root, up))
        {
            ex_set(e, EX_BAD_DATA,
                    "failed to unpack collection data; "
                    "(more info might be available in the node log)");
            break;
        }
    }

    free(version);
    return e->nr;
}

void ti_collection_tasks_clear(ti_collection_t * collection)
{
    for (vec_each(collection->vtasks, ti_vtask_t, vtask))
        ti_vtask_del(vtask->id, collection);
}

/*
 * Shortcut to ti_collection_unpack() with bytes as input.
 *
 * When the return value (and thus e->nr) is EX_BAD_DATA or EX_SUCCESS, the
 * collection is changed and therefore the same changes must be applied to all
 * nodes. Any other error indicates no changes are made.
 */
int ti_collection_load(
        ti_collection_t * collection,
        ti_raw_t * bytes,
        ex_t * e)
{
    mp_unp_t up;
    mp_unp_init(&up, bytes->data, bytes->n);

    return ti_collection_unpack(collection, &up, e);
}
