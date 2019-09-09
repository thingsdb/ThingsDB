#include <ti/fn/fn.h>

#define RENAME_USER_DOC_ TI_SEE_DOC("#rename_user")

static int do__f_rename_user(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    int nargs = langdef_nd_n_function_params(nd);
    ti_task_t * task;
    ti_user_t * user;
    ti_raw_t * rname;

    if (fn_not_thingsdb_scope("rename_user", query, e))
        return e->nr;

    if (nargs != 2)
    {
        ex_set(e, EX_BAD_DATA,
            "function `rename_user` takes 2 arguments but %d %s given"
            RENAME_USER_DOC_, nargs, nargs == 1 ? "was" : "were");
        return e->nr;
    }

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `rename_user` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"RENAME_USER_DOC_,
            ti_val_str(query->rval));
        return e->nr;
    }

    rname = (ti_raw_t *) query->rval;
    user = ti_users_get_by_namestrn((const char *) rname->data, rname->n);
    if (!user)
        return ti_raw_err_not_found(rname, "user", e);

    ti_val_drop(query->rval);
    query->rval = NULL;
    if (ti_do_statement(query, nd->children->next->next->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `rename_user` expects argument 2 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"RENAME_USER_DOC_,
            ti_val_str(query->rval));
        return e->nr;
    }

    rname = (ti_raw_t *) query->rval;
    if (!rname)
    {
        ex_set_mem(e);
        return e->nr;
    }

    if (ti_user_rename(user, rname, e))
        return e->nr;

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_rename_user(task, user))
        ex_set_mem(e);  /* task cleanup is not required */

    return e->nr;
}
