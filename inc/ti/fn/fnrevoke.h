#include <ti/fn/fn.h>

#define REVOKE_DOC_ TI_SEE_DOC("#revoke")

static int do__f_revoke(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    int n;
    ti_user_t * user;
    ti_raw_t * uname;
    ti_task_t * task;
    uint64_t mask, target_id;
    vec_t ** access_;

    if (fn_not_thingsdb_scope("replace_node", query, e))
        return e->nr;

    n = langdef_nd_n_function_params(nd);
    if (n != 3)
    {
        ex_set(e, EX_BAD_DATA,
                "function `revoke` takes 3 arguments but %d %s given"
                REVOKE_DOC_,
            n, n == 1 ? "was" : "were");
        return e->nr;
    }

    /* target */
    if (ti_do_scope(query, nd->children->node, e))
        return e->nr;

    access_ = ti_val_get_access(query->rval, e, &target_id);
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
    if (ti_do_scope(query, nd->children->next->next->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `revoke` expects argument 2 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"REVOKE_DOC_,
            ti_val_str(query->rval));
        return e->nr;
    }

    uname = (ti_raw_t *) query->rval;
    user = ti_users_get_by_namestrn((const char *) uname->data, uname->n);
    if (!user)
        return ti_raw_err_not_found(uname, "user", e);

    /* mask */
    ti_val_drop(query->rval);
    query->rval = NULL;
    if (ti_do_scope(query, nd->children->next->next->next->next->node, e))
        return e->nr;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `revoke` expects argument 3 to be of "
            "type `"TI_VAL_INT_S"` but got type `%s`"REVOKE_DOC_,
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
        ex_set(e, EX_BAD_DATA,
                "it is not possible to revoke your own `GRANT` privileges"
                REVOKE_DOC_);
        return e->nr;
    }

    ti_access_revoke(*access_, user, mask);

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_revoke(task, target_id, user, mask))
        ex_set_mem(e);  /* task cleanup is not required */

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
