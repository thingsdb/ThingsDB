#include <ti/fn/fn.h>

#define REFS_DOC_ TI_SEE_DOC("#refs")

static int do__f_refs(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    uint32_t ref;

    if (fn_chained("refs", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_NUM_ARGUMENTS,
                "function `refs` takes 1 argument but %d were given"
                REFS_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    ref = query->rval->ref;
    ti_val_drop(query->rval);

    query->rval = (ti_val_t *) ti_vint_create((int64_t) ref);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
