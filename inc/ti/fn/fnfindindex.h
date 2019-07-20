#include <ti/fn/fn.h>

#define FINDINDEX_DOC_ TI_SEE_DOC("#findindex")

static int do__f_findindex(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    size_t idx = 0;
    ti_varr_t * varr;
    ti_closure_t * closure;
    int lock_was_set;

    if (!ti_val_is_array(query->rval))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `findindex`"FINDINDEX_DOC_,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `findindex` takes 1 argument but %d were given"
                FINDINDEX_DOC_, nargs);
        return e->nr;
    }

    lock_was_set = ti_val_ensure_lock(query->rval);
    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    if (ti_do_scope(query, nd->children->node, e))
        goto fail0;

    if (!ti_val_is_closure(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `findindex` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead"
                FINDINDEX_DOC_, ti_val_str(query->rval));
        goto fail0;
    }

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_try_lock_and_use(closure, query, e))
        goto fail1;


    for (vec_each(varr->vec, ti_val_t, v), ++idx)
    {
        _Bool found;

        if (ti_closure_vars_val_idx(closure, v, idx))
        {
            ex_set_mem(e);
            goto fail2;
        }

        if (ti_do_scope(query, ti_closure_scope(closure), e))
            goto fail2;

        found = ti_val_as_bool(query->rval);
        ti_val_drop(query->rval);

        if (found)
        {
            query->rval = (ti_val_t *) ti_vint_create(idx);
            if (!query->rval)
                ex_set_mem(e);

            goto done;
        }

        query->rval = NULL;
    }

    query->rval = (ti_val_t *) ti_nil_get();

done:
fail2:
    ti_closure_unlock_use(closure, query);

fail1:
    ti_val_drop((ti_val_t *) closure);

fail0:
    ti_val_unlock((ti_val_t *) varr, lock_was_set);
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}
