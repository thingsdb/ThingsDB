#include <ti/cfn/fn.h>

#define FINDINDEX_DOC_ TI_SEE_DOC("#findindex")

static int cq__f_findindex(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    size_t idx = 0;
    ti_varr_t * varr = (ti_varr_t *) ti_query_val_pop(query);
    ti_closure_t * closure = NULL;

    if (!ti_val_is_array((ti_val_t *) varr))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `findindex`"FINDINDEX_DOC_,
                ti_val_str((ti_val_t *) varr));
        goto done;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `findindex` takes 1 argument but %d were given"
                FINDINDEX_DOC_, nargs);
        goto done;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        goto done;

    if (!ti_val_is_closure(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `findindex` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead"
                FINDINDEX_DOC_, ti_val_str(query->rval));
        goto done;
    }

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (ti_closure_try_lock(closure, e) ||
        ti_scope_local_from_closure(query->scope, closure, e))
        goto done;

    for (vec_each(varr->vec, ti_val_t, v), ++idx)
    {
        _Bool found;

        if (ti_scope_polute_val(query->scope, v, idx))
        {
            ex_set_alloc(e);
            goto done;
        }

        if (ti_cq_optscope(query, ti_closure_scope_nd(closure), e))
            goto done;

        found = ti_val_as_bool(query->rval);
        ti_val_drop(query->rval);

        if (found)
        {
            query->rval = (ti_val_t *) ti_vint_create(idx);
            if (!query->rval)
                ex_set_alloc(e);

            goto done;
        }

        query->rval = NULL;
    }

    query->rval = (ti_val_t *) ti_nil_get();

done:
    ti_closure_unlock(closure);
    ti_val_drop((ti_val_t *) closure);
    ti_val_drop((ti_val_t *) varr);

    return e->nr;
}
