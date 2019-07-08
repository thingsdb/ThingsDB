#include <ti/rfn/fn.h>

#define USER_DOC_ TI_SEE_DOC("#user")

static int rq__f_user(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    ti_user_t * user;
    ti_raw_t * uname;
    int nargs = langdef_nd_n_function_params(nd);

    if (nargs > 1)
    {
        ex_set(e, EX_BAD_DATA,
            "function `user` takes at most 1 argument but %d were given"
            USER_DOC_, nargs);
        return e->nr;
    }

    if (!nargs)
    {
        /*
         * Without arguments, return user info about the connected user
         * which is allowed without `GRANT` privileges.
         */
        user = query->user;
    }
    else
    {
        /* check for `GRANT` privileges since user information is exposed */
        if (ti_access_check_err(
                ti()->access_thingsdb,
                query->user, TI_AUTH_GRANT, e))
            return e->nr;

        if (ti_rq_scope(query, nd->children->node, e))
            return e->nr;

        if (!ti_val_is_raw(query->rval))
        {
            ex_set(e, EX_BAD_DATA,
                "function `user` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead"USER_DOC_,
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
    }

    query->rval = ti_user_as_qpval(user);
    if (!query->rval)
        ex_set_alloc(e);

    return e->nr;
}
