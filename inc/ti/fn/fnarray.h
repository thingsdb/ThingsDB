#include <ti/fn/fn.h>

#define ARRAY_DOC_ TI_SEE_DOC("#array")

static int do__f_array(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    int nargs = langdef_nd_n_function_params(nd);

    if (fn_chained("array", query, e))
        return e->nr;

    if (nargs > 1)
    {
        ex_set(e, EX_NUM_ARGUMENTS,
                "function `array` takes at most 1 argument but %d "
                "were given"ARRAY_DOC_, nargs);
        return e->nr;
    }

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
