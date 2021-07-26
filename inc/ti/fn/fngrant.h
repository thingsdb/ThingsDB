#include <ti/fn/fn.h>

static int do__f_grant(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    cleri_children_t * child = nd->children;
    ti_user_t * user;
    ti_task_t * task;
    ti_raw_t * ruser;
    uint64_t mask, scope_id;
    vec_t ** access_;

    if (fn_not_thingsdb_scope("grant", query, e) ||
        fn_nargs("grant", DOC_GRANT, 3, nargs, e) ||
        ti_do_statement(query, child->node, e))
        return e->nr;

    access_ = ti_val_get_access(query->rval, e, &scope_id);
    if (e->nr || ti_access_check_err(*access_, query->user, TI_AUTH_GRANT, e))
        return e->nr;

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    /* read user */
    if (ti_do_statement(query, (child = child->next->next)->node, e) ||
        fn_arg_str_slow("grant", DOC_GRANT, 2, query->rval, e))
        return e->nr;

    ruser = (ti_raw_t *) query->rval;
    user = ti_users_get_by_namestrn((const char *) ruser->data, ruser->n);
    if (!user)
        return ti_raw_err_not_found(ruser, "user", e);

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    /* read mask */
    if (ti_do_statement(query, (child = child->next->next)->node, e) ||
        fn_arg_int("grant", DOC_GRANT, 3, query->rval, e))
        return e->nr;

    mask = (uint64_t) VINT(query->rval);

    /* make sure CHANGE when GRANT */
    if (mask & TI_AUTH_GRANT)
        mask |= TI_AUTH_CHANGE;

    if (ti_access_grant(access_, user, mask))
    {
        ex_set_mem(e);
        return e->nr;
    }

    task = ti_task_get_task(query->change, ti.thing0);
    if (!task || ti_task_add_grant(task, scope_id, user, mask))
        ex_set_mem(e);  /* task cleanup is not required */

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
