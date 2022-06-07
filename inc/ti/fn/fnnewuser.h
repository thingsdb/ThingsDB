#include <ti/fn/fn.h>

static int do__f_new_user(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_user_t * nuser;
    ti_raw_t * rname;
    ti_task_t * task;

    if (fn_not_thingsdb_scope("new_user", query, e) ||
        ti_access_check_err(
                    ti.access_thingsdb,
                    query->user, TI_AUTH_GRANT, e) ||
        fn_nargs("new_user", DOC_NEW_USER, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_str("new_user", DOC_NEW_USER, 1, query->rval, e))
        return e->nr;

    rname = (ti_raw_t *) query->rval;
    nuser = ti_users_new_user(
            (const char *) rname->data,
            rname->n,
            NULL,
            util_now_usec(),
            e);
    if (!nuser)
        return e->nr;

    task = ti_task_get_task(query->change, ti.thing0);
    if (!task || ti_task_add_new_user(task, nuser))
        ex_set_mem(e);  /* task cleanup is not required */

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vint_create((int64_t) nuser->id);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
