#include <ti/cfn/fn.h>

#define ADD_DOC_ TI_SEE_DOC("#add")

static int cq__f_add(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    const int nargs = langdef_nd_n_function_params(nd);
    cleri_children_t * child = nd->children;    /* first in argument list */
    vec_t * added = vec_new(nargs);  /* weak references to things */
    ti_vset_t * vset = (ti_vset_t *) ti_query_val_pop(query);
    _Bool is_attached = ti_scope_is_attached(query->scope);

    if (!added)
    {
        ex_set_alloc(e);
        goto done;
    }

    if (!ti_val_is_set((ti_val_t *) vset))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `add`"ADD_DOC_,
                ti_val_str((ti_val_t *) vset));
        goto done;
    }

    if (!nargs)
    {
        ex_set(e, EX_BAD_DATA,
                "function `add` requires at least 1 argument but 0 "
                "were given"ADD_DOC_);
        goto done;
    }

    if (    is_attached &&
            ti_scope_in_use_val(query->scope->prev, (ti_val_t *) vset))
    {
        ex_set(e, EX_BAD_DATA,
            "cannot use function `add` while the set is in use"ADD_DOC_);
        goto done;
    }

    assert (child);

    for (; child; child = child->next->next)
    {
        int rc;
        assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

        if (ti_cq_scope(query, child->node, e))
            goto failed;

        rc = ti_vset_add_val(vset, query->rval, e);

        if (rc < 0)
            goto failed;

        if (rc > 0)
            VEC_push(added, query->rval);  /* at least the `set` has a
                                              reference now */
        ti_val_drop(query->rval);
        query->rval = NULL;

        if (!child->next)
            break;
    }

    if (added->n && is_attached)
    {
        ti_task_t * task = ti_task_get_task(query->ev, query->scope->thing, e);
        if (!task)
            goto failed;

        if (ti_task_add_add(
                task,
                query->scope->name,
                added))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `query->ev->tasks` */
    }

    assert (e->nr == 0);

    query->rval = (ti_val_t *) ti_vint_create((int64_t) added->n);
    if (query->rval)
        goto done;

alloc_err:
    ex_set_alloc(e);

failed:
    while (added->n)
        ti_val_drop((ti_val_t *) ti_vset_pop(vset, vec_pop(added)));

done:
    free(added);
    ti_val_drop((ti_val_t *) vset);
    return e->nr;
}
