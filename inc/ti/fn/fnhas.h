#include <ti/fn/fn.h>

#define HAS_SET_DOC_ TI_SEE_DOC("#has-set")
#define HAS_THING_DOC_ TI_SEE_DOC("#has-thing")

static int do__f_has_set(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    _Bool has;
    ti_vset_t * vset;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `has` takes 1 argument but %d were given"
                HAS_SET_DOC_,
                nargs);
        return e->nr;
    }

    vset = (ti_vset_t *) query->rval;
    query->rval = NULL;

    if (ti_do_scope(query, nd->children->node, e))
        goto fail1;

    if (!ti_val_is_thing(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `has` expects argument 1 to be of "
                "type `"TI_VAL_THING_S"` but got type `%s` instead"
                HAS_SET_DOC_,
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
    ti_raw_t * rname;
    ti_name_t * name;
    ti_thing_t * thing;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `has` takes 1 argument but %d were given"
                HAS_THING_DOC_, nargs);
        return e->nr;
    }

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_scope(query, nd->children->node, e))
        goto fail1;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `has` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead"
                HAS_THING_DOC_,
                ti_val_str(query->rval));
        goto fail1;
    }

    rname = (ti_raw_t *) query->rval;
    name = ti_names_weak_get((const char *) rname->data, rname->n);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(
            name && ti_thing_prop_weak_get(thing, name));

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

    ex_set(e, EX_INDEX_ERROR,
            "type `%s` has no function `has`"HAS_SET_DOC_ HAS_THING_DOC_,
            ti_val_str(query->rval));
    return e->nr;
}
