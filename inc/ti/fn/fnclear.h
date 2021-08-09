#include <ti/fn/fn.h>

static int do__f_clear(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const char * doc;
    const int nargs = fn_get_nargs(nd);
    ti_val_t * val;
    size_t n;

    doc = doc_clear(query->rval);
    if (!doc)
        return fn_call_try("clear", query, nd, e);

    if (fn_nargs("clear", doc, 0, nargs, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    val = query->rval;
    query->rval = (ti_val_t *) ti_nil_get();

    switch (val->tp)
    {
    case TI_VAL_THING:
    {
        ti_thing_t * thing = (ti_thing_t *) val;
        n = ti_thing_n(thing);
        if (ti_thing_is_dict(thing))
            smap_clear(thing->items.smap, (smap_destroy_cb) ti_item_destroy);
        else
            vec_clear_cb(thing->items.vec, (vec_destroy_cb) ti_prop_destroy);

        if (n && thing->id)
        {
            ti_task_t * task = ti_task_get_task(query->change, thing);
            if (!task || ti_task_add_thing_clear(task))
                ex_set_mem(e);
        }
        break;
    }
    case TI_VAL_ARR:
    {
        ti_varr_t * varr = (ti_varr_t *) val;
        n = varr->vec->n;
        vec_clear_cb(varr->vec, (vec_destroy_cb) ti_val_unsafe_gc_drop);
        if (n && varr->parent && varr->parent->id)
        {
            ti_task_t * task = ti_task_get_task(query->change, varr->parent);
            if (!task || ti_task_add_arr_clear(task, ti_varr_key(varr)))
                ex_set_mem(e);
        }
        break;
    }
    case TI_VAL_SET:
    {
        ti_vset_t * vset = (ti_vset_t *) val;
        n = vset->imap->n;
        imap_clear(vset->imap, (imap_destroy_cb) ti_val_unsafe_gc_drop);
        if (n && vset->parent && vset->parent->id)
        {
            ti_task_t * task = ti_task_get_task(query->change, vset->parent);
            if (!task || ti_task_add_set_clear(task, ti_vset_key(vset)))
                ex_set_mem(e);
        }
        break;
    }
    }

    ti_val_unlock(val, true  /* lock was set */);
    ti_val_unsafe_drop(val);

    return e->nr;
}
