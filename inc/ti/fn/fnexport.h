#include <ti/fn/fn.h>


static int do__f_export(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_not_collection_scope("export", query, e) ||
        fn_nargs("export", "; Warning: `export(..)` is experimental", 0, nargs, e))
        return e->nr;

    log_warning("function `export(..)` is experimental");

    query->rval = (ti_val_t *) ti_export_collection(query->collection);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
