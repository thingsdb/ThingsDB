#include <ti/fn/fn.h>

static int do__f_del_expired(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    uint64_t after_ts;
    ti_task_t * task;

    if (fn_not_thingsdb_scope("del_expired", query, e) ||
        ti_access_check_err(
            ti.access_thingsdb,
            query->user, TI_AUTH_GRANT, e) ||
        fn_nargs("del_expired", DOC_DEL_EXPIRED, 0, nargs, e))
        return e->nr;

    after_ts = util_now_usec();

    task = ti_task_get_task(query->ev, ti.thing0);
    if (!task || ti_task_add_del_expired(task, after_ts))
        ex_set_mem(e);  /* task cleanup is not required */
    else
        ti_users_del_expired(after_ts);

    query->rval = (ti_val_t *) ti_nil_get();
    return e->nr;
}
