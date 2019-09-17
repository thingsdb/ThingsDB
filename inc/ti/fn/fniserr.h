#include <ti/fn/fn.h>

#define ISERROR_DOC_ TI_SEE_DOC("#iserr")

static int do__f_iserr(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    _Bool is_error;

    if (fn_chained("iserr", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_NUM_ARGUMENTS,
                "function `iserr` takes 1 argument but %d were given"
                ISERROR_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_error = ti_val_is_error(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_error);

    return e->nr;
}
