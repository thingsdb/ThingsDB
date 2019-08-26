#include <ti/fn/fn.h>

#define TYPE_DOC_ TI_SEE_DOC("#type")

static int do__f_type(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const char * type_str;

    if (fn_chained("type", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `type` takes 1 argument but %d were given"
                TYPE_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_scope(query, nd->children->node, e))
        return e->nr;

    type_str = ti_val_str(query->rval);
    ti_val_drop(query->rval);

    query->rval = (ti_val_t *) ti_raw_from_strn(type_str, strlen(type_str));
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
