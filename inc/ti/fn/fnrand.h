#include <ti/fn/fn.h>

static int do__f_rand(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_nargs("rand", DOC_RAND, 0, nargs, e))
        return e->nr;

    query->rval = (ti_val_t *) ti_vfloat_create((double) rand() / (RAND_MAX));
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
