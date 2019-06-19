#include <ti/cfn/fn.h>

#define HAS_DOC_ TI_SEE_DOC("#has")

static int cq__f_has(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool has;
    ti_vset_t * vset = (ti_vset_t *) ti_query_val_pop(query);

    if (!ti_val_is_set((ti_val_t *) vset))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `has`"HAS_DOC_,
                ti_val_str((ti_val_t *) vset));
        goto done;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `has` takes 1 argument but %d were given"HAS_DOC_,
                nargs);
        goto done;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        goto done;

    if (!ti_val_is_thing(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `has` expects argument 1 to be of "
                "type `"TI_VAL_THING_S"` but got type `%s` instead"HAS_DOC_,
                ti_val_str(query->rval));
        goto done;
    }

    has = ti_vset_has(vset, (ti_thing_t *) query->rval);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(has);

done:
    ti_val_drop((ti_val_t *) vset);
    return e->nr;
}
