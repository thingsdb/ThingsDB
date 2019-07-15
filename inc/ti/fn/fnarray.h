#include <ti/fn/fn.h>

#define ARRAY_DOC_ TI_SEE_DOC("#array")

static int do__f_array(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    int nargs = langdef_nd_n_function_params(nd);

    if (nargs > 1)
    {
        ex_set(e, EX_BAD_DATA,
                "function `array` takes at most 1 argument but %d "
                "were given"ARRAY_DOC_, nargs);
        return e->nr;
    }

    if (nargs == 1)
    {
        return (
            ti_do_scope(query, nd->children->node, e) ||
            ti_val_convert_to_array(&query->rval, e)
        );
    }

    assert (query->rval == NULL);
    query->rval = (ti_val_t *) ti_varr_create(0);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
