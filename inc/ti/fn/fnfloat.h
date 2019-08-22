#include <ti/fn/fn.h>

#define FLOAT_DOC_ TI_SEE_DOC("#float")

static int do__f_float(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        if (nargs == 0)
        {
            assert (query->rval == NULL);
            query->rval = (ti_val_t *) ti_vfloat_create(0.0);
            return e->nr;
        }
        ex_set(e, EX_BAD_DATA,
                "function `float` takes at most 1 argument but %d were given"
                FLOAT_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_scope(query, nd->children->node, e))
        return e->nr;

    return ti_val_convert_to_float(&query->rval, e);
}
