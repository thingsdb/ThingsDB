#include <ti/node.t.h>
#include <ex.h>

static void timer__async_cb(uv_async_t * async)
{
    ti_timer_t * timer = async->data;
    ex_t e = {0};
    ti_node_t * this_node = ti.node;
    ti_pkg_t * resp = NULL;
    ti_query_t * query = NULL;
    ti_scope_t scope;
    vec_t * access_;
    ti_collection_t * collection;

    uv_close((uv_handle_t *) async, free);

    if (this_node->status < TI_NODE_STAT_READY &&
        this_node->status != TI_NODE_STAT_SHUTTING_DOWN)
    {
        ti_timer_fwd(timer);
        ti_timer_drop(timer);
        return;
    }

    ti_scope_load_from_scope_id(timer->scope_id, &access_, &collection);
    if (!access_)
    {
        ti_timer_ex_set(timer, EX_INTERNAL,
                "invalid scope id: %"PRIu64,
                timer->scope_id);
        ti_timer_drop(timer);
        return;
    }

    query = ti_query_create(0);
    if (!query)
    {
        ti_timer_ex_set(timer, EX_MEMORY, EX_MEMORY_S);
        ti_timer_drop(timer);
        return;
    }

    query->user = timer->user;
    query->with_tp = TI_QUERY_WITH_TIMER;
    query->pkg_id = 0;
    query->with.timer = timer;          /* move the timer reference */
    query->collection = collection;     /* may be NULL */

    if (timer->closure->flags & TI_CLOSURE_FLAG_WSE)
    {
        query->qbind.flags |= TI_QBIND_FLAG_EVENT;
        query->flags |= TI_QUERY_FLAG_WSE;
    }

    if (ti_access_check_err(access_, query->user, TI_AUTH_EVENT, &e))
        goto finish;

    if (ti_query_will_update(query))
    {
        if (ti_events_create_new_event(query, &e))
            goto finish;
        return;
    }

    ti_query_run_timer(query);
    return;

finish:
    ti_query_destroy(query);
    if (e.nr)
    {
        ++ti.counters->queries_with_error;
        ti_timer_ex_cpy(timer, &e);
    }
}

void ti_timer_run(ti_timer_t * timer)
{
    uv_async_t * async = malloc(sizeof(uv_async_t));
    if (!async)
    {
        ti_timer_ex_set(timer, EX_MEMORY, EX_MEMORY_S);
        return;
    }

    ti_incref(timer);
    async->data = timer;

    if (uv_async_init(ti.loop, async, (uv_async_cb) timer__async_cb))
    {
        ti_timer_ex_set(timer, EX_INTERNAL, EX_INTERNAL_S);
        ti_timer_drop(timer);
        free(async);
    }
}

void ti_timer_fwd(ti_timer_t * timer)
{
    ti_node_t * node = ti_nodes_random_ready_node();
    if (!node)
    {
        ti_timer_ex_set(timer, EX_NODE_ERROR,
                "cannot find a node for running timer `%s` (%"PRIu64")",
                timer->name ? timer->name->str : "anonymous",
                timer->id);
        return;
    }
}

static ti_rpkg_t * timer__rpkg_e(ti_timer_t * timer)
{
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_pkg_t * pkg;
    ti_rpkg_t * rpkg;
    ti_node_t * ti_node = ti.node;

    if (mp_sbuffer_alloc_init(&buffer, timer->e->n + 28, sizeof(ti_pkg_t)))
        return NULL;

    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    (void) msgpack_pack_array(&pk, 3);
    (void) msgpack_pack_uint64(&pk, timer->id);
    (void) msgpack_pack_int(&pk, (int) timer->e->nr);
    (void) mp_pack_strn(&pk, timer->e->msg, timer->e->n);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_NODE_INFO, buffer.size);

    rpkg = ti_rpkg_create(pkg);
    if (!rpkg)
        free(pkg);

    return rpkg;
}

void ti_timer_broadcast_e(ti_timer_t * timer)
{
    vec_t * nodes_vec = ti.nodes->vec;
    ti_node_t * this_node = ti.node;
    ti_rpkg_t * rpkg = timer__rpkg_e(timer);

    if (!rpkg)
    {
        log_error(EX_MEMORY_S);
        return;
    }

    for (vec_each(nodes_vec, ti_node_t, node))
        if (node != this_node && (node->status & (
                TI_NODE_STAT_SYNCHRONIZING |
                TI_NODE_STAT_AWAY |
                TI_NODE_STAT_AWAY_SOON |
                TI_NODE_STAT_READY)) &&
                ti_stream_write_rpkg(node->stream, rpkg))
            log_error(EX_INTERNAL_S);

    ti_rpkg_drop(rpkg);
}

void ti_timer_ex_set(
        ti_timer_t * timer,
        ex_enum errnr,
        const char * errmsg,
        ...)
{
    va_list args;

    if (!timer->e && !(timer->e = malloc(sizeof(ex_t))))
    {
        log_error(EX_MEMORY_S);
        return;
    }

    va_start(args, errmsg);
    ex_setv(timer->e, errnr, errmsg, args);
    va_end(args);

    ti_timer_broadcast_e(timer);
}

void ti_timer_ex_cpy(ti_timer_t * timer, ex_t * e)
{
    if (!timer->e && !(timer->e = malloc(sizeof(ex_t))))
    {
        log_error(EX_MEMORY_S);
        return;
    }

    memcpy(timer->e, e, sizeof(ex_t));
    ti_timer_broadcast_e(timer);
}
