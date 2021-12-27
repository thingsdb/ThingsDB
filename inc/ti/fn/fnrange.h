#include <ti/fn/fn.h>

#define TI_RANGE_MAX 1024

static int do__f_range(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    int64_t start = 0, stop, step = 1, n;
    ti_varr_t * varr;
    cleri_node_t * child = nd->children;

    if (fn_nargs_range("range", DOC_RANGE, 1, 3, nargs, e) ||
        ti_do_statement(query, child, e) ||
        fn_arg_int("range", DOC_RANGE, 1, query->rval, e))
        return e->nr;

    stop = VINT(query->rval);
    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (nargs >= 2)
    {
        if (ti_do_statement(query, (child = child->next->next), e) ||
            fn_arg_int("range", DOC_RANGE, 2, query->rval, e))
            return e->nr;

        start = stop;
        stop = VINT(query->rval);
        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }

    if (nargs == 3)
    {
        if (ti_do_statement(query, (child = child->next->next), e) ||
            fn_arg_int("range", DOC_RANGE, 3, query->rval, e))
            return e->nr;

        step = VINT(query->rval);
        if (step == 0)
        {
            ex_set(e, EX_VALUE_ERROR, "step value must not be zero"DOC_RANGE);
            return e->nr;
        }

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }

    n = stop - start;
    n = n / step + !!(n % step);

    if (n > TI_RANGE_MAX)
    {
        ex_set(e, EX_OPERATION,
                "maximum range length exceeded"DOC_RANGE);
        return e->nr;
    }

    varr = ti_varr_create(n > 0 ? n : 0);
    if (!varr)
    {
        ex_set_mem(e);
        return e->nr;
    }

    query->rval = (ti_val_t *) varr;

    while (n-- > 0)
    {
        ti_vint_t * vint = ti_vint_create(start);
        if (!vint)
        {
            ex_set_mem(e);
            return e->nr;
        }
        VEC_push(varr->vec, vint);
        start += step;
    }

    return e->nr;
}
