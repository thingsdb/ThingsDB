#include <ti/fn/fn.h>

static int do__f_splice(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    int32_t n, x, l;
    cleri_node_t * child = nd->children;    /* first in argument list */
    uint32_t current_n, new_n;
    int64_t i, c;
    ti_varr_t * retv;
    ti_varr_t * varr;

    if (!ti_val_is_list(query->rval))
        return fn_call_try("splice", query, nd, e);

    n = fn_get_nargs(nd);
    if (fn_nargs_min("splice", DOC_LIST_SPLICE, 2, n, e) ||
        ti_query_test_varr_operation(query, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, child, e) ||
        fn_arg_int("splice", DOC_LIST_SPLICE, 1, query->rval, e))
        goto fail1;

    i = VINT(query->rval);
    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;
    child = child->next->next;

    if (ti_do_statement(query, child, e) ||
        fn_arg_int("splice", DOC_LIST_SPLICE, 2, query->rval, e))
        goto fail1;

    c = VINT(query->rval);
    current_n = varr->vec->n;

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (i < 0)
        i += current_n;

    i = i < 0 ? (c += i) && 0 : (i > current_n ? current_n : i);
    n -= 2;
    c = c < 0 ? 0 : (c > current_n - i ? current_n - i : c);
    new_n = current_n + n - c;

    if (new_n > current_n && vec_reserve(&varr->vec, new_n))
    {
        ex_set_mem(e);
        goto fail1;
    }

    retv = ti_varr_create(c);
    if (!retv)
    {
        ex_set_mem(e);
        goto fail1;
    }

    for (x = i, l = i + c; x < l; ++x)
        VEC_push(retv->vec, VEC_get(varr->vec, x));

    memmove(
        varr->vec->data + i + n,
        varr->vec->data + i + c,
        (current_n - i - c) * sizeof(void*));

    varr->vec->n = i;

    for (x = 0; x < n; ++x)
    {
        child = child->next->next;

        if (ti_do_statement(query, child, e) ||
            ti_val_varr_append(varr, &query->rval, e))
            goto fail2;

        query->rval = NULL;
    }

    if (varr->parent && varr->parent->id && (c || n))
    {
        ti_task_t * task = ti_task_get_task(query->change, varr->parent);
        if (!task || ti_task_add_splice(
                task,
                ti_varr_key(varr),
                varr,
                (uint32_t) i,
                (uint32_t) c,
                (uint32_t) n))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `query->change->tasks` */
    }

    assert(e->nr == 0);

    /* required since query->rval may not be NULL */
    ti_val_drop(query->rval);

    query->rval = (ti_val_t *) retv;
    varr->vec->n = new_n;
    if (new_n < current_n)
        (void) vec_may_shrink(&varr->vec);
    goto done;

alloc_err:
    ex_set_mem(e);

fail2:
    while (x--)
        ti_val_unsafe_drop(vec_pop(varr->vec));

    memmove(
        varr->vec->data + i + n,
        varr->vec->data + i + c,
        (current_n - i - c) * sizeof(void*));

    for (x = 0; x < c; ++x)
        VEC_push(varr->vec, VEC_get(retv->vec, x));

    retv->vec->n = 0;
    ti_val_unsafe_drop((ti_val_t *) retv);
    varr->vec->n = current_n;

done:
fail1:
    ti_val_unlock((ti_val_t *) varr, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;
}
