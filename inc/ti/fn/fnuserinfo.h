#include <ti/fn/fn.h>

static int do__f_user_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_user_t * user;
    ti_raw_t * ruser;

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
                ti.access_thingsdb,
                query->user, TI_AUTH_GRANT, e))
            return e->nr;

        if (ti_do_statement(query, nd->children, e) ||
            fn_arg_str("user_info", DOC_USER_INFO, 1, query->rval, e))
            return e->nr;

        ruser = (ti_raw_t *) query->rval;
        user = ti_users_get_by_namestrn((const char *) ruser->data, ruser->n);
        if (!user)
            return ti_raw_err_not_found(ruser, "user", e);

        ti_val_unsafe_drop(query->rval);
    }

    query->rval = ti_user_as_mpval(user);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
