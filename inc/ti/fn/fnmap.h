static int cq__f_map(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    size_t n;
    ti_varr_t * retvarr = NULL;
    ti_closure_t * closure = NULL;
    ti_val_t * iterval = ti_query_val_pop(query);

    if (iterval->tp != TI_VAL_ARR && iterval->tp != TI_VAL_THING)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `map`",
                ti_val_str(iterval));
        goto failed;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `map` takes 1 argument but %d were given", n);
        goto failed;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        goto failed;

    if (!ti_val_is_closure(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `map` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead",
                ti_val_str(query->rval));
        goto failed;
    }

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    n = ti_val_get_len(iterval);

    if (ti_scope_local_from_closure(query->scope, closure, e))
        goto failed;

    retvarr = ti_varr_create(n);
    if (!retvarr)
        goto failed;

    switch (iterval->tp)
    {
    case TI_VAL_THING:
        for (vec_each(((ti_thing_t *) iterval)->props, ti_prop_t, p))
        {
            if (ti_scope_polute_prop(query->scope, p))
                goto failed;

            if (ti_cq_optscope(query, ti_closure_scope_nd(closure), e))
                goto failed;

            if (ti_varr_append(retvarr, (void **) &query->rval, e))
                goto failed;

            query->rval = NULL;
        }
        break;
    case TI_VAL_ARR:
    {
        int64_t idx = 0;
        for (vec_each(((ti_varr_t *) iterval)->vec, ti_val_t, v), ++idx)
        {
            if (ti_scope_polute_val(query->scope, v, idx))
                goto failed;

            if (ti_cq_optscope(query, ti_closure_scope_nd(closure), e))
                goto failed;

            if (ti_varr_append(retvarr, (void **) &query->rval, e))
                goto failed;

            query->rval = NULL;
        }
        break;
    }
    }

    assert (query->rval == NULL);
    assert (retvarr->vec->n == n);
    query->rval = (ti_val_t *) retvarr;

    goto done;

failed:
    ti_val_drop((ti_val_t *) retvarr);
    if (!e->nr)  /* all not set errors are allocation errors */
        ex_set_alloc(e);

done:
    ti_val_drop((ti_val_t *) closure);
    ti_val_drop(iterval);
    return e->nr;
}
