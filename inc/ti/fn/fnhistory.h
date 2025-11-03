#include <ti/fn/fn.h>





static int do__f_history(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    vec_t ** commits;
    vec_t * arr;

    if (fn_not_thingsdb_scope("history", query, e) ||
        ti_access_check_err(
                    ti.access_thingsdb,
                    query->user, TI_AUTH_GRANT, e) ||
        fn_nargs("history", DOC_HISTORY, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_thing("history", DOC_HISTORY, 1, query->rval, e))
        return e->nr;

    options.e = e;

    if (ti_thing_walk(
        (ti_thing_t *) query->rval,
        (ti_thing_item_cb) history__option,
        &options))
        return e->nr;

    arr





    return e->nr;
}
