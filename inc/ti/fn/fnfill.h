#include <ti/fn/fn.h>

static int do__f_fill(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_varr_t * varr;

    if (!ti_val_is_list(query->rval))
        return fn_call_try("fill", query, nd, e);

    if (fn_nargs("fill", DOC_LIST_FILL, 1, nargs, e) ||
        ti_query_test_varr_operation(query, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    varr = (ti_varr_t*) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e) ||
        ti_val_varr_fill(varr, &query->rval, e))
        goto done;

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) varr;
    ti_incref(varr);

    if (varr->vec->n && varr->parent && varr->parent->id)
    {
        ti_task_t * task = ti_task_get_task(query->change, varr->parent);
        if (!task || ti_task_add_fill(
                task,
                ti_varr_key(varr),
                VEC_get(varr->vec, 0)))
        {
            ti_panic("task `fill` creation has failed");
            ex_set_mem(e);
        }
    }

done:
    ti_val_unlock((ti_val_t *) varr, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;
}
