#include <ti/fn/fn.h>

static int do__f_str(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_chained("str", query, e) ||
        fn_nargs_max("str", DOC_STR, 1, nargs, e))
        return e->nr;

    if (nargs == 0)
    {
        assert (query->rval == NULL);
        query->rval = ti_val_empty_str();
        return e->nr;
    }

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    return ti_val_convert_to_str(&query->rval, e);
}
