#include <ti/fn/fn.h>

static int do__f_wse(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    uint8_t wse_flag_state;

    if (fn_nargs("wse", DOC_WSE, 1, nargs, e))
        return e->nr;

    wse_flag_state = ~query->qbind.flags & TI_QBIND_FLAG_WSE;
    query->qbind.flags |= TI_QBIND_FLAG_WSE;

    (void) ti_do_statement(query, nd->children->node, e);

    query->qbind.flags &= ~wse_flag_state;
    return e->nr;
}
