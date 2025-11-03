#include <ti/fn/fn.h>

static int do__f_commit(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    vec_t ** commits = ti_query_commits(query);
    ti_commit_t * commit;

    if (fn_not_thingsdb_or_collection_scope("commit", query, e) ||
        fn_nargs("commit", DOC_COMMIT, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_str("commit", DOC_COMMIT, 1, query->rval, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (!(*commits))
    {
        ex_set(e, EX_OPERATION,
                "commit history is not enabled for the `%s` scope"
                DOC_SET_HISTORY,
                ti_query_scope_name(query));
        goto fail0;
    }

    switch ((ti_query_with_enum) query->with_tp)
    {
        case TI_QUERY_WITH_PARSERES: break;
        case TI_QUERY_WITH_PROCEDURE:
            ex_set(e, EX_OPERATION,
                "function `commit` is not allowed in a procedure");
            goto fail0;
        case TI_QUERY_WITH_FUTURE:
            ex_set(e, EX_OPERATION,
                "function `commit` is not allowed in a future");
            goto fail0;
        case TI_QUERY_WITH_TASK:
        case TI_QUERY_WITH_TASK_FINISH:
            ex_set(e, EX_OPERATION,
                "function `commit` is not allowed in a task");
            goto fail0;
    }

    if (!query->change)
    {
        ex_set(e, EX_OPERATION, "commit without a change"DOC_SET_HISTORY);
        goto fail0;
    }

    if (!query->user)
    {
        ex_set(e, EX_OPERATION, "commit without a user"DOC_SET_HISTORY);
        goto fail0;
    }

    if (!raw->n)
    {
        ex_set(e, EX_VALUE_ERROR, "commit message must not be empty");
        goto fail0;
    }

    if (query->commit)
    {
        ex_set(e, EX_OPERATION, "commit message already set");
        goto fail0;
    }

    commit = ti_commit_make(
        query->change->id,
        query->with.parseres->str,
        query->user->name,
        raw);

    if (!commit || vec_push(commits, commit))
    {
        ti_commit_destroy(commit);
        ex_set_mem(e);
        goto fail0;
    }

    query->rval = ti_nil_get();
fail0:
    ti_val_unsafe_drop((ti_val_t *) raw);
    return e->nr;
}
