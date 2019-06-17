static int cq__f_splice(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool from_scope = !query->rval;
    int32_t n, x, l;
    cleri_children_t * child = nd->children;    /* first in argument list */
    uint32_t current_n, new_n;
    int64_t i, c;
    ti_varr_t * retv;
    ti_varr_t * varr = (ti_varr_t *) ti_query_val_pop(query);

    if (!ti_val_is_list((ti_val_t *) varr))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `splice`",
                ti_val_str((ti_val_t *) varr));
        goto done;
    }

    n = langdef_nd_n_function_params(nd);
    if (n < 2)
    {
        ex_set(e, EX_BAD_DATA,
                "function `splice` requires at least 2 arguments "
                "but %d %s given",
                n, n == 1 ? "was" : "were");
        goto done;
    }

    if (from_scope && ti_scope_current_val_in_use(query->scope))
    {
        ex_set(e, EX_BAD_DATA,
                "cannot use function `splice` while the list is in use");
        goto done;
    }

    if (ti_cq_scope(query, child->node, e))
        goto done;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `splice` expects argument 1 to be of "
                "type `"TI_VAL_INT_S"` but got type `%s` instead",
                ti_val_str(query->rval));
        goto done;
    }

    i = ((ti_vint_t *) query->rval)->int_;
    ti_val_drop(query->rval);
    query->rval = NULL;
    child = child->next->next;

    if (ti_cq_scope(query, child->node, e))
        goto done;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `splice` expects argument 2 to be of "
                "type `"TI_VAL_INT_S"` but got type `%s` instead",
                ti_val_str(query->rval));
        goto done;
    }

    c = ((ti_vint_t *) query->rval)->int_;
    current_n = varr->vec->n;

    ti_val_drop(query->rval);
    query->rval = NULL;

    if (i < 0)
        i += current_n;

    i = i < 0 ? (c += i) && 0 : (i > current_n ? current_n : i);
    n -= 2;
    c = c < 0 ? 0 : (c > current_n - i ? current_n - i : c);
    new_n = current_n + n - c;

    if (new_n >= query->target->quota->max_array_size)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum array size quota of %zu has been reached, "
                "see "TI_DOCS"#quotas", query->target->quota->max_array_size);
        goto done;
    }

    if (new_n > current_n && vec_resize(&varr->vec, new_n))
    {
        ex_set_alloc(e);
        goto done;
    }

    retv = ti_varr_create(c);
    if (!retv)
    {
        ex_set_alloc(e);
        goto done;
    }

    for (x = i, l = i + c; x < l; ++x)
        VEC_push(retv->vec, vec_get(varr->vec, x));

    memmove(
        varr->vec->data + i + n,
        varr->vec->data + i + c,
        (current_n - i - c) * sizeof(void*));

    varr->vec->n = i;

    for (x = 0; x < n; ++x)
    {
        child = child->next->next;
        assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

        if (ti_cq_scope(query, child->node, e))
            goto failed;

        if (ti_varr_append(varr, (void **) &query->rval, e))
            goto failed;

        query->rval = NULL;
    }

    if (from_scope)
    {
        ti_task_t * task;

        task = ti_task_get_task(query->ev, query->scope->thing, e);
        if (!task)
            goto failed;

        if (ti_task_add_splice(
                task,
                query->scope->name,
                varr,
                i,
                c,
                n))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `query->ev->tasks` */
    }

    assert (e->nr == 0);

    /* required since query->rval may not be NULL */
    ti_val_drop(query->rval);

    query->rval = (ti_val_t *) retv;
    varr->vec->n = new_n;
    if (new_n < current_n)
        (void) vec_shrink(&varr->vec);
    goto done;

alloc_err:
    ex_set_alloc(e);

failed:
    while (x--)
        ti_val_drop(vec_pop(varr->vec));

    memmove(
        varr->vec->data + i + n,
        varr->vec->data + i + c,
        (current_n - i - c) * sizeof(void*));

    for (x = 0; x < c; ++x)
        VEC_push(varr->vec, vec_get(retv->vec, x));

    retv->vec->n = 0;
    ti_val_drop((ti_val_t *) retv);
    varr->vec->n = current_n;

done:
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}
