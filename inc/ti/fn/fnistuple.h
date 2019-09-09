#include <ti/fn/fn.h>

#define ISTUPLE_DOC_ TI_SEE_DOC("#istuple")

static int do__f_istuple(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    _Bool is_tuple;

    if (fn_chained("istuple", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `istuple` takes 1 argument but %d were given"
                ISTUPLE_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_tuple = ti_val_is_tuple(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_tuple);

    return e->nr;
}
