#include <ti/fn/fn.h>

static int do__f_is_raw(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    _Bool is_raw;

    if (fn_nargs("is_raw", DOC_IS_RAW, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e))
        return e->nr;

    is_raw = ti_val_is_raw(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_raw);

    return e->nr;
}
