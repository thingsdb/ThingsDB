#include <ti/cfn/fn.h>

#define PUSH_DOC_ TI_SEE_DOC("#push")

static int cq__f_push(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    const int nargs = langdef_nd_n_function_params(nd);
    cleri_children_t * child = nd->children;    /* first in argument list */
    uint32_t current_n, new_n;
    ti_varr_t * varr = (ti_varr_t *) ti_query_val_pop(query);

    if (!ti_val_is_list((ti_val_t *) varr))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `push`"PUSH_DOC_,
                ti_val_str((ti_val_t *) varr));
        goto done;
    }

    if (!nargs)
    {
        ex_set(e, EX_BAD_DATA,
                "function `push` requires at least 1 argument but 0 "
                "were given"PUSH_DOC_);
        goto done;
    }

    if (ti_varr_is_assigned(varr) &&
        ti_scope_in_use_val(query->scope->prev, (ti_val_t *) varr))
    {
        ex_set(e, EX_BAD_DATA,
            "cannot use function `push` while the list is in use"PUSH_DOC_);
        goto done;
    }

    current_n = varr->vec->n;
    new_n = current_n + nargs;

    if (new_n >= query->target->quota->max_array_size)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum array size quota of %zu has been reached"
                TI_SEE_DOC("#quotas"), query->target->quota->max_array_size);
        goto done;
    }

    if (vec_resize(&varr->vec, new_n))
    {
        ex_set_alloc(e);
        goto done;
    }

    assert (child);

    for (; child; child = child->next->next)
    {
        assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

        if (ti_cq_scope(query, child->node, e))
            goto failed;

        if (ti_varr_append(varr, (void **) &query->rval, e))
            goto failed;

        query->rval = NULL;

        if (!child->next)
            break;
    }

    if (ti_varr_is_assigned(varr))
    {
        ti_task_t * task;
        assert (query->scope->thing);
        assert (query->scope->name);
        task = ti_task_get_task(query->ev, query->scope->thing, e);
        if (!task)
            goto failed;

        if (ti_task_add_splice(
                task,
                query->scope->name,
                varr,
                current_n,
                0,
                nargs))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `query->ev->tasks` */
    }

    assert (e->nr == 0);
    query->rval = (ti_val_t *) ti_vint_create((int64_t) varr->vec->n);
    if (query->rval)
        goto done;

alloc_err:
    ex_set_alloc(e);

failed:
    while (varr->vec->n > current_n)
    {
        ti_val_drop(vec_pop(varr->vec));
    }
    (void) vec_shrink(&varr->vec);

done:
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}
