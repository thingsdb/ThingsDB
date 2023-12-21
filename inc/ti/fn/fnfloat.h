#include <ti/fn/fn.h>

static int do__f_float(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    if (fn_nargs_max("float", DOC_FLOAT, 1, nargs, e))
        return e->nr;

    if (nargs == 0)
    {
        assert(query->rval == NULL);
        query->rval = (ti_val_t *) ti_vfloat_create(0.0);
        return e->nr;
    }

    if (ti_do_statement(query, nd->children, e))
        return e->nr;

    return ti_val_convert_to_float(&query->rval, e);
}
