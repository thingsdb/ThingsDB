#include <ti/cfn/fn.h>

#define FILTER_DOC_ TI_SEE_DOC("#filter")

static int cq__f_filter(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * retval = NULL;
    ti_closure_t * closure = NULL;
    ti_val_t * iterval = ti_query_val_pop(query);

    if (    iterval->tp != TI_VAL_ARR &&
            iterval->tp != TI_VAL_SET &&
            iterval->tp != TI_VAL_THING)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `filter`"FILTER_DOC_,
                ti_val_str(iterval));
        goto fail1;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `filter` takes 1 argument but %d were given"
                FILTER_DOC_, nargs);
        goto fail1;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        goto fail1;

    if (!ti_val_is_closure(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `filter` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead"FILTER_DOC_,
                ti_val_str(query->rval));
        goto fail1;
    }

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (ti_closure_try_lock(closure, e))
        goto fail1;

    if (ti_scope_local_from_closure(query->scope, closure, e))
        goto fail2;

    switch (iterval->tp)
    {
    case TI_VAL_THING:
    {
        ti_thing_t * thing;

        if (ti_quota_things(query->target->quota, query->target->things->n, e))
            goto fail2;

        thing = ti_thing_create(0, query->target->things);
        if (!thing)
            goto fail2;

        retval = (ti_val_t *) thing;

        for (vec_each(((ti_thing_t *) iterval)->props, ti_prop_t, p))
        {
            if (ti_scope_polute_prop(query->scope, p))
                goto fail2;

            if (ti_cq_optscope(query, ti_closure_scope_nd(closure), e))
                goto fail2;

            if (ti_val_as_bool(query->rval))
            {
                if (    ti_val_make_assignable(&p->val, e) ||
                        ti_thing_prop_set(thing, p->name, p->val))
                    goto fail2;
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
            goto fail2;

        retval = (ti_val_t *) varr;

        for (vec_each(((ti_varr_t *) iterval)->vec, ti_val_t, v), ++idx)
        {
            if (ti_scope_polute_val(query->scope, v, idx))
                goto fail2;

            if (ti_cq_optscope(query, ti_closure_scope_nd(closure), e))
                goto fail2;

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
    case TI_VAL_SET:
    {
        vec_t * vec = imap_vec(((ti_vset_t *) iterval)->imap);
        ti_vset_t * vset = ti_vset_create();
        if (!vset || !vec)
            goto fail2;

        retval = (ti_val_t *) vset;

        for (vec_each(vec, ti_thing_t, t))
        {
            if (ti_scope_polute_val(query->scope, (ti_val_t *) t, t->id))
                goto fail2;

            if (ti_cq_optscope(query, ti_closure_scope_nd(closure), e))
                goto fail2;

            if (ti_val_as_bool(query->rval))
            {
                if (ti_vset_add(vset, t))
                    goto fail2;
                ti_incref(t);
            }

            ti_val_drop(query->rval);
            query->rval = NULL;
        }
    }
    }

    assert (query->rval == NULL);
    query->rval = retval;

    goto done;

fail2:
    if (!e->nr)
        ex_set_alloc(e);
    ti_val_drop(retval);

done:
    ti_closure_unlock(closure);

fail1:
    ti_val_drop((ti_val_t *) closure);
    ti_val_drop(iterval);
    return e->nr;
}
