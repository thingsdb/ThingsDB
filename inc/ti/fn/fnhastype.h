#include <ti/fn/fn.h>

static int do__f_has_type(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool has_type;

    if (fn_not_collection_scope("has_type", query, e) ||
        fn_nargs("has_type", DOC_HAS_TYPE, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `has_type` expects argument 1 to be of "
            "type `"TI_VAL_STR_S"` but got type `%s` instead"
            DOC_HAS_TYPE,
            ti_val_str(query->rval));
        return e->nr;
    }

    has_type = !!ti_types_by_raw(
            query->collection->types,
            (ti_raw_t *) query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(has_type);

    return e->nr;
}
