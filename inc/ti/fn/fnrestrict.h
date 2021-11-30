#include <ti/fn/fn.h>

static int do__f_restrict(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_task_t * task;
    ti_thing_t * thing;
    ti_raw_t * rname;
    ti_type_t * type;

    if (!ti_val_is_object(query->rval))
        return fn_call_try("restrict", query, nd, e);

    if (fn_nargs("restrict", DOC_THING_RESTRICT, 1, nargs, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("to_type", DOC_THING_RESTRICT, 1, query->rval, e))
        goto fail0;

    /* TODO: check restriction when imDOC_THING_TO_TYPEplemented */

    rname = (ti_raw_t *) query->rval;
    query->rval = (ti_val_t *) ti_nil_get();

    type = query->collection
       ? ti_types_by_raw(query->collection->types, rname)
       : NULL;

    if (!type)
    {
        (void) ti_raw_err_not_found(rname, "type", e);
        goto fail1;
    }

    if (ti_type_wrap_only_e(type, e) ||
        ti_type_use(type, e) ||
        ti_type_convert(type, thing, query->change, e))
        goto fail1;

    if (thing->id)
    {
        task = ti_task_get_task(query->change, thing);
        if (!task || ti_task_add_to_type(task, type))
        {
            ex_set_mem(e);
            goto fail1;
        }
    }

fail1:
    ti_val_unsafe_drop((ti_val_t *) rname);

fail0:
    ti_val_unsafe_drop((ti_val_t *) thing);
    return e->nr;
}
