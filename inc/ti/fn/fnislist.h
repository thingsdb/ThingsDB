#include <ti/fn/fn.h>

#define ISLIST_DOC_ TI_SEE_DOC("#islist")

static int do__f_islist(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    _Bool is_list;

    if (fn_chained("isint", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `islist` takes 1 argument but %d were given"
                ISLIST_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_scope(query, nd->children->node, e))
        return e->nr;

    is_list = ti_val_is_list(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_list);

    return e->nr;
}
