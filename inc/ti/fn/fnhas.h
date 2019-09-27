#include <ti/fn/fn.h>


static int do__f_has_set(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool has;
    ti_vset_t * vset;

    if (fn_nargs("has", DOC_HAS_SET, 1, nargs, e))
        return e->nr;

    vset = (ti_vset_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e))
        goto fail1;

    if (!ti_val_is_thing(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `has` expects argument 1 to be of "
                "type `"TI_VAL_THING_S"` but got type `%s` instead"
                DOC_HAS_SET,
                ti_val_str(query->rval));
        goto fail1;
    }

    has = ti_vset_has(vset, (ti_thing_t *) query->rval);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(has);

fail1:
    ti_val_drop((ti_val_t *) vset);
    return e->nr;
}

static int do__f_has_thing(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * rname;
    ti_name_t * name;
    ti_thing_t * thing;

    if (fn_nargs("has", DOC_HAS_THING, 1, nargs, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e))
        goto fail1;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `has` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead"
                DOC_HAS_THING,
                ti_val_str(query->rval));
        goto fail1;
    }

    rname = (ti_raw_t *) query->rval;
    name = ti_names_weak_get((const char *) rname->data, rname->n);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(
            name && ti_thing_o_prop_weak_get(thing, name));

fail1:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}

static inline int do__f_has(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    if (fn_not_chained("has", query, e))
        return e->nr;

    if (ti_val_is_thing(query->rval))
        return do__f_has_thing(query, nd, e);

    if (ti_val_is_set(query->rval))
        return do__f_has_set(query, nd, e);

    ex_set(e, EX_LOOKUP_ERROR,
            "type `%s` has no function `has`"DOC_HAS_SET DOC_HAS_THING,
            ti_val_str(query->rval));
    return e->nr;
}
