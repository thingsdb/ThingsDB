#include <ti/fn/fn.h>

#define ADD_DOC_ TI_SEE_DOC("#add")

static int do__f_add(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    const int nargs = langdef_nd_n_function_params(nd);
    cleri_children_t * child = nd->children;    /* first in argument list */
    vec_t * added = vec_new(nargs);  /* weak references to things */
    ti_vset_t * vset;
    ti_chain_t * chain = ti_chained_get(query->chained);

    if (!added)
    {
        ex_set_mem(e);
        goto fail0;
    }

    if (!ti_val_is_set(query->rval))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `add`"ADD_DOC_,
                ti_val_str(query->rval));
        goto fail0;
    }

    if (!nargs)
    {
        ex_set(e, EX_BAD_DATA,
                "function `add` requires at least 1 argument but 0 "
                "were given"ADD_DOC_);
        goto fail0;
    }

    if (ti_chained_in_use(query->chained, chain, e) ||
        ti_val_try_lock(query->rval, e))
        goto fail0;

    vset = (ti_vset_t *) query->rval;
    query->rval = NULL;

    assert (child);

    for (; child; child = child->next->next)
    {
        int rc;
        assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

        if (ti_do_scope(query, child->node, e))
            goto fail1;

        rc = ti_vset_add_val(vset, query->rval, e);

        if (rc < 0)
            goto fail1;

        if (rc > 0)
            VEC_push(added, query->rval);  /* at least the `set` has a
                                              reference now */
        ti_val_drop(query->rval);
        query->rval = NULL;

        if (!child->next)
            break;
    }

    if (added->n && chain)
    {
        ti_task_t * task = ti_task_get_task(query->ev, chain->thing, e);
        if (!task)
            goto fail1;

        if (ti_task_add_add(
                task,
                chain->name,
                added))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `query->ev->tasks` */
    }

    assert (e->nr == 0);

    query->rval = (ti_val_t *) ti_vint_create((int64_t) added->n);
    if (query->rval)
        goto done;

alloc_err:
    ex_set_mem(e);

fail1:
    while (added->n)
        ti_val_drop((ti_val_t *) ti_vset_pop(vset, vec_pop(added)));

done:
    ti_val_drop((ti_val_t *) vset);

fail0:
    free(added);
    return e->nr;
}
