#include <ti/fn/fn.h>

static int do__f_move(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_datetime_t * dt;
    datetime_unit_e unit;
    int64_t num;

    if (!ti_val_is_datetime(query->rval))
        return fn_call_try("move", query, nd, e);

    if (fn_nargs("move", DOC_DATETIME_MOVE, 2, nargs, e))
        return e->nr;

    if (query->rval->ref > 1)
    {
        dt = ti_datetime_copy((ti_datetime_t *) query->rval);
        if (!dt)
        {
            ex_set_mem(e);
            return e->nr;
        }
        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }
    else
    {
        dt = (ti_datetime_t *) query->rval;
        query->rval = NULL;
    }

    if (ti_do_statement(query, nd->children, e) ||
        fn_arg_str("move", DOC_DATETIME_MOVE, 1, query->rval, e))
        goto fail;

    unit = ti_datetime_get_unit((ti_raw_t *) query->rval, e);
    if (e->nr)
        goto fail;

    ti_val_unsafe_drop(query->rval);  /* this destroys `raw_unit` */
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next, e) ||
        fn_arg_int("move", DOC_DATETIME_MOVE, 2, query->rval, e))
        goto fail;

    num = VINT(query->rval);
    ti_val_unsafe_drop(query->rval);  /* this destroys `integer value` */

    (void) ti_datetime_move(dt, unit, num, e);
    query->rval = (ti_val_t *) dt;

    return e->nr;

fail:
    ti_val_unsafe_drop((ti_val_t *) dt);
    return e->nr;
}
