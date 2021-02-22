#include <ti/fn/fn.h>

static int do__f_has_user(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    _Bool has_user;
    ti_raw_t * uname;

    if (fn_not_thingsdb_scope("has_user", query, e) ||
        ti_access_check_err(
                        ti.access_thingsdb,
                        query->user, TI_AUTH_GRANT, e) ||
        fn_nargs("has_user", DOC_HAS_USER, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("has_user", DOC_HAS_USER, 1, query->rval, e))
        return e->nr;

    uname = (ti_raw_t *) query->rval;
    has_user = !!ti_users_get_by_namestrn(
            (const char *) uname->data,
            uname->n);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(has_user);

    return e->nr;
}
