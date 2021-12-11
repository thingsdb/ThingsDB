#include <ti/fn/fn.h>

static int do__f_return(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    log_debug(
            "function return() is deprecated; "
            "use the `return` statement instead");  /* TODO add doc */

    if (fn_nargs_max("return", DOC_RETURN, 2, nargs, e))
        return e->nr;

    if (nargs == 0)
    {
        ex_set_return(e);
        query->rval = (ti_val_t *) ti_nil_get();
        return e->nr;
    }

    if (nargs == 2)
    {
        if (ti_do_statement(query, nd->children->next->next->node, e) ||
            ti_deep_from_val(query->rval, &query->qbind.deep, e))
            return e->nr;

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }

    if (ti_do_statement(query, nd->children->node, e) == 0)
        ex_set_return(e);  /* on success */

    return e->nr;
}
