#include <ti/fn/fn.h>

static int do__f_ceil(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    double d;
    int64_t i;
    ti_val_t * v;

    if (fn_nargs("ceil", DOC_MATH_CEIL, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_number("ceil", DOC_MATH_CEIL, 1, query->rval, e))
        return e->nr;

    if (query->rval->tp == TI_VAL_INT)
        return e->nr;  /* if integer, we're done */

    d = ceil(VFLOAT(query->rval));  /* works with NAN and INFINITY */
    if (ti_val_overflow_cast(d))
    {
        ex_set(e, EX_OVERFLOW, "integer overflow");
        return e->nr;
    }
    i = (int64_t) d;
    v = (ti_val_t *) ti_vint_create(i);
    if (!v)
        ex_set_mem(e);

    ti_val_unsafe_drop(query->rval);
    query->rval = v;
    return e->nr;
}
