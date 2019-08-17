#include <ti/fn/fn.h>

#define KEYS_DOC_ TI_SEE_DOC("#keys")

static int do__f_keys(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_thing_t * thing;
    ti_varr_t * varr;

    if (!ti_val_is_thing(query->rval))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `keys`"KEYS_DOC_,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `keys` takes 0 arguments but %d %s given"KEYS_DOC_,
                nargs, nargs == 1 ? "was" : "were");
        return e->nr;
    }

    thing = (ti_thing_t *) query->rval;
    varr = ti_varr_create(thing->props->n);
    if (!varr)
    {
        ex_set_mem(e);
        return e->nr;
    }

    for (vec_each(thing->props, ti_prop_t, prop))
    {
        VEC_push(varr->vec, prop->name);
        ti_incref(prop->name);
    }

    query->rval = (ti_val_t *) varr;
    ti_val_drop((ti_val_t *) thing);

    return e->nr;
}
