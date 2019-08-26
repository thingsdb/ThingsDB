#include <ti/fn/fn.h>

#define ISRAW_DOC_ TI_SEE_DOC("#israw")

static int do__f_israw(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    _Bool is_raw;

    if (fn_chained("israw", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `israw` takes 1 argument but %d were given"
                ISRAW_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_scope(query, nd->children->node, e))
        return e->nr;

    is_raw = ti_val_is_raw(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_raw);

    return e->nr;
}
