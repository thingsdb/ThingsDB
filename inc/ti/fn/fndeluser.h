#include <ti/fn/fn.h>

static int do__f_del_user(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_user_t * user;
    ti_task_t * task;
    ti_raw_t * ruser;

    if (fn_not_thingsdb_scope("del_user", query, e) ||
        fn_nargs("del_user", DOC_DEL_USER, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `del_user` expects argument 1 to be of "
            "type `"TI_VAL_STR_S"` but got type `%s` instead"DOC_DEL_USER,
            ti_val_str(query->rval));
        return e->nr;
    }

    ruser = (ti_raw_t *) query->rval;
    user = ti_users_get_by_namestrn((const char *) ruser->data, ruser->n);
    if (!user)
        return ti_raw_err_not_found(ruser, "user", e);

    if (query->user == user)
    {
        ex_set(e, EX_OPERATION_ERROR,
                "it is not possible to delete your own user account"
                DOC_DEL_USER);
        return e->nr;
    }

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_del_user(task, user))
        ex_set_mem(e);  /* task cleanup is not required */

    /* this will remove the user so it cannot be used after here */
    ti_users_del_user(user);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
