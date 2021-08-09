#include <ti/fn/fn.h>

static int do__f_clear(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_vset_t * vset;
    size_t n;

    if (!ti_val_is_set(query->rval))
        return fn_call_try("clear", query, nd, e);

    if (fn_nargs("clear", DOC_SET_CLEAR, 0, nargs, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    vset = (ti_vset_t *) query->rval;
    query->rval = (ti_val_t *) ti_nil_get();

    n = vset->imap->n;

    imap_clear(vset->imap, (imap_destroy_cb) ti_val_unsafe_gc_drop);

    if (n && vset->parent && vset->parent->id)
    {
        ti_task_t * task = ti_task_get_task(query->change, vset->parent);
        if (!task || ti_task_add_set_clear(task, ti_vset_key(vset)))
            ex_set_mem(e);
    }

    ti_val_unlock((ti_val_t *) vset, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) vset);

    return e->nr;
}
