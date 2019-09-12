#include <ti/fn/fn.h>

#define GRANT_DOC_ TI_SEE_DOC("#grant")

static int do__f_grant(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    int nargs;
    ti_user_t * user;
    ti_task_t * task;
    ti_raw_t * ruser;
    uint64_t mask, scope_id;
    vec_t ** access_;

    if (fn_not_thingsdb_scope("grant", query, e))
        return e->nr;

    nargs = langdef_nd_n_function_params(nd);
    if (nargs != 3)
    {
        ex_set(e, EX_BAD_DATA,
            "function `grant` takes 3 arguments but %d %s given"GRANT_DOC_,
            nargs, nargs == 1 ? "was" : "were");
        return e->nr;
    }

    /* scope */
    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    access_ = ti_val_get_access(query->rval, e, &scope_id);
    if (e->nr)
        return e->nr;

    /* check for privileges */
    if (ti_access_check_err(
            *access_,
            query->user, TI_AUTH_GRANT, e))
        return e->nr;

    /* user */
    ti_val_drop(query->rval);
    query->rval = NULL;
    if (ti_do_statement(query, nd->children->next->next->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `grant` expects argument 2 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"GRANT_DOC_,
            ti_val_str(query->rval));
        return e->nr;
    }

    ruser = (ti_raw_t *) query->rval;
    user = ti_users_get_by_namestrn((const char *) ruser->data, ruser->n);
    if (!user)
        return ti_raw_err_not_found(ruser, "user", e);

    /* mask */
    ti_val_drop(query->rval);
    query->rval = NULL;
    if (ti_do_statement(query, nd->children->next->next->next->next->node, e))
        return e->nr;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `grant` expects argument 3 to be of "
            "type `"TI_VAL_INT_S"` but got type `%s` instead"GRANT_DOC_,
            ti_val_str(query->rval));
        return e->nr;
    }

    mask = (uint64_t) ((ti_vint_t *) query->rval)->int_;

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
