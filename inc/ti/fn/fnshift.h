#include <ti/fn/fn.h>

static int do__f_shift(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_varr_t * varr;

    if (!ti_val_is_list(query->rval))
        return fn_call_try("shift", query, nd, e);

    if (fn_nargs("shift", DOC_LIST_SHIFT, 0, nargs, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    varr = (ti_varr_t*) query->rval;

    query->rval = varr->vec->n ? vec_remove(varr->vec, 0) : NULL;

    if (!query->rval)
    {
        ex_set(e, EX_LOOKUP_ERROR, "shift from empty list"DOC_LIST_SHIFT);
        goto done;
    }

    if (varr->parent && varr->parent->id)
    {
        ti_task_t * task = ti_task_get_task(query->ev, varr->parent);
        if (!task || ti_task_add_splice(
                task,
                varr->name,
                NULL,
                0,
                1,
                0))
        {
            ex_set_mem(e);
            goto restore;
        }

    }
    else
        ti_thing_may_push_gc((ti_thing_t *) query->rval);

    (void) vec_shrink(&varr->vec);

    goto done;

restore:
    vec_insert(&varr->vec, query->rval, 0);
    query->rval = NULL;

done:
    ti_val_unlock((ti_val_t *) varr, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;
}
