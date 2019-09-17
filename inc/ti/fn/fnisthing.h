#include <ti/fn/fn.h>

#define ISTHING_DOC_ TI_SEE_DOC("#isthing")

static int do__f_isthing(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    _Bool is_thing;

    if (fn_chained("isthing", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_NUM_ARGUMENTS,
                "function `isthing` takes 1 argument but %d were given"
                ISTHING_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_thing = ti_val_is_thing(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_thing);

    return e->nr;
}
