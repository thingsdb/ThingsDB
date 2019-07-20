#include <ti/fn/fn.h>

#define HAS_DOC_ TI_SEE_DOC("#has")

static int do__f_has(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool has;
    ti_vset_t * vset;

    if (!ti_val_is_set(query->rval))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `has`"HAS_DOC_,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `has` takes 1 argument but %d were given"HAS_DOC_,
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
                "type `"TI_VAL_THING_S"` but got type `%s` instead"HAS_DOC_,
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
