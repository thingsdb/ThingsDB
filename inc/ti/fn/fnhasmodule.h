#include <ti/fn/fn.h>

static int do__f_has_module(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    _Bool has_module;

    if (fn_nargs("has_module", DOC_HAS_MODULE, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("has_module", DOC_HAS_MODULE, 1, query->rval, e))
        return e->nr;

    has_module = !!ti_modules_by_raw((ti_raw_t *) query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(has_module);

    return e->nr;
}
