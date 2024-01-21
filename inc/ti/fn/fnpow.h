#include <ti/fn/fn.h>

static int do__f_pow(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    double base;
    double power;
    double d;

    if (fn_nargs("pow", DOC_MATH_POW, 2, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_number("pow", DOC_MATH_POW, 1, query->rval, e))
        return e->nr;

    if (query->rval->tp == TI_VAL_INT)
    {
        int64_t i = VINT(query->rval);
        base = (double) i;
    }
    else
    {
        base = VFLOAT(query->rval);
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next, e) ||
        fn_arg_number("pow", DOC_MATH_POW, 2, query->rval, e))
        return e->nr;

    if (query->rval->tp == TI_VAL_INT)
    {
        int64_t i = VINT(query->rval);
        power = (double) i;
    }
    else
    {
        power = VFLOAT(query->rval);
    }

    ti_val_unsafe_drop(query->rval);
    d = pow(base, power);

    query->rval = (ti_val_t *) ti_vfloat_create(d);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
