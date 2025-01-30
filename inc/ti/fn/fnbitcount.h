#include <ti/fn/fn.h>

static int do__f_bit_count(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    uint64_t v, c;
    int64_t i;

    if (!ti_val_is_int(query->rval))
        return fn_call_try("bit_count", query, nd, e);

    if (fn_nargs("bit_count", DOC_INT_BIT_COUNT, 0, nargs, e))
        return e->nr;

    i = VINT(query->rval);
    v = i < 0 ? 0-(uint64_t)i : (uint64_t)i;

    // from http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
    c = v - ((v >> 1) & 0x55555555);
    c = ((c >> 2) & 0x33333333) + (c & 0x33333333);
    c = ((c >> 4) + c) & 0x0F0F0F0F;
    c = ((c >> 8) + c) & 0x00FF00FF;
    c = ((c >> 16) + c) & 0x0000FFFF;

    ti_val_unsafe_drop(query->rval);

    /* this cannot fail as all valid values are from stack */
    query->rval = (ti_val_t *) ti_vint_create((int64_t) c);
    return 0;
}
