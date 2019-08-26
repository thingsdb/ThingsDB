#include <ti/fn/fn.h>

#define ISINT_DOC_ TI_SEE_DOC("#isint")

static int do__f_isint(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    _Bool is_isint;

    if (fn_chained("isint", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `isint` takes 1 argument but %d were given"
                ISINT_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_scope(query, nd->children->node, e))
        return e->nr;

    is_isint = ti_val_is_int(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_isint);

    return e->nr;
}
