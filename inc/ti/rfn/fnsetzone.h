#include <ti/rfn/fn.h>

#define SET_ZONE_DOC_ TI_SEE_DOC("#set_zone")

static int rq__f_set_zone(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_node(query, nd, e));
    assert (!query->ev);    /* node queries do never create an event */
    assert (e->nr == 0);
    assert (query->rval == NULL);

    int64_t izone;
    uint8_t zone;

    /* check for privileges */
    if (ti_access_check_err(ti()->access_node,
            query->user, TI_AUTH_MODIFY, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `set_zone` takes 1 argument but %d were given"
                SET_ZONE_DOC_, nargs);
        return e->nr;
    }

    if (rq__scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `set_zone` expects argument 1 to be of "
            "type `"TI_VAL_INT_S"` but got type `%s` instead"SET_ZONE_DOC_,
            ti_val_str(query->rval));
        return e->nr;
    }

    izone = ((ti_vint_t *) query->rval)->int_;

    if (izone < 0 || izone > 0xff)
    {
        ex_set(e, EX_BAD_DATA,
            "`zone` should be an integer between 0 and 255, got %"PRId64
            SET_ZONE_DOC_, izone);
        return e->nr;
    }

    zone = (uint8_t) izone;

    ti_set_and_broadcast_node_zone(zone);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
