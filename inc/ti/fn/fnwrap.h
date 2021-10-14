#include <ti/fn/fn.h>

static int do__f_wrap(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_type_t * type;
    ti_thing_t * thing;

    if (!ti_val_is_thing(query->rval))
        return fn_call_try("wrap", query, nd, e);

    if (fn_nargs_max("wrap", DOC_THING_WRAP, 1, nargs, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (nargs == 1)
    {
        if (ti_do_statement(query, nd->children->node, e) ||
            fn_arg_str("wrap", DOC_THING_WRAP, 1, query->rval, e))
            goto fail0;

        type = query->collection
            ? ti_types_by_raw(query->collection->types, (ti_raw_t *) query->rval)
            : NULL;

        if (!type)
        {
            (void) ti_raw_err_not_found((ti_raw_t *) query->rval, "type", e);
            goto fail0;
        }

        ti_val_unsafe_drop(query->rval);
    }
    else if (ti_thing_is_object(thing))
    {
        ex_set(e, EX_NUM_ARGUMENTS,
                "using `wrap` on a non-typed `"TI_VAL_THING_S"` requires 1 "
                "argument but 0 were given"DOC_THING_WRAP);
        goto fail0;
    }
    else
    {
        /* Instance, wrap with itself to calculate methods. */
        type = thing->via.type;
    }

    query->rval = (ti_val_t *) ti_wrap_create(thing, type->type_id);
    if (!query->rval)
        ex_set_mem(e);

fail0:
    ti_val_unsafe_drop((ti_val_t *) thing);
    return e->nr;
}
