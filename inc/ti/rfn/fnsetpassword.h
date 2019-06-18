#include <ti/rfn/fn.h>

static int rq__f_set_password(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    int n;
    char * passstr = NULL;
    ti_raw_t * uname;
    ti_user_t * user;
    ti_task_t * task;
    n = langdef_nd_n_function_params(nd);

    if (n != 2)
    {
        ex_set(e, EX_BAD_DATA,
            "function `set_password` requires 2 arguments but %d %s given",
            n, n == 1 ? "was" : "were");
        return e->nr;
    }

    if (rq__scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `set_password` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead",
            ti_val_str(query->rval));
        return e->nr;
    }

    uname = (ti_raw_t *) query->rval;
    user = ti_users_get_by_namestrn((const char *) uname->data, uname->n);
    if (!user)
    {
        ex_set(e, EX_INDEX_ERROR, "user `%.*s` not found",
                (int) uname->n,
                (char *) uname->data);
        return e->nr;
    }

    ti_val_drop(query->rval);
    query->rval = NULL;

    if (rq__scope(query, nd->children->next->next->node, e))
        goto done;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `set_password` expects argument 2 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead",
            ti_val_str(query->rval));
        goto done;
    }

    passstr = ti_raw_to_str((ti_raw_t *) query->rval);
    if (!passstr)
    {
        ex_set_alloc(e);
        goto done;
    }

    if (!ti_user_pass_check(passstr, e))
        goto done;

    if (ti_user_set_pass(user, passstr))
    {
        ex_set_alloc(e);
        goto done;
    }

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        goto done;

    if (ti_task_add_set_password(task, user))
        ex_set_alloc(e);  /* task cleanup is not required */

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

done:
    free(passstr);
    return e->nr;
}