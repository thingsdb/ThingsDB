#include <ti/fn/fn.h>

#define INT_DOC_ TI_SEE_DOC("#int")

static int do__f_int(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        if (nargs == 0)
        {
            assert (query->rval == NULL);
            query->rval = (ti_val_t *) ti_vint_create(0);
            return e->nr;
        }
        ex_set(e, EX_BAD_DATA,
                "function `int` takes at most 1 argument but %d were given"
                INT_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_scope(query, nd->children->node, e))
        return e->nr;

    return ti_val_convert_to_int(&query->rval, e);
}
