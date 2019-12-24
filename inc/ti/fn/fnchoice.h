#include <ti/fn/fn.h>

static int do__f_choice(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_varr_t * arr;
    size_t n;

    if (!ti_val_is_array(query->rval))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no function `choice`",
                ti_val_str(query->rval));
        return e->nr;
    }

    if (fn_nargs("choice", DOC_LIST_CHOICE, 0, nargs, e))
        return e->nr;

    arr = (ti_varr_t *) query->rval;
    n = arr->vec->n;

    if (n == 0)
    {
        ex_set(e, EX_LOOKUP_ERROR, "choice from empty list");
        return e->nr;
    }

    /*
     * When the `size` is large relative to RAND_MAX, then simply the module
     * of rand() will result in very badly random numbers (low numbers will
     * significantly appear more since the result of the modulo). For this
     * reason we use a different, slower method for greater randomness.
     */
    if (n > (RAND_MAX >> 2))
    {
        uint64_t r;
        util_randu64(&r);
        n = (r % n);
    }
    else
    {
        n = rand() % n;
    }

    query->rval = vec_get(arr->vec, n);
    ti_incref(query->rval);

    ti_val_drop((ti_val_t *) arr);
    return e->nr;
}
