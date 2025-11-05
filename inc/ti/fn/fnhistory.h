#include <ti/fn/fn.h>

static int do__f_history(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_commits_options_t options = {0};
    ti_commits_history_t history = {0};
    vec_t * filtered;
    _Bool detail;
    uint32_t i = 0;

    if (fn_not_thingsdb_scope("history", query, e) ||
        fn_nargs("history", DOC_HISTORY, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_thing("history", DOC_HISTORY, 1, query->rval, e) ||
        ti_commits_options(&options, (ti_thing_t *) query->rval, true, e) ||
        ti_commits_history(&history, &options, e))
        return e->nr;

    if (ti_access_check_err(*history.access, query->user, TI_COMMITS_MASK, e))
        return e->nr;

    filtered = ti_commits_find(*history.commits, &options);
    if (!filtered)
        goto fail;

    detail = options.detail && options.detail->bool_;

    for (vec_each_addr(filtered, void, commit), i++)
    {
        ti_val_t * v = ti_commit_as_mpval(*commit, detail);
        if (!v)
            goto fail;
        *commit = v;
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_varr_from_vec(filtered);
    if (!query->rval)
        goto fail;

    return e->nr;
fail:
    filtered->n = i;
    vec_destroy(filtered, (vec_destroy_cb) ti_val_unassign_drop);
    ex_set_mem(e);
    return e->nr;
}
