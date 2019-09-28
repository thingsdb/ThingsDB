#include <ti/fn/fn.h>

static int do__f_set_password(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    char * passstr = NULL;
    ti_raw_t * uname;
    ti_user_t * user;
    ti_task_t * task;

    if (fn_not_thingsdb_scope("set_password", query, e) ||
        fn_nargs("set_password", DOC_SET_PASSWORD, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `set_password` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"DOC_SET_PASSWORD,
            ti_val_str(query->rval));
        return e->nr;
    }

    uname = (ti_raw_t *) query->rval;
    user = ti_users_get_by_namestrn((const char *) uname->data, uname->n);
    if (!user)
        return ti_raw_err_not_found(uname, "user", e);

    ti_val_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next->node, e))
        goto done;

    if (ti_val_is_raw(query->rval))
    {
        passstr = ti_raw_to_str((ti_raw_t *) query->rval);
        if (!passstr)
        {
            ex_set_mem(e);
            goto done;
        }

        if (!ti_user_pass_check(passstr, e))
            goto done;
    }
    else if (!ti_val_is_nil(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `set_password` expects argument 2 to be of "
            "type `"TI_VAL_RAW_S"` or `"TI_VAL_NIL_S"` but got "
            "type `%s` instead"DOC_SET_PASSWORD,
            ti_val_str(query->rval));
        goto done;
    }

    if (ti_user_set_pass(user, passstr))
    {
        ex_set_mem(e);
        goto done;
    }

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        goto done;

    if (ti_task_add_set_password(task, user))
        ex_set_mem(e);  /* task cleanup is not required */

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

done:
    free(passstr);
    return e->nr;
}
