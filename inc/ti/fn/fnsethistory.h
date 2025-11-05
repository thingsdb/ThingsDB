#include <ti/fn/fn.h>

static int do__f_set_history(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    _Bool state;
    ti_task_t * task;
    uint64_t  scope_id;
    vec_t ** access_, ** commits;

    if (fn_not_thingsdb_scope("set_history", query, e) ||
        fn_nargs("set_history", DOC_SET_HISTORY, 2, nargs, e) ||
        ti_do_statement(query, nd->children, e))
        return e->nr;

    access_ = ti_val_get_access(query->rval, e, &scope_id);
    if (e->nr || ti_access_check_err(*access_, query->user, TI_COMMITS_MASK, e))
        return e->nr;

    commits = ti_commits_from_scope((ti_raw_t *) query->rval, e);
    if (e->nr)
        return e->nr;

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    /* read state */
    if (ti_do_statement(query, nd->children->next->next, e) ||
        fn_arg_bool("set_history", DOC_SET_HISTORY, 2, query->rval, e))
        return e->nr;

    state = VBOOL(query->rval);
    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    if (state != !!(*commits))
    {
        if (ti_commits_set_history(commits, state))
        {
            ex_set_mem(e);
            return e->nr;
        }

        task = ti_task_get_task(
            query->change,
            query->collection ? query->collection->root : ti.thing0);

        if (!task || ti_task_add_set_history(task, scope_id, state))
            ex_set_mem(e);  /* task cleanup is not required */
    }

    return e->nr;
}
