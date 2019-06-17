static int cq__f_hasprop(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_raw_t * rname;
    ti_name_t * name;
    ti_thing_t * thing = (ti_thing_t *) ti_query_val_pop(query);

    if (!ti_val_is_thing((ti_val_t *) thing))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `hasprop`",
                ti_val_str((ti_val_t *) thing));
        goto done;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `hasprop` takes 1 argument but %d were given", n);
        goto done;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        goto done;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `hasprop` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead",
                ti_val_str(query->rval));
        goto done;
    }

    rname = (ti_raw_t *) query->rval;
    name = ti_names_weak_get((const char *) rname->data, rname->n);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(
            name && ti_thing_prop_weak_get(thing, name));

done:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}
