#include <ti/fn/fn.h>

static int do__f_timer_id(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_timer_t * timer;

    if (fn_nargs("timer_id", DOC_TIMER_ID, 0, nargs, e))
        return e->nr;

    if (query->with_tp != TI_QUERY_WITH_TIMER)
    {
        ex_set(e, EX_OPERATION,
                "function `timer_id` cannot be used outside a timer"
                DOC_TIMER_ID);
        return e->nr;
    }

    timer = query->with.timer;

    query->rval = (ti_val_t *) ti_vint_create((int64_t) timer->id);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
