#include <ti/fn/fn.h>

static void type__add(
        ti_query_t * query,
        ti_type_t * type,
        ti_name_t * name,
        cleri_node_t * nd,
        ex_t * e)
{
    static const char * fnname = "type_mod` with task `add";
    size_t n = 0;
    cleri_children_t * child;
    ti_field_t * field = ti_field_by_name(type, name);
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_nargs_range(fnname, DOC_TYPE_MOD, 4, 5, nargs, e))
        return;

    if (field)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "property `%.*s` already exist on type `%s`",
                name->str, type->name);
        return;
    }

    child = nd->children->next->next->next->next->next->next;

    if (ti_do_statement(query, child->node, e) ||
        fn_arg_raw(fnname, DOC_TYPE_MOD, 4, query->rval, e))
        return;

    if (nargs != 5 && (n = ti_collection_ntype(query->collection, type)))
    {
        ex_set(e, EX_OPERATION_ERROR,
                "function `type_mod` requires an initial value when "
                "adding a property to a type with one or more instances; "
                "%zu instance%s of type `%s` found"DOC_TYPE_MOD,
                n, type->name);
        return;
    }




}

static int do__f_type_mod(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    ti_type_t * type;
    ti_name_t * name;
    ti_raw_t * rmod;
    cleri_children_t * child;

    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_not_collection_scope("type_mod", query, e) ||
        fn_nargs_min("type_mod", DOC_TYPE_MOD, 3, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_raw("type_mod", DOC_TYPE_MOD, 1, query->rval, e))
        return e->nr;

    type = ti_types_by_raw(query->collection->types, (ti_raw_t *) query->rval);
    if (!type)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "type", e);

    ti_val_drop(query->rval);
    query->rval = NULL;
    child = nd->children->next->next;

    if (ti_do_statement(query, child->node, e) ||
        fn_arg_name_check("type_mod", DOC_TYPE_MOD, 2, query->rval, e))
        return e->nr;

    rmod = (ti_raw_t *) query->rval;
    query->rval = NULL;
    child = child->next->next;

    if (ti_do_statement(query, child->node, e) ||
        fn_arg_name_check("type_mod", DOC_TYPE_MOD, 3, query->rval, e))
        goto fail0;

    name = ti_names_from_raw(query->rval);
    if (!name)
    {
        ex_set_mem(e);
        goto fail0;
    }

    if (ti_raw_eq_strn(rmod, "add", 3))
    {
        type__add(query, type, name, nd, e);
        goto done;
    }

    if (ti_raw_eq_strn(rmod, "del", 3))
    {
        type__add(query, type, name, nd, e);
        goto done;
    }

    if (ti_raw_eq_strn(rmod, "mod", 3))
    {
        type__add(query, type, name, nd, e);
        goto done;
    }

    ex_set(e, EX_VALUE_ERROR,
            "function `type_mod` expects argument 2 to be "
            "`add`, `del` or `mod` but got `%.*s` instead"DOC_TYPE_MOD,
            (int) rmod->n, (const char *) rmod->data);

done:
    ti_name_drop(name);

fail0:
    ti_val_drop((ti_val_t *) rmod);
    return e->nr;
}
