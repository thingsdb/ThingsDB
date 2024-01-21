#include <ti/fn/fn.h>

static int do__f_abs(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    if (fn_nargs("abs", DOC_MATH_ABS, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_number("abs", DOC_MATH_ABS, 1, query->rval, e))
        return e->nr;

    if (query->rval->tp == TI_VAL_INT)
    {
        int64_t i = VINT(query->rval);
        if (i >= 0)
            return e->nr;
        if (i == LLONG_MIN)
        {
            ex_set(e, EX_OVERFLOW, "integer overflow");
            return e->nr;
        }
        ti_val_unsafe_drop(query->rval);
        query->rval = (ti_val_t *) ti_vint_create(-i);
    }
    else
    {
        double d = VFLOAT(query->rval);
        if (d >= 0 || isnan(d))
            return e->nr;
        ti_val_unsafe_drop(query->rval);
        query->rval = (ti_val_t *) ti_vfloat_create(-d);
    }

    if (!query->rval)
        ex_set_mem(e);
    return e->nr;
}
