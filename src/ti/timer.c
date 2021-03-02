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

#define TIMER__FWD_AGAIN 120

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
    ti_val_drop((ti_val_t *) timer->doc);
    ti_val_drop((ti_val_t *) timer->def);

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

    if (this_node->status != TI_NODE_STAT_READY)
    {
        (void) ti_timer_fwd(timer);
        ti_timer_drop(timer);
        return;
    }

    if (!timer->repeat)
        timer->next_run = 0;

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
        ex_set(&e, EX_MEMORY, EX_MEMORY_S);
        ti_timer_done(timer, &e);
        ti_timer_drop(timer);
        return;
    }

    query->user = timer->user;
    query->with_tp = TI_QUERY_WITH_TIMER;
    query->pkg_id = 0;
    query->with.timer = timer;          /* move the timer reference */
    query->collection = collection;     /* may be NULL */
    query->qbind.flags |= (timer->scope_id == TI_SCOPE_THINGSDB)
        ? TI_QBIND_FLAG_THINGSDB
        : TI_QBIND_FLAG_COLLECTION;

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
        ti_timer_done(timer, &e);
    }
    ti_query_destroy(query);
}

/*
 * If this function is not able to start an a-sync call, an exception will
 * be set but `ti_timer_done(..)` will not be called.
 */
int ti_timer_run(ti_timer_t * timer)
{
    uv_async_t * async = malloc(sizeof(uv_async_t));
    if (!async)
    {
        log_error(EX_MEMORY_S);
        return -1;
    }

    ti_incref(timer);

    async->data = timer;

    if (uv_async_init(ti.loop, async, (uv_async_cb) timer__async_cb) ||
        uv_async_send(async))
    {
        log_error(EX_INTERNAL_S);
        ti_timer_drop(timer);
        free(async);
        return -1;
    }

    return 0;
}

/*
 * If this function is not able to forward the timer, an exception will
 * be set but `ti_timer_done(..)` will not be called.
 */
int ti_timer_fwd(ti_timer_t * timer)
{

    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_pkg_t * pkg;
    ti_node_t * node = ti_nodes_random_ready_node();
    if (!node)
    {
        log_error(
            "cannot find a ready node for starting timer %"PRIu64,
            timer->id);
        return -1;
    }

    if (mp_sbuffer_alloc_init(&buffer, 32, sizeof(ti_pkg_t)))
    {
        log_error(EX_MEMORY_S);
        return -1;
    }
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);
    msgpack_pack_uint64(&pk, timer->scope_id);
    msgpack_pack_uint64(&pk, timer->id);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_NODE_FWD_TIMER, buffer.size);

    if (ti_stream_write_pkg(node->stream, pkg))
    {
        free(pkg);
        log_error("failed to forward timer package");
        return -1;
    }

    timer->next_run += timer->repeat ? timer->repeat : TIMER__FWD_AGAIN;
    return 0;
}

void ti_timer_mark_del(ti_timer_t * timer)
{
    ti_user_drop(timer->user);
    timer->user = NULL;
}

/* may return an empty string but never NULL */
ti_raw_t * ti_timer_doc(ti_timer_t * timer)
{
    if (!timer->doc)
        timer->doc = ti_closure_doc(timer->closure);
    return timer->doc;
}

/* may return an empty string but never NULL */
ti_raw_t * ti_timer_def(ti_timer_t * timer)
{
    if (!timer->def)
        /* calculate only once */
        timer->def = ti_closure_def(timer->closure);
    return timer->def;
}

static ti_rpkg_t * timer__rpkg_ex(ti_timer_t * timer, ex_t * e)
{
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_pkg_t * pkg;
    ti_rpkg_t * rpkg;

    if (mp_sbuffer_alloc_init(&buffer, e->n + 48, sizeof(ti_pkg_t)))
        return NULL;

    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    (void) msgpack_pack_array(&pk, 5);
    (void) msgpack_pack_uint64(&pk, timer->scope_id);
    (void) msgpack_pack_uint64(&pk, timer->id);
    (void) msgpack_pack_uint64(&pk, timer->next_run);
    (void) msgpack_pack_fix_int8(&pk, (int8_t) e->nr);
    (void) mp_pack_strn(&pk, e->msg, e->n);

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
    pkg_init(pkg, 0, TI_PROTO_NODE_OK_TIMER, buffer.size);

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

void ti_timer_done(ti_timer_t * timer, ex_t * e)
{
    ti_rpkg_t * rpkg;

    if (e->nr)
        ++ti.counters->timers_with_error;
    else
        ++ti.counters->timers_success;

    if (timer->repeat)
        timer->next_run += timer->repeat;
    else if (!timer->next_run)
        ti_timer_mark_del(timer);

    rpkg = (e->nr)
            ? timer__rpkg_ex(timer, e)
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
            if (timer->user && timer->id == id)
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

char * timer__next_run(ti_timer_t * timer)
{
    /* length 27 =  "2000-00-00 00:00:00+01.00Z" + 1 */
    static char buf[27];
    struct tm * tm_info;
    uint64_t now = util_now_usec();

    if (timer->next_run < now)
        return "pending";

    tm_info = gmtime((const time_t *) &timer->next_run);

    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%SZ", tm_info);
    return buf;
}


static int timer__info_to_pk(
        ti_timer_t * timer,
        msgpack_packer * pk,
        _Bool with_full_access)
{
    ti_raw_t * doc = ti_timer_doc(timer);
    ti_raw_t * def;
    size_t size = 5;

    if (with_full_access)
    {
        def = ti_timer_def(timer);
        size += 2;
    }

    if (timer->repeat)
        ++size;

    if (msgpack_pack_map(pk, size) ||

        mp_pack_str(pk, "doc") ||
        mp_pack_strn(pk, doc->data, doc->n) ||

        mp_pack_str(pk, "id") ||
        msgpack_pack_uint64(pk, timer->id) ||

        mp_pack_str(pk, "next_run") ||
        mp_pack_str(pk, timer__next_run(timer)) ||

        mp_pack_str(pk, "with_side_effects") ||
        mp_pack_bool(pk, timer->closure->flags & TI_CLOSURE_FLAG_WSE))
        return -1;

    if (mp_pack_str(pk, "arguments") ||
        msgpack_pack_array(pk, timer->closure->vars->n))
        return -1;

    for (vec_each(timer->closure->vars, ti_prop_t, prop))
        if (mp_pack_str(pk, prop->name->str))
            return -1;

    if (timer->repeat && (
        mp_pack_str(pk, "repeat") ||
        msgpack_pack_uint64(pk, timer->repeat)))
        return -1;

    if (with_full_access && (
        mp_pack_str(pk, "user") ||
        mp_pack_strn(pk, timer->user->name->data, timer->user->name->n) ||

        mp_pack_str(pk, "definition") ||
        mp_pack_strn(pk, def->data, def->n)))
        return -1;

    return 0;
}

ti_val_t * ti_timer_as_mpval(ti_timer_t * timer, _Bool with_full_access)
{
    ti_raw_t * raw;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_raw_t), sizeof(ti_raw_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (timer__info_to_pk(timer, &pk, with_full_access && timer->user))
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    raw = (ti_raw_t *) buffer.data;
    ti_raw_init(raw, TI_VAL_MP, buffer.size);

    return (ti_val_t *) raw;
}
