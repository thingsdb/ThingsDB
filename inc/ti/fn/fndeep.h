#include <ti/fn/fn.h>

#define DEEP_DOC_ TI_SEE_DOC("#deep")

static int do__f_deep(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    if (fn_chained("deep", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_NUM_ARGUMENTS,
                "function `deep` takes 0 arguments but %d %s given"DEEP_DOC_,
                nargs, nargs == 1 ? "was" : "were");
        return e->nr;
    }

    query->rval = (ti_val_t *) ti_vint_create((int64_t) query->syntax.deep);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
