#include <ti/fn/fn.h>

static int do__f_backups_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    if (fn_not_node_scope("backups_info", query, e) ||
        fn_nargs("backups_info", DOC_BACKUPS_INFO, 0, nargs, e))
        return e->nr;

    query->rval = (ti_val_t *) ti_backups_info();
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
