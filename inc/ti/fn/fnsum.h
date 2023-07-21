#include <ti/fn/fn.h>

typedef struct {
    double d;
    int64_t i;
    int is_float;
} sum__t;

static int sum__add(sum__t * sum, ti_val_t * val, ex_t * e)
{
    switch(val->tp)
    {
    case TI_VAL_INT:
        if (sum->is_float)
        {
            sum->d += VINT(val);
        }
        else
        {
            if ((VINT(val) > 0 && sum->i > LLONG_MAX - VINT(val)) ||
                (VINT(val) < 0 && sum->i < LLONG_MIN - VINT(val)))
                goto overflow;
            sum->i += VINT(val);
        }
        return 0;
    case TI_VAL_FLOAT:
        if (sum->is_float)
        {
            sum->d += VFLOAT(val);
        }
        else
        {
            sum->d = sum->i;
            sum->is_float = 1;
            sum->d += VFLOAT(val);
        }
        return 0;
    case TI_VAL_BOOL:
        if (sum->is_float)
        {
            sum->d += VBOOL(val);
        }
        else
        {
            if (sum->i == LLONG_MAX && VBOOL(val))
                goto overflow;
            sum->i += VBOOL(val);
        }
        return 0;
    default:
        ex_set(e, EX_TYPE_ERROR,
                "`-/+` not supported by type `%s`",
                ti_val_str(val));
        return e->nr;

    }
overflow:
    ex_set(e, EX_OVERFLOW, "integer overflow");
    return e->nr;
}

static int do__f_sum(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_varr_t * varr;
    sum__t sum = {0};

    if (!ti_val_is_array(query->rval))
        return fn_call_try("sum", query, nd, e);

    if (fn_nargs_max("sum", DOC_LIST_SUM, 1, nargs, e))
        return e->nr;

    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    if (nargs)
    {
        if (ti_do_statement(query, nd->children, e) ||
            fn_arg_closure("sum", DOC_LIST_SUM, 1, query->rval, e))
            goto fail0;

        int64_t idx = 0;
        ti_closure_t * closure = (ti_closure_t *) query->rval;
        int lock_was_set = ti_val_ensure_lock((ti_val_t *) varr);
        query->rval = NULL;

        if (ti_closure_try_wse(closure, query, e) ||
            ti_closure_inc(closure, query, e))
            goto fail1;

        for (vec_each(VARR(varr), ti_val_t, v), ++idx)
        {
            if (ti_closure_vars_val_idx(closure, v, idx))
            {
                ex_set_mem(e);
                goto fail2;
            }

            if (ti_closure_do_statement(closure, query, e) ||
                sum__add(&sum, query->rval, e))
                goto fail2;

            ti_val_unsafe_drop(query->rval);
            query->rval = NULL;
        }

fail2:
        ti_closure_dec(closure, query);
fail1:
        ti_val_unsafe_drop((ti_val_t *) closure);
        ti_val_unlock((ti_val_t *) varr, lock_was_set);
    }
    else
        for (vec_each(varr->vec, ti_val_t, v))
            if (sum__add(&sum, v, e))
                break;

    if (e->nr == 0)
    {
        query->rval = sum.is_float
                 ? (ti_val_t *) ti_vfloat_create(sum.d)
                 : (ti_val_t *) ti_vint_create(sum.i);
        if (!query->rval)
            ex_set_mem(e);
    }

fail0:
    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;
}
