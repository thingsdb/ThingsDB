/*
 * ti/wareq.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/access.h>
#include <ti/auth.h>
#include <ti/collections.h>
#include <ti/fwd.h>
#include <ti/proto.h>
#include <ti/thing.inline.h>
#include <ti/wareq.h>
#include <util/mpack.h>

static void wareq__destroy(ti_wareq_t * wareq);
static void wareq__destroy_cb(uv_async_t * task);
static void wareq__watch_cb(uv_async_t * task);
static void wareq__unwatch_cb(uv_async_t * task);

/*
 * Create a watch or unwatch request.
 *   Argument `task` should be either "watch" or "unwatch" (watch / unwatch)
 */
static ti_wareq_t * wareq__create(
        ti_stream_t * stream,
        ti_collection_t * collection,
        const char * task)
{
    ti_wareq_t * wareq = malloc(sizeof(ti_wareq_t));
    if (!wareq)
        return NULL;

    wareq->stream = ti_grab(stream);
    wareq->thing_ids = NULL;
    wareq->collection = collection;
    wareq->task = malloc(sizeof(uv_async_t));

    ti_incref(collection);

    if (!wareq->task || uv_async_init(
            ti()->loop,
            wareq->task,
            *task == 'w' ? wareq__watch_cb : wareq__unwatch_cb))
    {
        wareq__destroy(wareq);
        return NULL;
    }

    wareq->task->data = wareq;

    return wareq;
}

static int wareq__unpack(ti_wareq_t * wareq, ti_pkg_t * pkg, ex_t * e)
{
    mp_unp_t up;
    mp_obj_t obj, mp_id;
    size_t nargs;
    size_t i;

    mp_unp_init(&up, pkg->data, pkg->n);

    mp_next(&up, &obj);     /* array with at least size 1 */
    mp_skip(&up);           /* scope */

    nargs = obj.via.sz - 1;

    wareq->thing_ids = vec_new(nargs);
    if (!wareq->thing_ids)
    {
        ex_set_mem(e);
        return e->nr;
    }

    for (i = 0; i < nargs; ++i)
    {
        if (!mp_may_cast_u64(mp_next(&up, &mp_id)))
        {
            ex_set(e, EX_BAD_DATA,
                    "watch requests only excepts integer thing id's "DOC_WATCHING);
            return e->nr;
        }

        #if TI_USE_VOID_POINTER
        uintptr_t id = (uintptr_t) mp_id.via.u64;
        #else
        uint64_t * id = malloc(sizeof(uint64_t));
        if (!id)
        {
            ex_set_mem(e);
            return e->nr;
        }
        *id = (uint64_t) val.via.int64;
        #endif

        VEC_push(wareq->thing_ids, (void *) id);
    }

    return e->nr;
}

/*
 * Watch requests are only created for collection scopes so the return value
 * may be NULL. Make sure to test `e` for errors.
 */
ti_wareq_t * ti_wareq_may_create(
        ti_scope_t * scope,
        ti_stream_t * stream,
        ti_pkg_t * pkg,
        const char * task,
        ex_t * e)
{
    ti_collection_t * collection = NULL;
    ti_user_t * user = stream->via.user;
    ti_wareq_t * wareq;

    switch (scope->tp)
    {
    case TI_SCOPE_COLLECTION_NAME:
        collection = ti_collections_get_by_strn(
                scope->via.collection_name.name,
                scope->via.collection_name.sz);

        if (!collection)
        {
            ex_set(e, EX_LOOKUP_ERROR, "collection `%.*s` not found",
                (int) scope->via.collection_name.sz,
                scope->via.collection_name.name);
            return NULL;
        }
        break;
    case TI_SCOPE_COLLECTION_ID:
        collection = ti_collections_get_by_id(scope->via.collection_id);
        if (!collection)
        {
            ex_set(e, EX_LOOKUP_ERROR, TI_COLLECTION_ID" not found",
                    scope->via.collection_id);
            return NULL;
        }
        break;
    case TI_SCOPE_NODE:
        if (ti_access_check_err(ti()->access_node, user, TI_AUTH_WATCH, e))
            return NULL;

        if (scope->via.node_id != ti()->node->id)
            ex_set(e, EX_LOOKUP_ERROR,
                    "watch request to a `@node` scope are only allowed to "
                    "the node the client is connected to; change the scope "
                    "to simply `@n` if this is what you want");

        return NULL;
    case TI_SCOPE_THINGSDB:
        ex_set(e, EX_LOOKUP_ERROR,
                "watch request to the `@thingsdb` scope are not possible");
        return NULL;
    }

    assert (collection);

    if (ti_access_check_err(collection->access, user, TI_AUTH_WATCH, e))
        return NULL;

    wareq = wareq__create(stream, collection, task);
    if (!wareq)
    {
        ex_set_mem(e);
        return NULL;
    }

    if (wareq__unpack(wareq, pkg, e))
    {
        ti_wareq_destroy(wareq);
        return NULL;
    }

    return wareq;
}

void ti_wareq_destroy(ti_wareq_t * wareq)
{
    if (!wareq)
        return;
    assert (wareq->task);
    uv_close((uv_handle_t *) wareq->task, (uv_close_cb) wareq__destroy_cb);
}

int ti_wareq_init(ti_stream_t * stream)
{
    if (ti_stream_is_closed(stream))
        return -1;

    if (!stream->watching)
    {
        /*
         * This is the first watch request for this client, add listening for
         * thing0
         */
        ti_watch_t * watch = ti_thing_watch(ti()->thing0, stream);
        if (!watch)
        {
            log_error(EX_MEMORY_S);
            /*
             * This is not critical, it does however mean that the client will
             * not receive broadcast messages
             */
        }
    }

    return 0;
}

int ti_wareq_run(ti_wareq_t * wareq)
{
    return uv_async_send(wareq->task);
}

static void wareq__destroy(ti_wareq_t * wareq)
{
    #if TI_USE_VOID_POINTER
    vec_destroy(wareq->thing_ids, NULL);
    #else
    vec_destroy(wareq->thing_ids, free);
    #endif
    ti_stream_drop(wareq->stream);
    ti_collection_drop(wareq->collection);
    free(wareq->task);
    free(wareq);
}

static void wareq__destroy_cb(uv_async_t * task)
{
    ti_wareq_t * wareq = task->data;
    wareq__destroy(wareq);
}

static void wareq__watch_cb(uv_async_t * task)
{
    ti_wareq_t * wareq = task->data;
    ti_thing_t * thing;
    ti_watch_t * watch;
    uint32_t n;

    if (!wareq->collection || !wareq->thing_ids)
    {
        ti_wareq_destroy(wareq);
        return;
    }

    n = wareq->thing_ids->n;

    uv_mutex_lock(wareq->collection->lock);

    while (n--)
    {
        ti_pkg_t * pkg;
        msgpack_packer pk;
        msgpack_sbuffer buffer;

        #if TI_USE_VOID_POINTER
        uintptr_t id = (uintptr_t) vec_pop(wareq->thing_ids);
        #else
        uint64_t * idp = vec_pop(wareq->thing_ids);
        uint64_t id = *idp;
        free(idp);
        #endif

        thing = imap_get(wareq->collection->things, id);

        if (!thing)
            continue;

        watch = ti_thing_watch(thing, wareq->stream);
        if (!watch)
        {
            log_critical(EX_MEMORY_S);
            break;
        }

        if (mp_sbuffer_alloc_init(&buffer, 8192, sizeof(ti_pkg_t)))
        {
            log_critical(EX_MEMORY_S);
            break;
        }
        msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

        msgpack_pack_map(&pk, 2);

        mp_pack_str(&pk, "event");
        msgpack_pack_uint64(&pk, ti()->node->cevid);

        mp_pack_str(&pk, "thing");
        /* fetch exactly one level, options = 1 */
        if (ti_thing_to_pk(thing, &pk, 1 /* options */))
        {
            log_critical(EX_MEMORY_S);
            msgpack_sbuffer_destroy(&buffer);
            break;
        }

        pkg = (ti_pkg_t *) buffer.data;
        pkg_init(pkg, TI_PROTO_EV_ID, TI_PROTO_CLIENT_WATCH_INI, buffer.size);

        if (    ti_stream_is_closed(wareq->stream) ||
                ti_stream_write_pkg(wareq->stream, pkg))
        {
            free(pkg);
        }
    }

    uv_mutex_unlock(wareq->collection->lock);

    ti_wareq_destroy(wareq);
}

static void wareq__unwatch_cb(uv_async_t * task)
{
    ti_wareq_t * wareq = task->data;
    ti_thing_t * thing;
    uint32_t n = wareq->thing_ids->n;

    if (!n ||
        ti_stream_is_closed(wareq->stream) ||
        !wareq->stream->watching ||
        !wareq->collection)
    {
        ti_wareq_destroy(wareq);
        return;
    }

    uv_mutex_lock(wareq->collection->lock);

    while (n--)
    {
        #if TI_USE_VOID_POINTER
        uintptr_t id = (uintptr_t) vec_pop(wareq->thing_ids);
        #else
        uint64_t * idp = vec_pop(wareq->thing_ids);
        uint64_t id = *idp;
        free(idp);
        #endif

        thing = imap_get(wareq->collection->things, id);
        if (thing)
            (void) ti_thing_unwatch(thing, wareq->stream);
    }

    uv_mutex_unlock(wareq->collection->lock);

    ti_wareq_destroy(wareq);
}
