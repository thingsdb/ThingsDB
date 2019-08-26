#include <ti/fn/fn.h>

#define STR_DOC_ TI_SEE_DOC("#str")

static int do__f_str(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    if (fn_chained("str", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        if (nargs == 0)
        {
            assert (query->rval == NULL);
            query->rval = ti_val_empty_str();
            return e->nr;
        }
        ex_set(e, EX_BAD_DATA,
                "function `str` takes at most 1 argument but %d were given"
                STR_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_scope(query, nd->children->node, e))
        return e->nr;

    if (ti_val_convert_to_str(&query->rval))
        ex_set_mem(e);

    return e->nr;
}
