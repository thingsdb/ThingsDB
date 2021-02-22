#include <ti/fn/fn.h>

static int do__f_is_regex(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    _Bool is_regex;

    if (fn_nargs("is_regex", DOC_IS_REGEX, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_regex = ti_val_is_regex(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_regex);

    return e->nr;
}
