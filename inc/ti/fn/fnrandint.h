#include <ti/fn/fn.h>

static int do__f_randint(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    int64_t a, b;

    if (fn_nargs("randint", DOC_RANDINT, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (fn_arg_int("randint", DOC_RANDINT, 1, query->rval, e))
        return e->nr;

    a = VINT(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next->node, e) ||
        fn_arg_int("randint", DOC_RANDINT, 2, query->rval, e))
        return e->nr;

    b = VINT(query->rval);

    if (b <= a)
    {
        ex_set(e, EX_VALUE_ERROR,
            "function `randint` does not accept an empty range"DOC_RANDINT);
        return e->nr;
    }

    if (b > 0 && a < LLONG_MIN + b)
    {
        ex_set(e, EX_OVERFLOW, "integer overflow");
        return e->nr;
    }

    b -= a;

    /*
     * When the `range` is large relative to RAND_MAX, then simply the module
     * of rand() will result in very badly random numbers (low numbers will
     * significantly appear more since the result of the modulo). For this
     * reason we use a different, slower method for greater randomness.
     */
    if (b > (RAND_MAX >> 2))
    {
        uint64_t r;
        util_randu64(&r);
        a += (int64_t) (r % (uint64_t) b);
    }
    else
    {
        a += rand() % b;
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vint_create(a);

    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
