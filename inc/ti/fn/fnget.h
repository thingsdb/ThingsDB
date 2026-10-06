#include <ti/fn/fn.h>

static int do__f_get_thing(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_thing_t * thing;
    ti_witem_t witem;
    _Bool found;

    if (fn_nargs_range("get", DOC_THING_GET, 1, 2, nargs, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e) ||
        fn_arg_str("get", DOC_THING_GET, 1, query->rval, e))
        goto done;

    found = ti_thing_get_by_raw(&witem, thing, (ti_raw_t *) query->rval);

    ti_val_unsafe_drop(query->rval);

    if (!found)
    {
        if (nargs == 2)
        {
            query->rval = NULL;
            (void) ti_do_statement(query, nd->children->next->next, e);
            goto done;
        }

        query->rval = (ti_val_t *) ti_nil_get();
        goto done;
    }

    query->rval = *witem.val;
    ti_incref(query->rval);

done:
    ti_val_unsafe_drop((ti_val_t *) thing);
    return e->nr;
}

static int do__f_get_dict(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_dict_t * dict;
    ti_val_t * val;

    if (fn_nargs_range("get", DOC_DICT_GET, 1, 2, nargs, e))
        return e->nr;

    dict = (ti_dict_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e))
        goto done;

    val = ti_dict_get_weak(dict, query->rval);
    ti_val_unsafe_drop(query->rval);

    if (!val)
    {
        if (nargs == 2)
        {
            query->rval = NULL;
            (void) ti_do_statement(query, nd->children->next->next, e);
            goto done;
        }
        query->rval = (ti_val_t *) ti_nil_get();
        goto done;
    }

    query->rval = val;
    ti_incref(query->rval);

done:
    ti_val_unsafe_drop((ti_val_t *) dict);
    return e->nr;
}

static int do__f_get(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    return ti_val_is_thing(query->rval)
            ? do__f_get_thing(query, nd, e)
            : ti_val_is_dict(query->rval)
            ? do__f_get_dict(query, nd, e)
            : fn_call_try("get", query, nd, e);
}