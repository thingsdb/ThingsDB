#include <ti/rfn/fn.h>

static int rq__f_del_user(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    ti_user_t * user;
    ti_task_t * task;
    ti_raw_t * ruser;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `del_user` takes 1 argument but %d were given",
                n);
        return e->nr;
    }

    if (rq__scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `del_user` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead",
            ti_val_str(query->rval));
        return e->nr;
    }

    ruser = (ti_raw_t *) query->rval;
    user = ti_users_get_by_namestrn((const char *) ruser->data, ruser->n);
    if (!user)
    {
        ex_set(e, EX_INDEX_ERROR, "user `%.*s` not found",
                (int) ruser->n,
                (char *) ruser->data);
        return e->nr;
    }

    if (query->user == user)
    {
        ex_set(e, EX_BAD_DATA,
                "it is not possible to delete your own user account");
        return e->nr;
    }

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_del_user(task, user))
        ex_set_alloc(e);  /* task cleanup is not required */

    /* this will remove the user so it cannot be used after here */
    ti_users_del_user(user);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
