#include <ti/fn/fn.h>

#define DEL_EXPIRED_DOC_ TI_SEE_DOC("#del_expired")

static int do__f_del_expired(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (query->syntax.flags & TI_SYNTAX_FLAG_THINGSDB);
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    uint64_t after_ts;
    ti_task_t * task;

    /* check for privileges */
    if (ti_access_check_err(
            ti()->access_thingsdb,
            query->user, TI_AUTH_GRANT, e))
        return e->nr;

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `del_expired` takes 0 arguments but %d %s given"
                DEL_EXPIRED_DOC_, nargs, nargs == 1 ? "was" : "were");
        return e->nr;
    }

    after_ts = util_now_tsec();

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_del_expired(task, after_ts))
        ex_set_mem(e);  /* task cleanup is not required */
    else
        (void) ti_users_del_expired(after_ts);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
