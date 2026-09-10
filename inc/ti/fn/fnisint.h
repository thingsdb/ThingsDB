#include <ti/fn/fn.h>

static int do__f_is_int(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    _Bool is_int;

    if (fn_nargs("is_int", DOC_IS_INT, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e))
        return e->nr;

    is_int = ti_val_is_int(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_int);

    return 0;
}
