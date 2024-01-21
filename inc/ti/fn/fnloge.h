#include <ti/fn/fn.h>

static int do__f_loge(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    double d;

    if (fn_nargs("loge", DOC_MATH_LOGE, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_number("loge", DOC_MATH_LOGE, 1, query->rval, e))
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
    if (d <= 0)
    {
        ex_set(e, EX_VALUE_ERROR, "math domain error");
        return e->nr;
    }
    d = log(d);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vfloat_create(d);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
