#include <ti/fn/fn.h>

static int do__f_extend(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    uint32_t current_n, source_n;
    ti_varr_t * varr_dest, * varr_source;

    if (!ti_val_is_list(query->rval))
        return fn_call_try("extend", query, nd, e);

    if (fn_nargs("extend", DOC_LIST_EXTEND, 1, nargs, e) ||
        ti_query_test_varr_operation(query, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    varr_dest = (ti_varr_t *) query->rval;
    query->rval = NULL;

    current_n = varr_dest->vec->n;

    if (ti_do_statement(query, nd->children, e))
        goto fail1;

    if (!ti_val_is_array(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `extend` expects argument 1 to be of "
                "type `"TI_VAL_LIST_S"` or type `"TI_VAL_TUPLE_S"` "
                "but got type `%s` instead"
                DOC_LIST_EXTEND,
                ti_val_str(query->rval));
        goto fail1;
    }

    varr_source = (ti_varr_t *) query->rval;
    query->rval = NULL;

    source_n = varr_source->vec->n;

    if (vec_reserve(&varr_dest->vec, source_n))
        goto alloc_err;

    for (vec_each(varr_source->vec, ti_val_t, v))
    {
        ti_incref(v);
        if (ti_val_varr_append(varr_dest, &v, e))
        {
            ti_decref(v);
            goto undo;
        }
    }

    if (varr_dest->parent && varr_dest->parent->id && source_n)
    {
        ti_task_t * task = ti_task_get_task(query->change, varr_dest->parent);
        if (!task || ti_task_add_splice(
                task,
                ti_varr_key(varr_dest),
                varr_dest,
                current_n,
                0,
                source_n))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `query->change->tasks` */
    }

    assert(e->nr == 0);
    query->rval = (ti_val_t *) ti_vint_create((int64_t) varr_dest->vec->n);
    if (query->rval)
        goto done;

alloc_err:
    ex_set_mem(e);
undo:
    while (varr_dest->vec->n > current_n)
        ti_val_unsafe_drop(VEC_pop(varr_dest->vec));

    (void) vec_shrink(&varr_dest->vec);

done:
    ti_val_unsafe_drop((ti_val_t *) varr_source);

fail1:
    ti_val_unlock((ti_val_t *) varr_dest, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) varr_dest);
    return e->nr;
}
