#include <ti/cfn/fn.h>

static int cq__f_filter(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * retval = NULL;
    ti_closure_t * closure = NULL;
    ti_val_t * iterval = ti_query_val_pop(query);

    if (iterval->tp != TI_VAL_ARR && iterval->tp != TI_VAL_THING)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `filter`",
                ti_val_str(iterval));
        goto failed;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `filter` takes 1 argument but %d were given", n);
        goto failed;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        goto failed;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (closure->tp != TI_VAL_CLOSURE)
    {
        ex_set(e, EX_BAD_DATA,
                "function `filter` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead",
                ti_val_str((ti_val_t *) closure));
        goto failed;
    }

    if (ti_scope_local_from_closure(query->scope, closure, e))
        goto failed;

    switch (iterval->tp)
    {
    case TI_VAL_THING:
    {
        ti_thing_t * thing;

        if (query->target->things->n >= query->target->quota->max_things)
        {
            ex_set(e, EX_MAX_QUOTA,
                    "maximum things quota of %zu has been reached"
                    TI_SEE_DOC("#quotas"), query->target->quota->max_things);
            goto failed;
        }

        thing = ti_thing_create(0, query->target->things);
        if (!thing)
            goto failed;

        retval = (ti_val_t *) thing;

        for (vec_each(((ti_thing_t *) iterval)->props, ti_prop_t, p))
        {
            if (ti_scope_polute_prop(query->scope, p))
                goto failed;

            if (ti_cq_optscope(query, ti_closure_scope_nd(closure), e))
                goto failed;

            if (ti_val_as_bool(query->rval))
            {
                if (ti_thing_prop_set(thing, p->name, p->val))
                    goto failed;
                ti_incref(p->name);
                ti_incref(p->val);
            }

            ti_val_drop(query->rval);
            query->rval = NULL;
        }
        break;
    }
    case TI_VAL_ARR:
    {
        int64_t idx = 0;
        ti_varr_t * varr = ti_varr_create(((ti_varr_t *) iterval)->vec->n);
        if (!varr)
            goto failed;

        retval = (ti_val_t *) varr;

        for (vec_each(((ti_varr_t *) iterval)->vec, ti_val_t, v), ++idx)
        {
            if (ti_scope_polute_val(query->scope, v, idx))
                goto failed;

            if (ti_cq_optscope(query, ti_closure_scope_nd(closure), e))
                goto failed;

            if (ti_val_as_bool(query->rval))
            {
                ti_incref(v);
                VEC_push(varr->vec, v);
            }

            ti_val_drop(query->rval);
            query->rval = NULL;

        }
        (void) vec_shrink(&varr->vec);
        break;
    }
    }

    assert (query->rval == NULL);
    query->rval = retval;

    goto done;

failed:
    ti_val_drop(retval);
    if (!e->nr)  /* all not set errors are allocation errors */
        ex_set_alloc(e);
done:
    ti_val_drop((ti_val_t *) closure);
    ti_val_drop(iterval);
    return e->nr;
}
