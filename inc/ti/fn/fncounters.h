#include <ti/fn/fn.h>

#define COUNTERS_DOC_ TI_SEE_DOC("#counters")

static int do__f_counters(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    if (fn_not_node_scope("counters", query, e))
        return e->nr;

    assert (!query->ev);    /* node queries do never create an event */

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_NUM_ARGUMENTS,
                "function `counters` takes 0 arguments but %d %s given"
                COUNTERS_DOC_, nargs, nargs == 1 ? "was" : "were");
        return e->nr;
    }

    query->rval = ti_counters_as_qpval();
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
