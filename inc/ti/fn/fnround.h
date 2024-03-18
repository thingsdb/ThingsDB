#include <ti/fn/fn.h>

static int do__f_round(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    int is_int = 0;
    double d, m = 1.0;

    if (fn_nargs_range("round", DOC_MATH_ROUND, 1, 2, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_number("round", DOC_MATH_ROUND, 1, query->rval, e))
        return e->nr;

    if (query->rval->tp == TI_VAL_INT)
    {
        d = (double) VINT(query->rval);;
        is_int = 1;
    }
    else
    {
        d = VFLOAT(query->rval);
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (nargs == 2)
    {
        if (ti_do_statement(query, nd->children->next->next, e) ||
            fn_arg_int("round", DOC_MATH_ROUND, 2, query->rval, e))
            return e->nr;

        m = pow(10.0, (double) VINT(query->rval));
        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }

    d = round(d*m)/m;

    if (is_int)
    {
        int64_t i;
        if (ti_val_overflow_cast(d))
        {
            ex_set(e, EX_OVERFLOW, "integer overflow");
            return e->nr;
        }
        i = (int64_t) d;
        query->rval = (ti_val_t *) ti_vint_create(i);
    }
    else
        query->rval = (ti_val_t *) ti_vfloat_create(d);

    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
