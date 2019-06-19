#include <ti/cfn/fn.h>

#define FIND_DOC_ TI_SEE_DOC("#find")

static int cq__f_find(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    const int nargs = langdef_nd_n_function_params(nd);
    int64_t idx = 0;
    ti_closure_t * closure = NULL;
    ti_varr_t * varr = (ti_varr_t *) ti_query_val_pop(query);

    if (!ti_val_is_array((ti_val_t *) varr))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `find`"FIND_DOC_,
                ti_val_str((ti_val_t *) varr));
        goto failed;
    }

    if (nargs < 1)
    {
        ex_set(e, EX_BAD_DATA,
                "function `find` requires at least 1 argument but 0 "
                "were given"FIND_DOC_);
        goto failed;
    }

    if (nargs > 2)
    {
        ex_set(e, EX_BAD_DATA,
                "function `find` takes at most 2 arguments but %d "
                "were given"FIND_DOC_, nargs);
        goto failed;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        goto failed;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (closure->tp != TI_VAL_CLOSURE)
    {
        ex_set(e, EX_BAD_DATA,
                "function `find` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead"FIND_DOC_,
                ti_val_str((ti_val_t *) closure));
        goto failed;
    }

    if (ti_scope_local_from_closure(query->scope, closure, e))
        goto failed;

    for (vec_each(varr->vec, ti_val_t, v), ++idx)
    {
        _Bool found;

        if (ti_scope_polute_val(query->scope, v, idx))
            goto failed;

        if (ti_cq_optscope(query, ti_closure_scope_nd(closure), e))
            goto failed;

        found = ti_val_as_bool(query->rval);
        ti_val_drop(query->rval);

        if (found)
        {
            query->rval = v;
            ti_incref(v);
            goto done;
        }

        query->rval = NULL;
    }

    assert (query->rval == NULL);
    if (nargs == 2)
    {
        /* lazy evaluation of the alternative value */
        if (ti_cq_scope(query, nd->children->next->next->node, e))
            goto failed;
    }
    else
    {
        query->rval = (ti_val_t *) ti_nil_get();
    }

    goto done;

failed:
    if (!e->nr)  /* all not set errors are allocation errors */
        ex_set_alloc(e);

done:
    ti_val_drop((ti_val_t *) closure);
    ti_val_drop((ti_val_t *) varr);

    return e->nr;
}
