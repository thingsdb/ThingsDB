#include <ti/fn/fn.h>

#define NEW_DOC_ TI_SEE_DOC("#new")

static int do__f_new(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_not_collection_scope("new", query, e))
        return e->nr;



}
