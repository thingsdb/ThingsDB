#include <ti/fn/fn.h>

static int do__f_bool(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool tobool;

    if (fn_chained("bool", query, e) ||
        fn_nargs_max("bool", DOC_BOOL, 1, nargs, e))
        return e->nr;

    if (nargs == 0)
    {
        assert (query->rval == NULL);
        query->rval = (ti_val_t *) ti_vbool_get(false);
        return e->nr;
    }

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    tobool = ti_val_as_bool(query->rval);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(tobool);

    return e->nr;
}
