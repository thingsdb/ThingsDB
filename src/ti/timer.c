/*
 * ti/timer.c
 */
#include <assert.h>
#include <ex.h>
#include <ti.h>
#include <ti/access.h>
#include <ti/auth.h>
#include <ti/name.h>
#include <ti/node.t.h>
#include <ti/node.t.h>
#include <ti/pkg.t.h>
#include <ti/query.h>
#include <ti/query.t.h>
#include <ti/rpkg.t.h>
#include <ti/scope.h>
#include <ti/timer.h>
#include <ti/timer.inline.h>
#include <ti/timer.t.h>
#include <ti/user.h>
#include <ti/vint.h>
#include <ti/val.inline.h>

ti_timer_t * ti_timer_create(
        uint64_t id,
        uint64_t next_run,
        uint32_t repeat,
        uint64_t scope_id,
        ti_user_t * user,
        ti_closure_t * closure,
        vec_t * args)
{
    ti_timer_t * timer = malloc(sizeof(ti_timer_t));
    if (!timer)
        return NULL;

    timer->ref = 1;
    timer->id = id;
    timer->next_run = next_run;
    timer->repeat = repeat;
    timer->scope_id = scope_id;
    timer->user = user;
    timer->closure = closure;
    timer->args = args;
    timer->doc = NULL;
    timer->def = NULL;

    if (user)
        ti_incref(user);
    ti_incref(closure);

    return timer;
}

void ti_timer_destroy(ti_timer_t * timer)
{
    ti_val_unsafe_drop((ti_val_t *) timer->closure);
    ti_user_drop(timer->user);
    vec_destroy(timer->args, (vec_destroy_cb) ti_val_unsafe_drop);
    free(timer);
}

static void timer__async_cb(uv_async_t * async)
{
    ti_timer_t * timer = async->data;
    ex_t e = {0};
    ti_node_t * this_node = ti.node;
    ti_query_t * query = NULL;
    vec_t * access_;
    ti_collection_t * collection;

    uv_close((uv_handle_t *) async, (uv_close_cb) free);

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
        ex_set(&e, EX_INTERNAL, "invalid scope id: %"PRIu64, timer->scope_id);
        ti_timer_done(timer, &e);
        ti_timer_drop(timer);
        return;
    }

    query = ti_query_create(0);
    if (!query)
    {
        ti_timer_ex_set(timer, EX_MEMORY, EX_MEMORY_S);
        ti_timer_done(timer);
        ti_timer_drop(timer);
        return;
    }

    query->user = timer->user;
    query->with_tp = TI_QUERY_WITH_TIMER;
    query->pkg_id = 0;
    query->with.timer = timer;          /* move the timer reference */
    query->collection = collection;     /* may be NULL */

    ti_incref(query->user);
    ti_incref(query->collection);

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
    if (e.nr)
    {
        ++ti.counters->timers_with_error;
        ti_timer_ex_set_from_e(timer, &e);
        ti_timer_done(timer);
    }
    ti_query_destroy(query);
}

/*
 * If this function is not able to start an a-sync call, an exception will
 * be set but `ti_timer_done(..)` will not be called.
 */
void ti_timer_run(ti_timer_t * timer)
{
    uv_async_t * async = malloc(sizeof(uv_async_t));
    if (!async)
    {
        ti_timer_ex_set(timer, EX_MEMORY, EX_MEMORY_S);
        return;
    }

    ti_incref(timer);

    if (!timer->repeat)
        timer->next_run = 0;

    async->data = timer;

    if (uv_async_init(ti.loop, async, (uv_async_cb) timer__async_cb) ||
        uv_async_send(async))
    {
        ti_timer_ex_set(timer, EX_INTERNAL, EX_INTERNAL_S);
        ti_timer_drop(timer);
        free(async);
        return;
    }
}

/*
 * If this function is not able to forward the timer, an exception will
 * be set but `ti_timer_done(..)` will not be called.
 */
void ti_timer_fwd(ti_timer_t * timer)
{
    ti_node_t * node = ti_nodes_random_ready_node();
    if (!node)
    {
        ti_timer_ex_set(timer, EX_NODE_ERROR,
                "cannot find a node for running timer (ID: %"PRIu64")",
                timer->id);
        return;
    }
}

void ti_timer_mark_del(ti_timer_t * timer)
{
    ti_user_drop(timer->user);
    timer->user = NULL;
}

static ti_rpkg_t * timer__rpkg_ex(ti_timer_t * timer)
{
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_pkg_t * pkg;
    ti_rpkg_t * rpkg;

    if (mp_sbuffer_alloc_init(&buffer, timer->e->n + 48, sizeof(ti_pkg_t)))
        return NULL;

    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    (void) msgpack_pack_array(&pk, 5);
    (void) msgpack_pack_uint64(&pk, timer->scope_id);
    (void) msgpack_pack_uint64(&pk, timer->id);
    (void) msgpack_pack_uint64(&pk, timer->next_run);
    (void) msgpack_pack_fix_int8(&pk, (int8_t) timer->e->nr);
    (void) mp_pack_strn(&pk, timer->e->msg, timer->e->n);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_NODE_EX_TIMER, buffer.size);

    rpkg = ti_rpkg_create(pkg);
    if (!rpkg)
        free(pkg);

    return rpkg;
}

static ti_rpkg_t * timer__rpkg_ok(ti_timer_t * timer)
{
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_pkg_t * pkg;
    ti_rpkg_t * rpkg;

    if (mp_sbuffer_alloc_init(&buffer, 64, sizeof(ti_pkg_t)))
        return NULL;

    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    (void) msgpack_pack_array(&pk, 3);
    (void) msgpack_pack_uint64(&pk, timer->scope_id);
    (void) msgpack_pack_uint64(&pk, timer->id);
    (void) msgpack_pack_uint64(&pk, timer->next_run);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_NODE_EX_TIMER, buffer.size);

    rpkg = ti_rpkg_create(pkg);
    if (!rpkg)
        free(pkg);

    return rpkg;
}

static void timer__broadcast_rpkg(ti_rpkg_t * rpkg)
{
    vec_t * nodes_vec = ti.nodes->vec;
    ti_node_t * this_node = ti.node;

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
}

void ti_timer_ex_set_from_e(ti_timer_t * timer, ex_t * e)
{
    if (!timer->e && !(timer->e = malloc(sizeof(ex_t))))
    {
        log_error(EX_MEMORY_S);
        return;
    }

    memcpy(timer->e, e, sizeof(ex_t));
}

void ti_timer_done(ti_timer_t * timer, ex_t * e)
{
    ti_rpkg_t * rpkg;

    if (timer->repeat)
        timer->next_run += timer->repeat;
    else if (!timer->next_run)
        ti_timer_mark_del(timer);

    rpkg = (timer->e && timer->e->nr)
            ? timer__rpkg_ex(timer)
            : timer__rpkg_ok(timer);

    if (!rpkg)
    {
        log_error(EX_MEMORY_S);
        return;
    }

    timer__broadcast_rpkg(rpkg);
}

ti_timer_t * ti_timer_from_val(vec_t * timers, ti_val_t * val, ex_t * e)
{
    if (ti_val_is_int(val))
    {
        uint64_t id = (uint64_t) VINT(val);
        for (vec_each(timers, ti_timer_t, timer))
            if (timer->id == id)
                return timer;
        ex_set(e, EX_LOOKUP_ERROR, TI_TIMER_ID" not found", id);
    }
    else
        ex_set(e, EX_TYPE_ERROR,
                "expecting type `"TI_VAL_STR_S"` "
                "or `"TI_VAL_INT_S"` as timer but got type `%s` instead",
                ti_val_str(val));
    return NULL;
}
