#include <ti/fn/fn.h>

static int do__f_int(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    if (fn_nargs_max("int", DOC_INT, 1, nargs, e))
        return e->nr;

    if (nargs == 0)
    {
        query->rval = (ti_val_t *) ti_vint_create(0);
        return e->nr;
    }

    if (ti_do_statement(query, nd->children, e))
        return e->nr;

    return ti_val_convert_to_int(&query->rval, e);
}
