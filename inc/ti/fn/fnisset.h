#include <ti/fn/fn.h>

#define ISSET_DOC_ TI_SEE_DOC("#isset")

static int do__f_isset(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    _Bool is_set;

    if (fn_chained("isset", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_NUM_ARGUMENTS,
                "function `isset` takes 1 argument but %d were given"
                ISSET_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_set = ti_val_is_set(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_set);

    return e->nr;
}
