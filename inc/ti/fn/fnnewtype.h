#include <ti/fn/fn.h>

static int do__f_new_type(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * rname;
    ti_type_t * type;
    ti_thing_t * thing;
    uint16_t type_id;

    if (fn_not_collection_scope("new_type", query, e) ||
        fn_nargs("new_type", DOC_NEW_TYPE, 2, nargs, e))
        return e->nr;

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `new_type` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"DOC_NEW_TYPE,
            ti_val_str(query->rval));
        return e->nr;
    }

    rname = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (!ti_type_is_valid_strn((const char *) rname->data, rname->n))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `new_type` expects argument 1 to be a valid type name"
            TI_SEE_DOC("#types"));
        goto fail0;
    }

    if (ti_types_by_raw(query->collection->types, rname))
    {
        ex_set(e, EX_LOOKUP_ERROR,
            "type `%.*s` already exists; "
            "use `define(...)` if you want to change the type definition"
            DOC_DEFINE,
            (int) rname->n,
            (const char *) rname->data);
        goto fail0;
    }

    type_id = ti_types_get_new_id(query->collection->types, e);
    if (e->nr)
        goto fail0;

    if (ti_do_statement(query, nd->children->next->next->node, e))
        goto fail0;

    if (!ti_val_is_thing(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `new_type` expects argument 1 to be of "
            "type `"TI_VAL_THING_S"` but got type `%s` instead"DOC_NEW_TYPE,
            ti_val_str(query->rval));
        goto fail0;
    }

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    type = ti_type_create(type_id, (const char *) rname->data, rname->n);

    if (!type)
    {
        ex_set_mem(e);
        goto fail1;
    }

    if (ti_type_init_from_thing(type, thing, e))
        goto fail2;

    if (ti_types_add(query->collection->types, type))
    {
        ex_set_internal(e);
        goto fail2;
    }

    goto done;

fail2:
    ti_type_destroy(type);
done:
fail1:
    ti_val_drop((ti_val_t *) thing);
fail0:
    ti_val_drop((ti_val_t *) rname);
    return e->nr;
}
