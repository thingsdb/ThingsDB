#include <ti/fn/fn.h>

#define ISARRAY_DOC_ TI_SEE_DOC("#isarray")

static int do__f_isarray(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    _Bool is_array;

    if (fn_chained("isarray", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_NUM_ARGUMENTS,
                "function `isarray` takes 1 argument but %d were given"
                ISARRAY_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_array = ti_val_is_array(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_array);

    return e->nr;
}
