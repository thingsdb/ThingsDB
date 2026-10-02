#include <ti/fn/fn.h>


static int do__f_restriction_thing(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_thing_t * thing;

    if (fn_nargs("restriction", DOC_THING_RESTRICTION, 0, nargs, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;

    query->rval = thing->via.spec != TI_SPEC_ANY
        ? (ti_val_t *) ti_spec_raw(thing->via.spec, thing->collection)
        : (ti_val_t *) ti_nil_get();

    if (!query->rval)
        ex_set_mem(e);

    ti_val_unsafe_drop((ti_val_t *) thing);
    return e->nr;
}

static int do__f_restriction_list(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_varr_t * varr;
    uint16_t spec;

    if (fn_nargs("restriction", DOC_LIST_RESTRICTION, 0, nargs, e))
        return e->nr;

    varr = (ti_varr_t *) query->rval;
    spec = ti_varr_spec(varr);
    query->rval = spec != TI_SPEC_ANY
        ? (ti_val_t *) ti_spec_raw(spec, query->collection)
        : (ti_val_t *) ti_nil_get();

    if (!query->rval)
        ex_set_mem(e);

    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;
}

static int do__f_restriction_set(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_vset_t * vset;
    uint16_t spec;

    if (fn_nargs("restriction", DOC_SET_RESTRICTION, 0, nargs, e))
        return e->nr;

    vset = (ti_vset_t *) query->rval;
    spec = ti_vset_spec(vset);
    query->rval = spec != TI_SPEC_ANY
        ? (ti_val_t *) ti_spec_raw(spec, query->collection)
        : (ti_val_t *) ti_nil_get();

    if (!query->rval)
        ex_set_mem(e);

    ti_val_unsafe_drop((ti_val_t *) vset);
    return e->nr;
}

static int do__f_restriction_dict(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_dict_t * dict;
    ti_varr_t * varr;
    ti_raw_t * key, * val;

    if (fn_nargs("restriction", DOC_DICT_RESTRICTION, 0, nargs, e))
        return e->nr;

    dict = (ti_dict_t *) query->rval;
    varr = ti_varr_create(2);

    /* query->collection may be NULL, but this is not an issue as in that case
     * we alwas have spec any */
    key = ti_spec_raw(ti_dict_key_spec(dict), query->collection);
    val = ti_spec_raw(ti_dict_val_spec(dict), query->collection);

    if (!varr || !key || !val)
    {
        ti_val_drop((ti_val_t *) varr);
        ti_val_drop((ti_val_t *) key);
        ti_val_drop((ti_val_t *) val);
        ex_set_mem(e);
        return e->nr;
    }
    VEC_push(varr->vec, key);
    VEC_push(varr->vec, val);

    query->rval = (ti_val_t *) varr;
    ti_val_unsafe_drop((ti_val_t *) dict);
    return e->nr;
}

static inline int do__f_restriction(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    return ti_val_is_object(query->rval)
            ? do__f_restriction_thing(query, nd, e)
            : ti_val_is_list(query->rval)
            ? do__f_restriction_list(query, nd, e)
            : ti_val_is_set(query->rval)
            ? do__f_restriction_set(query, nd, e)
            : ti_val_is_dict(query->rval)
            ? do__f_restriction_dict(query, nd, e)
            : fn_call_try("restriction", query, nd, e);
}
