#include <ti/fn/fn.h>

#define NEW_USER_DOC_ TI_SEE_DOC("#new_user")

static int do__f_new_user(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    ti_user_t * nuser;
    ti_raw_t * rname;
    ti_task_t * task;

    if (fn_not_thingsdb_scope("new_user", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_NUM_ARGUMENTS,
                "function `new_user` takes 1 argument but %d were given"
                NEW_USER_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `new_user` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"NEW_USER_DOC_,
            ti_val_str(query->rval));
        return e->nr;
    }

    rname = (ti_raw_t *) query->rval;
    nuser = ti_users_new_user((const char *) rname->data, rname->n, NULL, e);
    if (!nuser)
        return e->nr;

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_new_user(task, nuser))
        ex_set_mem(e);  /* task cleanup is not required */

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vint_create((int64_t) nuser->id);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
