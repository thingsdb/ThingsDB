#include <ti/fn/fn.h>

static int do__f_enum(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_enum_t * enum_;

    if (fn_not_collection_scope("enum", query, e) ||
        fn_nargs("enum", DOC_ENUM, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `enum` expects argument 1 to be of "
            "type `"TI_VAL_STR_S"` but got type `%s` instead"DOC_ENUM,
            ti_val_str(query->rval));
        return e->nr;
    }

    enum_ = ti_enums_by_raw(query->collection->enums, (ti_raw_t *) query->rval);
    if (!enum_)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "enum", e);

    ti_val_drop(query->rval);
    query->rval = NULL;

    return e->nr;
}
