#include <ti/fn/fn.h>

static int do__f_exp(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    double d;

    if (fn_nargs("exp", DOC_MATH_EXP, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_number("exp", DOC_MATH_EXP, 1, query->rval, e))
        return e->nr;

    if (query->rval->tp == TI_VAL_INT)
    {
        int64_t i = VINT(query->rval);
        d = (double) i;
    }
    else
    {
        d = VFLOAT(query->rval);
        if (isnan(d))
            return e->nr;
    }
    d = exp(d);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vfloat_create(d);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
