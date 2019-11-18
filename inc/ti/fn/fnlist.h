#include <ti/fn/fn.h>

static int do__f_list(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_chained("list", query, e) ||
        fn_nargs_max("list", DOC_LIST, 1, nargs, e))
        return e->nr;

    if (nargs == 1)
    {
        return (
            ti_do_statement(query, nd->children->node, e) ||
            ti_val_convert_to_array(&query->rval, e)
        );
    }

    assert (query->rval == NULL);
    query->rval = (ti_val_t *) ti_varr_create(0);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
