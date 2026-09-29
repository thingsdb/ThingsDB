#include <ti/fn/fn.h>

static int do__f_clear_object(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_thing_t * thing;
    size_t n;

    if (fn_nargs("clear", DOC_THING_CLEAR, 0, nargs, e) ||
        ti_query_test_thing_operation(query, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = (ti_val_t *) ti_nil_get();

    n = ti_thing_n(thing);
    ti_thing_clear(thing);  /* always an object in this case */

    if (n && thing->id)
    {
        ti_task_t * task = ti_task_get_task(query->change, thing);
        if (!task || ti_task_add_thing_clear(task))
            ti_panic("task thing_clear");
    }

    ti_val_unlock((ti_val_t *) thing, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) thing);

    return e->nr;
}

static int do__f_clear_list(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_varr_t * varr;
    size_t n;

    if (fn_nargs("clear", DOC_LIST_CLEAR, 0, nargs, e) ||
        ti_query_test_varr_operation(query, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    varr = (ti_varr_t *) query->rval;
    query->rval = (ti_val_t *) ti_nil_get();

    n = varr->vec->n;
    vec_clear_cb(varr->vec, (vec_destroy_cb) ti_val_unsafe_gc_drop);
    if (n && varr->parent && varr->parent->id)
    {
        ti_task_t * task = ti_task_get_task(query->change, varr->parent);
        if (!task || ti_task_add_arr_clear(task, ti_varr_key(varr)))
            ti_panic("task arr_clear");
    }

    ti_val_unlock((ti_val_t *) varr, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) varr);

    return e->nr;
}

static int do__f_clear_set(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_vset_t * vset;
    size_t n;

    if (fn_nargs("clear", DOC_SET_CLEAR, 0, nargs, e) ||
        ti_query_test_vset_operation(query, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    vset = (ti_vset_t *) query->rval;
    query->rval = (ti_val_t *) ti_nil_get();

    n = vset->imap->n;
    ti_vset_clear(vset);
    if (n && vset->parent && vset->parent->id)
    {
        /* clear must have a task as this is enforced */
        ti_task_t * task = ti_task_get_task(query->change, vset->parent);
        if (!task || ti_task_add_set_clear(task, ti_vset_key(vset)))
            ti_panic("task set_clear");
    }

    ti_val_unlock((ti_val_t *) vset, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) vset);

    return e->nr;
}


static int do__f_clear_dict(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_dict_t * dict;
    size_t n;

    if (fn_nargs("clear", DOC_DICT_CLEAR, 0, nargs, e) ||
        ti_query_test_dict_operation(query, e) ||
        ti_val_try_lock(query->rval, e))
        return e->nr;

    dict = (ti_dict_t *) query->rval;
    query->rval = (ti_val_t *) ti_nil_get();

    n = ti_dict_n(dict);
    ti_dict_clear(dict);
    if (n && dict->parent && dict->parent->id)
    {
        /* clear must have a task as this is enforced */
        ti_task_t * task = ti_task_get_task(query->change, dict->parent);
        if (!task || ti_task_add_dict_clear(task, ti_dict_key(dict)))
            ti_panic("task dict_clear");
    }

    ti_val_unlock((ti_val_t *) dict, true  /* lock was set */);
    ti_val_unsafe_drop((ti_val_t *) dict);

    return e->nr;
}

static inline int do__f_clear(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    return ti_val_is_object(query->rval)
            ? do__f_clear_object(query, nd, e)
            : ti_val_is_list(query->rval)
            ? do__f_clear_list(query, nd, e)
            : ti_val_is_set(query->rval)
            ? do__f_clear_set(query, nd, e)
            : ti_val_is_dict(query->rval)
            ? do__f_clear_dict(query, nd, e)
            : fn_call_try("clear", query, nd, e);
}
