#include <ti/fn/fn.h>

#define DEFINE_DOC_ TI_SEE_DOC("#define")

static int do__f_define(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * rname;
    ti_type_t * type;

    if (fn_not_collection_scope("define", query, e) ||
        fn_nargs("define", DEFINE_DOC_, 2, nargs, e))
        return e->nr;

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `define` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"DEFINE_DOC_,
            ti_val_str(query->rval));
        return e->nr;
    }

    rname = query->rval;
    query->rval = NULL;

    if (!ti_type_is_valid_strn((const char *) rname->data, rname->n))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `define` expects argument 1 to be a valid type name"
            TI_SEE_DOC("#types"));
        goto fail0;
    }

    type = ti_types_by_raw(query->collection->types, rname);

    if (ti_do_statement(query, nd->children->next->next->node, e))
        goto fail0;

    if (!ti_val_is_thing(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `define` expects argument 1 to be of "
            "type `"TI_VAL_THING_S"` but got type `%s` instead"DEFINE_DOC_,
            ti_val_str(query->rval));
        goto fail0;
    }

    if (type)
    {
        // exists
    }
    else
    {
        // new
    }




fail0:
    ti_val_drop((ti_val_t *) rname);
    return e->nr;
}
