#include <ti/fn/fn.h>

static int do__f_backups_ok(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    /* feature request, issue #341 */
    const int nargs = fn_get_nargs(nd);

    if (fn_not_node_scope("backups_ok", query, e) ||
        fn_nargs("backups_ok", DOC_BACKUPS_OK, 0, nargs, e))
        return e->nr;

    query->rval = (ti_val_t *) ti_vbool_get(ti_backups_ok());
    return e->nr;
}
