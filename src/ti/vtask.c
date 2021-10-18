/*
 * ti/vtask.c
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
#include <ti/vtask.h>
#include <ti/vtask.inline.h>
#include <ti/vtask.t.h>
#include <ti/user.h>
#include <ti/vint.h>
#include <ti/val.inline.h>

static const ti_vtask_t vtask__nil = {
        .ref=1,
        .tp=TI_VAL_TASK,
};

ti_vtask_t * ti_vtask_create(
        uint64_t id,
        uint64_t run_at,
        ti_user_t * user,
        ti_closure_t * closure,
        ti_verror_t * verr,     /* may be NULL */
        vec_t * args)
{
    ti_vtask_t * vtask = malloc(sizeof(ti_vtask_t));
    if (!vtask)
        return NULL;

    vtask->ref = 1;
    vtask->id = id;
    vtask->run_at = run_at;
    vtask->user = user;
    vtask->closure = closure;
    vtask->args = args;
    vtask->verr = verr;

    ti_incref(user);
    ti_incref(closure);
    if (verr)
        ti_incref(verr);

    return vtask;
}

ti_vtask_t * ti_vtask_nil(void)
{
    ti_incref(&vtask__nil);
    return &vtask__nil;
}

void ti_vtask_destroy(ti_vtask_t * vtask)
{
    if (vtask && vtask->id)
    {
        ti_user_drop(vtask->user);
        ti_closure_unsafe_drop(vtask->closure);
        ti_verror_unsafe_drop(vtask->verr);
        vec_destroy(vtask->args, (vec_destroy_cb) ti_val_unsafe_drop);
    }
    free(vtask);
}

/*
 * If this function is not able to forward the vtask, an exception will
 * be set but `ti_vtask_done(..)` will not be called.
 */
static int vtask__fwd(ti_vtask_t * vtask, ti_collection_t * collection)
{

    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_pkg_t * pkg;
    ti_node_t * node = ti_nodes_random_ready_node();
    uint64_t scope_id = collection ? collection->root->id : TI_SCOPE_THINGSDB;

    if (!node)
    {
        log_error(
            "cannot find a ready node for processing task %"PRIu64,
            vtask->id);
        return -1;
    }

    if (mp_sbuffer_alloc_init(&buffer, 32, sizeof(ti_pkg_t)))
    {
        log_error(EX_MEMORY_S);
        return -1;
    }
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);
    msgpack_pack_uint64(&pk, scope_id);
    msgpack_pack_uint64(&pk, vtask->id);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_NODE_FWD_TASK, buffer.size);

    if (ti_stream_write_pkg(node->stream, pkg))
    {
        free(pkg);
        log_error("failed to forward task package");
        return -1;
    }

    return 0;
}

int ti_vtask_run(ti_vtask_t * vtask, ti_collection_t * collection)
{
    ex_t e = {0};
    ti_node_t * this_node = ti.node;
    ti_query_t * query = NULL;

    if (this_node->status != TI_NODE_STAT_READY)
        return vtask__fwd(vtask, collection);

    query = ti_query_create(0);
    if (!query)
    {
        log_error(EX_MEMORY_S);
        return -1;
    }

    query->user = vtask->user;
    query->with_tp = TI_QUERY_WITH_TIMER;
    query->pkg_id = 0;
    query->with.vtask = vtask;          /* move the vtask reference */
    query->collection = collection;     /* may be NULL */
    query->qbind.flags |= collection
        ? TI_QBIND_FLAG_COLLECTION
        : TI_QBIND_FLAG_THINGSDB;

    ti_incref(query->user);

    if (query->collection)
        ti_incref(query->collection);

    if (vtask->closure->flags & TI_CLOSURE_FLAG_WSE)
        query->qbind.flags |= TI_QBIND_FLAG_WSE;

    if (ti_changes_create_new_change(query, &e))
    {
        log_error(e.msg);
        ti_query_destroy(query);
        return -1;
    }
    return 0;
}

static void vtask__clear(ti_vtask_t * vtask)
{
    ti_user_drop(vtask->user);
    ti_closure_unsafe_drop(vtask->closure);
    ti_verror_unsafe_drop(vtask->verr);
    vec_destroy(vtask->args, (vec_destroy_cb) ti_val_unsafe_drop);
    vtask->id = 0;
    vtask->run_at = 0;
    vtask->user = NULL;
    vtask->closure = NULL;
    vtask->verr = NULL;
    vtask->args = NULL;
}

void ti_vtask_del(uint64_t vtask_id, ti_collection_t * collection)
{
    uint32_t idx = 0;
    vec_t * vtasks = collection ? collection->vtasks : ti.tasks->vtasks;

    for (vec_each(vtasks, ti_vtask_t, vt), ++idx)
    {
        if (vt->id == vtask_id)
        {
            ti_vtask_t * vtask = vec_swap_remove(vtasks, idx);
            if (--vtask->ref)
                vtask__clear(vtask);
            else
                ti_vtask_destroy(vtask);
            break;
        }
    }
}

int ti_vtask_check_thingsdb_args(vec_t * args, ex_t * e)
{
    for (vec_each(args, ti_val_t, v))
    {
        if (!ti_val(v)->allowed_as_vtask_arg)
        {
            ex_set(e, EX_TYPE_ERROR,
                "type `%s` is not allowed as a task argument in "
                "the `@thingsdb` scope"DOC_TASK,
                ti_val_str(v));
            return e->nr;
        }
    }
    return 0;
}

ti_raw_t * ti_vtask_str(ti_vtask_t * vtask)
{
    if (vtask->id)
    {
        const char * status, * run_at;

        run_at = vtask->run_at
                ? ti_datetime_ts_str((const time_t *) &vtask->run_at)
                : "nil";
        status = vtask->verr
                ? vtask->verr->code
                ? "err"
                : "ok"
                : "nil";
        return ti_str_from_fmt(
                "task:<id:%"PRIu64" owner:%.*s run_at:%s status:%s>",
                vtask->id,
                vtask->user->name->n, vtask->user->name->data,
                run_at,
                status);
    }
    return ti_str_from_str("task:nil");
}

int ti_vtask_to_pk(ti_vtask_t * vtask, msgpack_packer * pk, int options)
{
    if (options < 0)
    {
        unsigned char buf[8];
        mp_store_uint64(vtask->id, buf);
        return mp_pack_ext(pk, MPACK_EXT_TASK, buf, sizeof(buf));
    }
    else if (vtask->id)
    {
        const char * status, * run_at;

        run_at = vtask->run_at
                ? ti_datetime_ts_str((const time_t *) &vtask->run_at)
                : "nil";
        status = vtask->verr
                ? vtask->verr->code
                ? "err"
                : "ok"
                : "nil";
        return mp_pack_fmt(pk,
                "task:<id:%"PRIu64" owner:%.*s run_at:%s status:%s>",
                vtask->id,
                vtask->user->name->n, vtask->user->name->data,
                run_at,
                status);
    }

    return mp_pack_str(pk, "task:nil");
}