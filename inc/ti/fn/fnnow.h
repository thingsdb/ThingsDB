#include <ti/fn/fn.h>

#define NOW_DOC_ TI_SEE_DOC("#now")

static int do__f_now(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    if (fn_chained("now", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `now` takes 0 arguments but %d %s given"NOW_DOC_,
                nargs, nargs == 1 ? "was" : "were");
        return e->nr;
    }

    query->rval = (ti_val_t *) ti_vfloat_create(util_now());
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
