#include <ti/fn/fn.h>

static int do__f_set_owner(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_task_t * task;
    ti_vtask_t * vtask;
    ti_raw_t * ruser;
    ti_user_t * user;

    if (!ti_val_is_task(query->rval))
        return fn_call_try("set_owner", query, nd, e);

    vtask = (ti_vtask_t *) query->rval;
    if (ti_vtask_lock(vtask, e))
        return e->nr;

    query->rval = NULL;

    if (fn_not_thingsdb_or_collection_scope("set_owner", query, e) ||
        fn_nargs("set_owner", DOC_TASK_SET_OWNER, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("set_owner", DOC_TASK_SET_OWNER, 1, query->rval, e))
        goto fail0;

    ruser = (ti_raw_t *) query->rval;
    user = ti_users_get_by_namestrn((const char *) ruser->data, ruser->n);
    if (!user)
    {
        (void) ti_raw_err_not_found(ruser, "user", e);
        goto fail0;
    }

    if (ti_access_check_err(
            query->collection ? query->collection->access : ti.access_thingsdb,
            user,
            TI_AUTH_CHANGE,
            e))
        goto fail0;

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    if (user != vtask->user)
    {
        ti_user_drop(vtask->user);
        vtask->user = user;
        ti_incref(user);

        task = ti_task_get_task(
                query->change,
                query->collection ? query->collection->root : ti.thing0);

        if (!task || ti_task_add_vtask_set_args(task, vtask))
            ex_set_mem(e);  /* task cleanup is not required */
    }
fail0:
    ti_vtask_unlock(vtask);
    ti_vtask_unsafe_drop(vtask);
    return e->nr;
}
