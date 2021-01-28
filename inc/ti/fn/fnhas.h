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
    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(has);

fail1:
    ti_val_unsafe_drop((ti_val_t *) vset);
    return e->nr;
}

static int do__f_has_thing(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool has;
    ti_thing_t * thing;

    if (fn_nargs("has", DOC_THING_HAS, 1, nargs, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("has", DOC_THING_HAS, 1, query->rval, e))
        goto fail1;

    has = ti_thing_o_has_key(thing, (ti_raw_t *) query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(has);

fail1:
    ti_val_unsafe_drop((ti_val_t *) thing);
    return e->nr;
}

static int do__f_has_list(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool has = false;
    ti_varr_t * varr;

    if (fn_nargs("has", DOC_LIST_HAS, 1, nargs, e))
        return e->nr;

    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e))
        goto fail1;

    for (vec_each(varr->vec, ti_val_t, v))
    {
        if (ti_opr_eq(v, query->rval))
        {
            has = true;
            break;
        }
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(has);

fail1:
    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;
}

static inline int do__f_has(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    switch((ti_val_enum) query->rval->tp)
    {
    case TI_VAL_THING:
        return do__f_has_thing(query, nd, e);
    case TI_VAL_SET:
        return do__f_has_set(query, nd, e);
    case TI_VAL_ARR:
        return do__f_has_list(query, nd, e);
    default:
        return fn_call_try("has", query, nd, e);
    }
    assert(0);
    return -1;
}
