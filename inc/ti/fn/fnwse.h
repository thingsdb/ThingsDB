#include <ti/fn/fn.h>

#define WSE_DOC_ TI_SEE_DOC("#wse")

static int do__f_wse(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    _Bool has_wse_flag;

    if (fn_chained("wse", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `wse` takes 1 argument but %d were given"
                WSE_DOC_, nargs);
        return e->nr;
    }

    has_wse_flag = query->syntax.flags & TI_SYNTAX_FLAG_WSE;
    query->syntax.flags |= TI_SYNTAX_FLAG_WSE;

    (void) ti_do_scope(query, nd->children->node, e);

    if (!has_wse_flag)
        query->syntax.flags &= ~TI_SYNTAX_FLAG_WSE;

    return e->nr;
}
