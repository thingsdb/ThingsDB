#include <ti/fn/fn.h>

static int do__f_wrap(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const char * doc;
    const int nargs = langdef_nd_n_function_params(nd);
    ti_type_t * type;
    ti_thing_t * thing;

    if (fn_not_chained("wrap", query, e))
        return e->nr;

    doc = doc_wrap(query->rval);
    if (!doc)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no function `wrap`",
                ti_val_str(query->rval));
        return e->nr;
    }

    if (fn_nargs("wrap", doc, 1, nargs, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e))
        goto fail0;

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `wrap` expects argument 1 to be of "
            "type `"TI_VAL_STR_S"` but got type `%s` instead"
            DOC_TYPE_INFO,
            ti_val_str(query->rval));
        goto fail0;
    }

    type = ti_types_by_raw(query->collection->types, (ti_raw_t *) query->rval);
    if (!type)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "type", e);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_wrap_create(thing, type->type_id);
    if (!query->rval)
        ex_set_mem(e);

fail0:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}
