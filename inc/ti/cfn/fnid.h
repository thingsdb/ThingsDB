#include <ti/cfn/fn.h>

static int cq__f_id(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_thing_t * thing = (ti_thing_t *) ti_query_val_pop(query);

    if (thing->tp != TI_VAL_THING)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `id`",
                ti_val_str((ti_val_t *) thing));
        goto done;
    }

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `id` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        goto done;
    }

    query->rval = (ti_val_t *) ti_vint_create((int64_t) thing->id);
    if (!query->rval)
        ex_set_alloc(e);

done:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}
