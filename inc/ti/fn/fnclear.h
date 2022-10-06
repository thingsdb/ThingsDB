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
        ti_thing_clear(thing);  /* always an object in this case */

        if (n && thing->id)
        {
            ti_task_t * task = ti_task_get_task(query->change, thing);
            if (!task || ti_task_add_thing_clear(task))
                ti_panic("task clear");
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
                ti_panic("task clear");
        }
        break;
    }
    case TI_VAL_SET:
    {
        ti_vset_t * vset = (ti_vset_t *) val;
        n = vset->imap->n;

        ti_vset_clear(vset);

        if (n && vset->parent && vset->parent->id)
        {
            /* clear must have a task as this is enforced */
            ti_task_t * task = ti_task_get_task(query->change, vset->parent);
            if (!task || ti_task_add_set_clear(task, ti_vset_key(vset)))
                ti_panic("task clear");
        }
        break;
    }
    }

    ti_val_unlock(val, true  /* lock was set */);
    ti_val_unsafe_drop(val);

    return e->nr;
}
