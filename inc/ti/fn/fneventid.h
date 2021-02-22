#include <ti/fn/fn.h>

static int do__f_event_id(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    if (fn_nargs("event_id", DOC_EVENT_ID, 0, nargs, e))
        return e->nr;

    query->rval = query->ev
            ? (ti_val_t *) ti_vint_create((int64_t) query->ev->id)
            : (ti_val_t *) ti_nil_get();

    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
