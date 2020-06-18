#include <ti/fn/fn.h>

static int do__f_new(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    int lock_was_set;
    ti_type_t * type;
    ti_thing_t * new_thing, * from_thing;

    if (fn_not_collection_scope("new", query, e) ||
        fn_nargs_range("new", DOC_NEW, 1, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `new` expects argument 1 to be of "
            "type `"TI_VAL_STR_S"` but got type `%s` instead"DOC_NEW,
            ti_val_str(query->rval));
        return e->nr;
    }

    type = ti_types_by_raw(query->collection->types, (ti_raw_t *) query->rval);
    if (!type)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "type", e);

    ti_val_drop(query->rval);
    query->rval = NULL;

    if (nargs == 1)
    {
        query->rval = ti_type_dval(type);
        if (!query->rval)
            ex_set_mem(e);
        return e->nr;
    }

    lock_was_set = ti_type_ensure_lock(type);

    (void) ti_do_statement(query, nd->children->next->next->node, e);
    /* make sure we unlock */
    ti_type_unlock(type, lock_was_set);

    if (e->nr)
        return e->nr;

    if (!ti_val_is_thing(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "new instances can be created from type `"TI_VAL_THING_S"` but "
            "got type `%s` instead"DOC_NEW,
            ti_val_str(query->rval));
        return e->nr;
    }

    from_thing = (ti_thing_t *) query->rval;

    new_thing = ti_type_from_thing(type, from_thing, e);

    ti_val_drop(query->rval);  /* from_thing */
    query->rval = (ti_val_t *) new_thing;

    return e->nr;
}
