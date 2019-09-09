#include <ti/fn/fn.h>

#define BOOL_DOC_ TI_SEE_DOC("#bool")

static int do__f_bool(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    _Bool tobool;

    if (fn_chained("bool", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        if (nargs == 0)
        {
            assert (query->rval == NULL);
            query->rval = (ti_val_t *) ti_vbool_get(false);
            return e->nr;
        }
        ex_set(e, EX_BAD_DATA,
                "function `bool` takes at most 1 argument but %d were given"
                BOOL_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    tobool = ti_val_as_bool(query->rval);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(tobool);

    return e->nr;
}
