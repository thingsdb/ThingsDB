#include <ti/fn/fn.h>

static int do__f_revoke(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    cleri_children_t * child = nd->children;
    ti_user_t * user;
    ti_raw_t * uname;
    ti_task_t * task;
    uint64_t mask, scope_id;
    vec_t ** access_;

    if (fn_not_thingsdb_scope("revoke", query, e) ||
        fn_nargs("revoke", DOC_REVOKE, 3, nargs, e) ||
        ti_do_statement(query, child->node, e))
        return e->nr;

    access_ = ti_val_get_access(query->rval, e, &scope_id);
    if (e->nr || ti_access_check_err(*access_, query->user, TI_AUTH_GRANT, e))
        return e->nr;

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    /* read user */
    if (ti_do_statement(query, (child = child->next->next)->node, e) ||
        fn_arg_str_slow("revoke", DOC_REVOKE, 2, query->rval, e))
        return e->nr;

    uname = (ti_raw_t *) query->rval;
    user = ti_users_get_by_namestrn((const char *) uname->data, uname->n);
    if (!user)
        return ti_raw_err_not_found(uname, "user", e);

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    /* read mask */
    if (ti_do_statement(query, (child = child->next->next)->node, e) ||
        fn_arg_int("revoke", DOC_REVOKE, 3, query->rval, e))
        return e->nr;

    mask = (uint64_t) VINT(query->rval);

    /* make sure EVENT when GRANT */
    if (mask & TI_AUTH_EVENT)
        mask |= TI_AUTH_GRANT;

    if (query->user == user && (mask & TI_AUTH_GRANT))
    {
        ex_set(e, EX_OPERATION,
                "it is not possible to revoke your own `GRANT` privileges"
                DOC_REVOKE);
        return e->nr;
    }

    ti_access_revoke(*access_, user, mask);

    task = ti_task_get_task(query->ev, ti.thing0);
    if (!task || ti_task_add_revoke(task, scope_id, user, mask))
        ex_set_mem(e);  /* task cleanup is not required */

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
