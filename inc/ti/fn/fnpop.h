#include <ti/fn/fn.h>

#define POP_DOC_ TI_SEE_DOC("#pop")

static int do__f_pop(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_varr_t * varr = (ti_varr_t *) ti_query_val_pop(query);
    _Bool is_attached = ti_scope_is_attached(query->scope);

    if (!ti_val_is_list((ti_val_t *) varr))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `pop`"POP_DOC_,
                ti_val_str((ti_val_t *) varr));
        goto done;
    }

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `pop` takes 0 arguments but %d %s given"POP_DOC_,
                nargs, nargs == 1 ? "was" : "were");
        goto done;
    }

    if (is_attached &&
        ti_scope_in_use_val(query->scope->prev, (ti_val_t *) varr))
    {
        ex_set(e, EX_BAD_DATA,
                "cannot use function `pop` while the list is in use"POP_DOC_);
        goto done;
    }

    query->rval = vec_pop(varr->vec);

    if (!query->rval)
    {
        ex_set(e, EX_INDEX_ERROR, "pop from empty array"POP_DOC_);
        goto done;
    }

    (void) vec_shrink(&varr->vec);

    if (is_attached)
    {
        ti_task_t * task;
        assert (query->scope->thing);
        assert (query->scope->name);
        task = ti_task_get_task(query->ev, query->scope->thing, e);
        if (!task)
            goto done;

        if (ti_task_add_splice(
                task,
                query->scope->name,
                NULL,
                varr->vec->n,
                1,
                0))
            ex_set_mem(e);
    }

done:
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}
