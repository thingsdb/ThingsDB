#include <ti/fn/fn.h>

static int do__f_revoke(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_user_t * user;
    ti_raw_t * uname;
    ti_task_t * task;
    uint64_t mask, scope_id;
    vec_t ** access_;

    if (fn_not_thingsdb_scope("revoke", query, e) ||
        fn_nargs("revoke", DOC_REVOKE, 3, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    access_ = ti_val_get_access(query->rval, e, &scope_id);
    if (e->nr || ti_access_check_err(*access_, query->user, TI_AUTH_GRANT, e))
        return e->nr;

    ti_val_drop(query->rval);
    query->rval = NULL;

    /* read user */
    if (ti_do_statement(query, nd->children->next->next->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `revoke` expects argument 2 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"DOC_REVOKE,
            ti_val_str(query->rval));
        return e->nr;
    }

    uname = (ti_raw_t *) query->rval;
    user = ti_users_get_by_namestrn((const char *) uname->data, uname->n);
    if (!user)
        return ti_raw_err_not_found(uname, "user", e);

    ti_val_drop(query->rval);
    query->rval = NULL;

    /* read mask */
    if (ti_do_statement(query, nd->children->next->next->next->next->node, e))
        return e->nr;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `revoke` expects argument 3 to be of "
            "type `"TI_VAL_INT_S"` but got type `%s`"DOC_REVOKE,
            ti_val_str(query->rval));
        return e->nr;
    }

    mask = (uint64_t) ((ti_vint_t *) query->rval)->int_;

    /* make sure READ when MODIFY and MODIFY when GRANT */
    if (mask & TI_AUTH_READ)
        mask |= TI_AUTH_MODIFY|TI_AUTH_GRANT;
    else if (mask & TI_AUTH_MODIFY)
        mask |= TI_AUTH_GRANT;

    if (query->user == user && (mask & TI_AUTH_GRANT))
    {
        ex_set(e, EX_OPERATION_ERROR,
                "it is not possible to revoke your own `GRANT` privileges"
                DOC_REVOKE);
        return e->nr;
    }

    ti_access_revoke(*access_, user, mask);

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_revoke(task, scope_id, user, mask))
        ex_set_mem(e);  /* task cleanup is not required */

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
