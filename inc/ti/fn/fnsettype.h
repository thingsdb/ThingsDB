#include <ti/fn/fn.h>

static int do__f_set_type(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * rname;
    ti_type_t * type;
    ti_thing_t * thing;
    ti_task_t * task;
    uint16_t type_id;

    if (fn_not_collection_scope("new_type", query, e) ||
        fn_nargs("new_type", DOC_NEW_TYPE, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `new_type` expects argument 1 to be of "
            "type `"TI_VAL_STR_S"` but got type `%s` instead"DOC_NEW_TYPE,
            ti_val_str(query->rval));
        return e->nr;
    }

    rname = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (!ti_type_is_valid_strn((const char *) rname->data, rname->n))
    {
        if (rname->n && !isupper((int) *rname->data))
            ex_set(e, EX_VALUE_ERROR,
                "function `new_type` expects "
                "argument 1 to be a valid type name; "
                "type names must start with an upper-case character"DOC_TYPES);
        else
            ex_set(e, EX_VALUE_ERROR,
                "function `new_type` expects "
                "argument 1 to be a valid type name"DOC_TYPES);
        goto fail0;
    }

    if (ti_types_by_raw(query->collection->types, rname))
    {
        ex_set(e, EX_LOOKUP_ERROR,
            "type `%.*s` already exists; "
            "use `mod_type(..)` if you want to change the type definition"
            DOC_MOD_TYPE,
            (int) rname->n,
            (const char *) rname->data);
        goto fail0;
    }

    type_id = ti_types_get_new_id(query->collection->types, rname, e);
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

    type = ti_type_create(
            query->collection->types,
            type_id,
            (const char *) rname->data,
            rname->n);

    if (!type)
    {
        ex_set_mem(e);
        goto fail1;
    }

    if (ti_type_init_from_thing(type, thing, e))
        goto fail2;

    task = ti_task_get_task(query->ev, query->collection->root, e);
    if (!task)
        goto fail2;

    if (ti_task_add_new_type(task, type))
    {
        ex_set_mem(e);
        goto fail2;
    }

    query->rval = (ti_val_t *) ti_nil_get();

    goto done;

fail2:
    assert (e->nr);
    ti_type_drop(type);

done:
fail1:
    ti_val_drop((ti_val_t *) thing);
fail0:
    ti_val_drop((ti_val_t *) rname);
    return e->nr;
}
