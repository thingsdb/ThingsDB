#include <ti/fn/fn.h>

static int do__f_pop(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_varr_t * varr;

    if (!ti_val_is_list(query->rval))
        return fn_call_try("pop", query, nd, e);

    if (fn_nargs("pop", DOC_LIST_POP, 0, nargs, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    varr = (ti_varr_t*) query->rval;
    query->rval = vec_pop(varr->vec);

    if (!query->rval)
    {
        ex_set(e, EX_LOOKUP_ERROR, "pop from empty list"DOC_LIST_POP);
        goto done;
    }

    if (varr->parent && varr->parent->id)
    {
        ti_task_t * task = ti_task_get_task(query->ev, varr->parent);
        if (!task || ti_task_add_splice(
                task,
                varr->name,
                NULL,
                varr->vec->n,
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
    VEC_push(varr->vec, query->rval);
    query->rval = NULL;

done:
    ti_val_unlock((ti_val_t *) varr, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;
}
