#include <ti/fn/fn.h>

static int do__f_grant(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_user_t * user;
    ti_task_t * task;
    ti_raw_t * ruser;
    uint64_t mask, scope_id;
    vec_t ** access_;

    if (fn_not_thingsdb_scope("grant", query, e) ||
        fn_nargs("grant", DOC_GRANT, 3, nargs, e) ||
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

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `grant` expects argument 2 to be of "
            "type `"TI_VAL_STR_S"` but got type `%s` instead"DOC_GRANT,
            ti_val_str(query->rval));
        return e->nr;
    }

    ruser = (ti_raw_t *) query->rval;
    user = ti_users_get_by_namestrn((const char *) ruser->data, ruser->n);
    if (!user)
        return ti_raw_err_not_found(ruser, "user", e);

    ti_val_drop(query->rval);
    query->rval = NULL;

    /* read mask */
    if (ti_do_statement(query, nd->children->next->next->next->next->node, e))
        return e->nr;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `grant` expects argument 3 to be of "
            "type `"TI_VAL_INT_S"` but got type `%s` instead"DOC_GRANT,
            ti_val_str(query->rval));
        return e->nr;
    }

    mask = (uint64_t) VINT(query->rval);

    /* make sure READ when MODIFY and MODIFY when GRANT */
    if (mask & TI_AUTH_GRANT)
        mask |= TI_AUTH_READ|TI_AUTH_MODIFY;
    else if (mask & TI_AUTH_MODIFY)
        mask |= TI_AUTH_READ;

    if (ti_access_grant(access_, user, mask))
    {
        ex_set_mem(e);
        return e->nr;
    }

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_grant(task, scope_id, user, mask))
        ex_set_mem(e);  /* task cleanup is not required */

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
