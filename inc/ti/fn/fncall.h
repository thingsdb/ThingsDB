#include <ti/fn/fn.h>

static inline int do__f_call(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    return fn_call(query, nd, e);
}

