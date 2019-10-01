#include <ti/fn/fn.h>

static int do__f_type_mk(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    ti_field_t * field;
    ti_name_t * name;
    ti_type_t * type;
    cleri_children_t * child;

    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_not_collection_scope("type_mk", query, e) ||
        fn_nargs("type_mk", DOC_TYPE_MK, 4, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_raw("type_mk", DOC_TYPE_MK, 1, query->rval, e))
        return e->nr;

    type = ti_types_by_raw(query->collection->types, (ti_raw_t *) query->rval);
    if (!type)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "type", e);

    ti_val_drop(query->rval);
    query->rval = NULL;
    child = nd->children->next->next;

    if (ti_do_statement(query, child->node, e) ||
        fn_arg_name_check("type_mk", DOC_TYPE_MK, 2, query->rval, e))
        return e->nr;

    name = ti_names_get(
            (const char *) ((ti_raw_t *) query->rval)->data,
            ((ti_raw_t *) query->rval)->n);

    if (!name)
    {
        ex_set_mem(e);
        return e->nr;
    }

    field = ti_field_by_name(type, name);
    if (field)
    {
        ex_set(e, EX_LOOKUP_ERROR,
            "type `%s` already has a property `%s`; "
            "use `type_mod(...)` if you want to change the current property",
            type->name, name->str);
        goto fail0;
    }

    ti_val_drop(query->rval);
    query->rval = NULL;
    child = nd->children->next->next;

    if (ti_do_statement(query, child->node, e) ||


fail0:
    ti_name_drop(name);
    return e->nr;
}
