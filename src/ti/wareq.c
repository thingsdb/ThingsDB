/*
 * ti/wareq.c
 */
#include <ti/wareq.h>
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/collections.h>
#include <ti/proto.h>
#include <ti/fwd.h>
#include <ti/auth.h>
#include <ti/access.h>
#include <util/qpx.h>

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
    qp_unpacker_t unpacker;
    qp_obj_t qp_id;

    qp_unpacker_init2(&unpacker, pkg->data, pkg->n, 0);

    qp_next(&unpacker, NULL);
    qp_next(&unpacker, NULL);

    wareq->thing_ids = vec_new(4);
    if (!wareq->thing_ids)
    {
        ex_set_mem(e);
        return e->nr;
    }

    while(qp_is_int(qp_next(&unpacker, &qp_id)))
    {
        #if TI_USE_VOID_POINTER
        uintptr_t id = (uintptr_t) qp_id.via.int64;
        #else
        uint64_t * id = malloc(sizeof(uint64_t));
        if (!id)
        {
            ex_set_mem(e);
            return e->nr;
        }
        *id = (uint64_t) val.via.int64;
        #endif
        if (vec_push(&wareq->thing_ids, (void *) id))
        {
            ex_set_mem(e);
            return e->nr;
        }
    }

    if (!qp_is_close(qp_id.tp))
        ex_set(e, EX_BAD_DATA,
                "watch requests only except integer thing id's "DOC_WATCH);

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
        wareq__destroy(wareq);
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
        qpx_packer_t * packer;

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

        packer = qpx_packer_create(512, 4);
        if (!packer)
        {
            log_critical(EX_MEMORY_S);
            break;
        }

        (void) qp_add_map(&packer);
        (void) qp_add_raw_from_str(packer, "event");
        (void) qp_add_int(packer, ti()->node->cevid);
        (void) qp_add_raw_from_str(packer, "thing");

        /* fetch exactly one level, options = 1 */
        if (    ti_thing_to_packer(thing, &packer, 1 /* options */) ||
                qp_close_map(packer))
        {
            log_critical(EX_MEMORY_S);
            qp_packer_destroy(packer);
            break;
        }

        pkg = qpx_packer_pkg(packer, TI_PROTO_CLIENT_WATCH_INI);

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
