#include <ti/fn/fn.h>

static size_t flat__count(vec_t * vec, int64_t i)
{
    size_t n = vec->n;
    if (i > 0)
        for (vec_each(vec, ti_val_t, v))
            if (ti_val_is_array(v))
                n += flat__count(VARR(v), i-1);
    return n;
}

static void flat__fill(ti_varr_t * vold, ti_varr_t * vnew, int64_t i)
{
    vnew->flags |= vold->flags & (TI_VARR_FLAG_MHT|TI_VARR_FLAG_MHR);

    for (vec_each(vold->vec, ti_val_t, v))
    {
        if (i > 0 && ti_val_is_array(v))
        {
            flat__fill((ti_varr_t *) v, vnew, i-1);
        }
        else
        {
            VEC_push(vnew->vec, v);
            ti_incref(v);
        }
    }
}

static int do__f_flat(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    int64_t i = 1;
    ti_varr_t * varr, * vnew;

    if (!ti_val_is_array(query->rval))
        return fn_call_try("flat", query, nd, e);

    if (fn_nargs_max("flat", DOC_LIST_FLAT, 1, nargs, e))
        return e->nr;

    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    if (nargs == 1)
    {
        if (ti_do_statement(query, nd->children, e) ||
            fn_arg_int("flat", DOC_LIST_FLAT, 1, query->rval, e))
            goto done;

        i = VINT(query->rval);
        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }

    vnew = ti_varr_create(flat__count(varr->vec, i));
    if (!vnew)
    {
        ex_set_mem(e);
        goto done;
    }

    flat__fill(varr, vnew, i);
    query->rval = (ti_val_t *) vnew;

done:
    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;
}
