#include <ti/fn/fn.h>

#define PROCEDURES_INFO_DOC_ TI_SEE_DOC("#procedures_info")

static int do__f_procedures_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (~query->syntax.flags & TI_SYNTAX_FLAG_NODE);
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    vec_t * procedures = query->target
            ? query->target->procedures
            : ti()->procedures;

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `procedures_info` takes 0 arguments but %d %s given"
                PROCEDURES_INFO_DOC_,
                nargs, nargs == 1 ? "was" : "were");
        return e->nr;
    }

    query->rval = ti_procedures_info_as_qpval(procedures);
    if (!query->rval)
        ex_set_alloc(e);

    return e->nr;
}
