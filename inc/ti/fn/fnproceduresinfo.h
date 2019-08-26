#include <ti/fn/fn.h>

#define PROCEDURES_INFO_DOC_ TI_SEE_DOC("#procedures_info")

static int do__f_procedures_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    vec_t * procedures = query->target
            ? query->target->procedures
            : ti()->procedures;

    if (fn_not_thingsdb_or_collection_scope("procedures_info", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `procedures_info` takes 0 arguments but %d %s given"
                PROCEDURES_INFO_DOC_,
                nargs, nargs == 1 ? "was" : "were");
        return e->nr;
    }

    query->rval = (ti_val_t *) ti_procedures_info(procedures);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
