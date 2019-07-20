#include <ti/fn/fn.h>

#define HASPROP_DOC_ TI_SEE_DOC("#hasprop")

static int do__f_hasprop(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_raw_t * rname;
    ti_name_t * name;
    ti_thing_t * thing;

    if (!ti_val_is_thing(query->rval))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `hasprop`"HASPROP_DOC_,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `hasprop` takes 1 argument but %d were given"
                HASPROP_DOC_, nargs);
        return e->nr;
    }

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_scope(query, nd->children->node, e))
        goto fail1;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `hasprop` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead"HASPROP_DOC_,
                ti_val_str(query->rval));
        goto fail1;
    }

    rname = (ti_raw_t *) query->rval;
    name = ti_names_weak_get((const char *) rname->data, rname->n);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(
            name && ti_thing_val_weak_get(thing, name));

fail1:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}
