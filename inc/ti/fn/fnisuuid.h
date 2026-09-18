#include <ti/fn/fn.h>

static int do__f_is_uuid(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    _Bool is_uuid;

    if (fn_nargs("is_uuid", DOC_IS_UUID, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e))
        return e->nr;

    is_uuid = ti_val_is_uuid(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_uuid);

    return 0;
}
