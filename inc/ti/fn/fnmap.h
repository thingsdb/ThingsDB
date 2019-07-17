#include <ti/fn/fn.h>

#define MAP_DOC_ TI_SEE_DOC("#map")

static int do__f_map(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    size_t n;
    ti_varr_t * retvarr = NULL;
    ti_closure_t * closure = NULL;
    ti_val_t * iterval = ti_query_val_pop(query);

    if (    iterval->tp != TI_VAL_ARR &&
            iterval->tp != TI_VAL_SET &&
            iterval->tp != TI_VAL_THING)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `map`"MAP_DOC_,
                ti_val_str(iterval));
        goto fail1;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `map` takes 1 argument but %d were given"MAP_DOC_,
                nargs);
        goto fail1;
    }

    if (ti_do_scope(query, nd->children->node, e))
        goto fail1;

    if (!ti_val_is_closure(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `map` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead"MAP_DOC_,
                ti_val_str(query->rval));
        goto fail1;
    }

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    n = ti_val_get_len(iterval);

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_try_lock(closure, e))
        goto fail1;

    if (ti_scope_local_from_closure(query->scope, closure, e))
        goto fail2;

    retvarr = ti_varr_create(n);
    if (!retvarr)
        goto fail2;

    switch (iterval->tp)
    {
    case TI_VAL_THING:
        for (vec_each(((ti_thing_t *) iterval)->props, ti_prop_t, p))
        {
            if (ti_scope_polute_prop(query->scope, p))
                goto fail2;

            if (ti_do_optscope(query, ti_closure_scope_nd(closure), e))
                goto fail2;

            if (ti_varr_append(retvarr, (void **) &query->rval, e))
                goto fail2;

            query->rval = NULL;
        }
        break;
    case TI_VAL_ARR:
    {
        int64_t idx = 0;
        for (vec_each(((ti_varr_t *) iterval)->vec, ti_val_t, v), ++idx)
        {
            if (ti_scope_polute_val(query->scope, v, idx))
                goto fail2;

            if (ti_do_optscope(query, ti_closure_scope_nd(closure), e))
                goto fail2;

            if (ti_varr_append(retvarr, (void **) &query->rval, e))
                goto fail2;

            query->rval = NULL;
        }
        break;
    }
    case TI_VAL_SET:
    {
        vec_t * vec = imap_vec(((ti_vset_t *) iterval)->imap);
        if (!vec)
            goto fail2;
        for (vec_each(vec, ti_thing_t, t))
        {
            if (ti_scope_polute_val(query->scope, (ti_val_t *) t, t->id))
                goto fail2;

            if (ti_do_optscope(query, ti_closure_scope_nd(closure), e))
                goto fail2;

            if (ti_varr_append(retvarr, (void **) &query->rval, e))
                goto fail2;

            query->rval = NULL;
        }
    }
    }

    assert (query->rval == NULL);
    assert (retvarr->vec->n == n);
    query->rval = (ti_val_t *) retvarr;

    goto done;

fail2:
    if (!e->nr)
        ex_set_mem(e);
    ti_val_drop((ti_val_t *) retvarr);

done:
    ti_closure_unlock(closure);

fail1:
    ti_val_drop((ti_val_t *) closure);
    ti_val_drop(iterval);
    return e->nr;
}
