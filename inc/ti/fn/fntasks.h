#include <ti/fn/fn.h>

static int do__f_task(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    vec_t ** tasks = ti_query_tasks(query);

    if (fn_not_thingsdb_or_collection_scope("tasks", query, e) ||
        fn_nargs("tasks", DOC_TASKS, 0, nargs, e))
        return e->nr;

    query->rval = (ti_val_t *) ti_tasks_list(*ti_tasks_list);
    if (!query->rval)
        ex_set_mem(e);
    return e->nr;
}
