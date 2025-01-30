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
    v = v - ((v >> 1) & (uint64_t)~(uint64_t)0/3);
    v = (v & (uint64_t)~(uint64_t)0/15*3) + ((v >> 2) & (uint64_t)~(uint64_t)0/15*3);
    v = (v + (v >> 4)) & (uint64_t)~(uint64_t)0/255*15;
    c = (uint64_t)(v * ((uint64_t)~(uint64_t)0/255)) >> (sizeof(uint64_t) - 1) * CHAR_BIT;

    ti_val_unsafe_drop(query->rval);

    /* this cannot fail as all valid values are from stack */
    query->rval = (ti_val_t *) ti_vint_create((int64_t) c);
    return 0;
}
