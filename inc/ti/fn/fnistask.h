#include <ti/fn/fn.h>

static int do__f_is_task(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    _Bool is_task;

    if (fn_nargs("is_task", DOC_IS_TASK, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_task = ti_val_is_task(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_task);

    return e->nr;
}
