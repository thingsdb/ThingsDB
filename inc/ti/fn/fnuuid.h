#include <ti/fn/fn.h>

static int do__f_uuid(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    if (fn_nargs_max("uuid", DOC_UUID, 1, nargs, e))
        return e->nr;

    if (nargs == 0)
    {
        query->rval = (ti_val_t *) ti_uuid_new();
        if (!query->rval)
            ex_set_mem(e);
        return e->nr;
    }

    if (ti_do_statement(query, nd->children, e))
        return e->nr;

    return ti_val(query->rval)->to_uuid(&query->rval, e);
}
