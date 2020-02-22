#include <ti/fn/fn.h>

static int do__f_has_set(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool has;
    ti_vset_t * vset;

    if (fn_nargs("has", DOC_SET_HAS, 1, nargs, e))
        return e->nr;

    vset = (ti_vset_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_thing("has", DOC_SET_HAS, 1, query->rval, e))
        goto fail1;

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
    ti_name_t * name;
    ti_thing_t * thing;

    if (fn_nargs("has", DOC_THING_HAS, 1, nargs, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("has", DOC_THING_HAS, 1, query->rval, e))
        goto fail1;

    name = ti_names_weak_from_raw((ti_raw_t *) query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(
            name && ti_thing_val_weak_get(thing, name));

fail1:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}

static inline int do__f_has(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    return ti_val_is_thing(query->rval)
            ? do__f_has_thing(query, nd, e)
            : ti_val_is_set(query->rval)
            ? do__f_has_set(query, nd, e)
            : fn_call_try("has", query, nd, e);
}
