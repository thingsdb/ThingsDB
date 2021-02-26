/*
 * ti/wareq.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/access.h>
#include <ti/auth.h>
#include <ti/collection.inline.h>
#include <ti/collections.h>
#include <ti/fwd.h>
#include <ti/proto.h>
#include <ti/thing.inline.h>
#include <ti/wareq.h>
#include <ti/warn.h>
#include <util/mpack.h>

static void wareq__destroy(ti_wareq_t * wareq);
static void wareq__destroy_cb(uv_async_t * task);
static void wareq__watch_cb(uv_async_t * task);
static void wareq__unwatch_cb(uv_async_t * task);

/*
 * On systems with a pointer size of 32-bit, space will be allocated for each
 * thing ID in the watch request. With a 64-bit pointer size, the thing ID is
 * stored in the pointer.
 */
static inline void wareq__vec_destroy(vec_t * vec)
{
    #if TI_IS64BIT
    vec_destroy(vec, NULL);
    #else
    vec_destroy(vec, free);
    #endif
}

/*
 * Create a watch or unwatch request.
 *   Argument `task` should be either "watch" or "unwatch" (watch / unwatch)
 */
ti_wareq_t * ti_wareq_create(
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
            ti.loop,
            (uv_async_t *) wareq->task,
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
        void * idp;
        if (mp_next(&up, &mp_id) <= 0 || mp_cast_u64(&mp_id))
        {
            ex_set(e, EX_BAD_DATA,
                    "watch requests only excepts integer thing id's "DOC_WATCHING);
            return e->nr;
        }

        idp = ti_wareq_id(mp_id.via.u64);
        VEC_push(wareq->thing_ids, idp);
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
                scope->via.collection_name.sz,
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
        if (ti_access_check_err(ti.access_node, user, TI_AUTH_WATCH, e))
            return NULL;

        if (scope->via.node_id != ti.node->id)
            ex_set(e, EX_LOOKUP_ERROR,
                    "watching a `@node` scope is only allowed to "
                    "the node the client is connected to; change the scope "
                    "to simply `@n` if this is what you want");

        return NULL;
    case TI_SCOPE_THINGSDB:
        ex_set(e, EX_LOOKUP_ERROR,
                "it is not possible to watch the `@thingsdb` scope");
        return NULL;
    }

    assert (collection);

    if (ti_access_check_err(collection->access, user, TI_AUTH_WATCH, e))
        return NULL;

    wareq = ti_wareq_create(stream, collection, task);
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

int ti_wareq_run(ti_wareq_t * wareq)
{
    return uv_async_send((uv_async_t *) wareq->task);
}

static void wareq__destroy(ti_wareq_t * wareq)
{
    wareq__vec_destroy(wareq->thing_ids);
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
    uint32_t n;
    vec_t * vec = NULL;
    _Bool reschedule;

    if (!wareq->collection || !wareq->thing_ids)
    {
        ti_wareq_destroy(wareq);
        return;
    }

    n = wareq->thing_ids->n;

    uv_mutex_lock(wareq->collection->lock);

    /*
     * When this node is in away mode and the queue contains events, it may
     * be that the client has received new ID's which are not yet processed
     * by this node; In this case we should reschedule when ID's are missing;
     */
    reschedule = ti_events_in_queue() && ti_away_is_busy();

    while (n--)
    {
        #if TI_IS64BIT
        uintptr_t id = (uintptr_t) vec_pop(wareq->thing_ids);
        #else
        uint64_t * idp = vec_pop(wareq->thing_ids);
        uint64_t id = *idp;
        free(idp);
        #endif

        thing = ti_collection_thing_by_id(wareq->collection, id);

        if (!thing)
        {
            if (reschedule)
            {
                void * idp = ti_wareq_id(id);
                if (vec_push_create(&vec, idp))
                    log_critical(EX_MEMORY_S);
                continue;
            }

            ti_warn(wareq->stream,
                    TI_WARN_THING_NOT_FOUND,
                    "failed to watch thing "TI_THING_ID", "
                    "it is not found in collection `%.*s`",
                    id,
                    wareq->collection->name->n,
                    (char *) wareq->collection->name->data);
            continue;
        }

        if (ti_thing_watch_init(thing, wareq->stream))
        {
            log_critical(EX_MEMORY_S);
            break;
        }
    }

    uv_mutex_unlock(wareq->collection->lock);

    if (vec)
    {
        if (ti.node->status != TI_NODE_STAT_SHUTTING_DOWN)
        {
            wareq__vec_destroy(wareq->thing_ids);
            wareq->thing_ids = vec;

            if (wareq->task->type == UV_TIMER)
                return;

            log_debug(
                "start watch retry procedure for %"PRIu32" missing thing%s "
                "for stream `%s`",
                vec->n, vec->n == 1 ? "" : "s",
                ti_stream_name(wareq->stream));

            uv_close((uv_handle_t *) wareq->task, (uv_close_cb) free);

            wareq->task = malloc(sizeof(uv_timer_t));
            if (!wareq->task)
            {
                log_critical(EX_MEMORY_S);
                return;  /* may leak a few bytes */
            }

            wareq->task->data = wareq;

            if (uv_timer_init(ti.loop, (uv_timer_t *) wareq->task) ||
                uv_timer_start(
                        (uv_timer_t *) wareq->task,
                        (uv_timer_cb) wareq__watch_cb,
                        250, 250))
            {
                log_critical(EX_INTERNAL_S);
            }

            return;
        }
        wareq__vec_destroy(vec);
    }

    if (wareq->task->type == UV_TIMER)
    {
        log_debug(
                "stop watch retry procedure for missing things "
                "for stream `%s`", ti_stream_name(wareq->stream));
        uv_timer_stop((uv_timer_t *) task);
    }

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
        #if TI_IS64BIT
        uintptr_t id = (uintptr_t) vec_pop(wareq->thing_ids);
        #else
        uint64_t * idp = vec_pop(wareq->thing_ids);
        uint64_t id = *idp;
        free(idp);
        #endif

        thing = ti_collection_thing_by_id(wareq->collection, id);
        if (thing)
            (void) ti_thing_unwatch(thing, wareq->stream);
    }

    uv_mutex_unlock(wareq->collection->lock);

    ti_wareq_destroy(wareq);
}
