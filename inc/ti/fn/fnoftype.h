#include <ti/fn/fn.h>


static int do__f_of_type(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_thing_t * thing;
    ti_type_t * type;
    uint64_t id;

    if (!ti_val_is_int(query->rval))
        return fn_call_try("of_type", query, nd, e);

    if (fn_nargs("of_type", DOC_INT_OF_TYPE, 1, nargs, e))
        return e->nr;

    id = VINT(query->rval);
    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e) ||
        fn_arg_str("of_type", DOC_INT_OF_TYPE, 1, query->rval, e))
        return e->nr;

    type = query->collection
        ? ti_types_by_raw(query->collection->types, (ti_raw_t *) query->rval)
        : NULL;
    if (!type)
    {
        (void) ti_raw_err_not_found((ti_raw_t *) query->rval, "type", e);
        return e->nr;
    }

    if (!(thing = ti_query_thing_from_id(query, id, e)))
        return e->nr;

    if (thing->type_id != type->type_id)
    {
        ex_set(e, EX_TYPE_ERROR,
                TI_THING_ID" is of type `%s`, not `%s`",
                thing->id, ti_val_str((ti_val_t *) thing), type->name);
        ti_decref(thing);
        return e->nr;
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) thing;
    return e->nr;
}
