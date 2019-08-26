#include <ti/fn/fn.h>

#define ISBOOL_DOC_ TI_SEE_DOC("#isbool")

static int do__f_isbool(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    _Bool is_bool;

    if (fn_chained("isbool", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `isbool` takes 1 argument but %d were given"
                ISBOOL_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_scope(query, nd->children->node, e))
        return e->nr;

    is_bool = ti_val_is_bool(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_bool);

    return e->nr;
}
