#include <ti/fn/fn.h>

static int do__f_del_history(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_commits_options_t options = {0};
    ti_commits_history_t history = {0};
    vec_t * deleted;
    ti_task_t * task;

    if (fn_not_thingsdb_scope("del_history", query, e) ||
        fn_nargs("del_history", DOC_DEL_HISTORY, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_thing("del_history", DOC_DEL_HISTORY, 1, query->rval, e) ||
        ti_commits_options(&options, (ti_thing_t *) query->rval, false, e) ||
        ti_commits_history(&history, &options, e))
        return e->nr;

    if (ti_access_check_err(*history.access, query->user, TI_COMMITS_MASK, e))
        return e->nr;

    deleted = ti_commits_del(history.commits, &options);
    if (!deleted)
        goto fail;

    if (deleted->n)
    {
        task = ti_task_get_task(
            query->change,
            query->collection ? query->collection->root : ti.thing0);
        if (!task || ti_task_add_del_history(task, history.scope_id, deleted))
            goto undo;
    }
    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vint_create((int64_t) deleted->n);
    if (!query->rval)
        ex_set_mem(e);

    vec_destroy(deleted, (vec_destroy_cb) ti_commit_destroy);
    return e->nr;
undo:
    for (vec_each(deleted, ti_commit_t, commit))
        VEC_push(*history.commits, commit);
    vec_destroy(deleted, NULL);
fail:
    ex_set_mem(e);
    return e->nr;
}
