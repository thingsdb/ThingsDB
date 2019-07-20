#include <ti/fn/fn.h>

#define POP_DOC_ TI_SEE_DOC("#pop")

static int do__f_pop(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_varr_t * varr;
    ti_chain_t * chain = ti_chained_get(query->chained);


    if (!ti_val_is_list(query->rval))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `pop`"POP_DOC_,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `pop` takes 0 arguments but %d %s given"POP_DOC_,
                nargs, nargs == 1 ? "was" : "were");
        return e->nr;
    }

    if (ti_chained_in_use(query->chained, chain, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    varr = (ti_varr_t*) query->rval;
    query->rval = vec_pop(varr->vec);

    if (!query->rval)
    {
        ex_set(e, EX_INDEX_ERROR, "pop from empty array"POP_DOC_);
        goto fail1;
    }

    (void) vec_shrink(&varr->vec);

    if (chain)
    {
        ti_task_t * task;
        task = ti_task_get_task(query->ev, chain->thing, e);
        if (!task)
            goto fail1;

        if (ti_task_add_splice(
                task,
                chain->name,
                NULL,
                varr->vec->n,
                1,
                0))
            ex_set_mem(e);
    }

fail1:
    ti_val_unlock((ti_val_t *) varr, true  /* lock was set */);
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}
