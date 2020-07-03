#include <ti/fn/fn.h>

static int do__f_if(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool tobool;

    if (fn_nargs_range("if", DOC_IF, 2, 3, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    tobool = ti_val_as_bool(query->rval);

    if (tobool)
        nd = nd->children->next->next->node;
    else if (nargs == 3)
        nd = nd->children->next->next->next->next->node;
    else
        goto done;

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;
    if (ti_do_statement(query, nd, e))
        return e->nr;

done:
    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();
    return e->nr;
}
