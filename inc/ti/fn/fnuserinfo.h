#include <ti/fn/fn.h>

static int do__f_user_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_user_t * user;
    ti_raw_t * uname;

    if (fn_not_thingsdb_scope("user_info", query, e) ||
        fn_nargs_max("user_info", DOC_USER_INFO, 1, nargs, e))
        return e->nr;

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

        if (ti_do_statement(query, nd->children->node, e))
            return e->nr;

        if (!ti_val_is_raw(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR,
                "function `user_info` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead"
                DOC_USER_INFO,
                ti_val_str(query->rval));
            return e->nr;
        }

        uname = (ti_raw_t *) query->rval;
        user = ti_users_get_by_namestrn((const char *) uname->data, uname->n);
        if (!user)
            return ti_raw_err_not_found(uname, "user", e);

        ti_val_drop(query->rval);
    }

    query->rval = ti_user_info_as_qpval(user);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
