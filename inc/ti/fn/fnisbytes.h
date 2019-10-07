#include <ti/fn/fn.h>

static int do__f_isbytes(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_bytes;
    ti_raw_t * raw;

    if (fn_chained("isbytes", query, e) ||
        fn_nargs("isbytes", DOC_ISBYTES, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    is_bytes = ti_val_is_bytes(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_bytes);

    return e->nr;
}

