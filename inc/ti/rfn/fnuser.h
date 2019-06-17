#include <ti/rfn/fn.h>

static int rq__f_user(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    ti_user_t * user;
    ti_raw_t * uname;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `user` takes 1 argument but %d were given",
                n);
        return e->nr;
    }

    if (rq__scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `user` expects argument 1 to be of "
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

    query->rval = ti_user_as_qpval(user);
    if (!query->rval)
        ex_set_alloc(e);

    return e->nr;
}
