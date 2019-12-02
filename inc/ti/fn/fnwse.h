#include <ti/fn/fn.h>

static int do__f_wse(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool has_wse_flag;

    if (fn_nargs("wse", DOC_WSE, 1, nargs, e))
        return e->nr;

    has_wse_flag = query->qbind.flags & TI_QBIND_FLAG_WSE;
    query->qbind.flags |= TI_QBIND_FLAG_WSE;

    (void) ti_do_statement(query, nd->children->node, e);

    if (!has_wse_flag)
        query->qbind.flags &= ~TI_QBIND_FLAG_WSE;

    return e->nr;
}
