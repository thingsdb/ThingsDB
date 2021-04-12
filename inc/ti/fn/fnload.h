#include <ti/fn/fn.h>

static int do__f_load(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    mp_unp_t up;
    const int nargs = fn_get_nargs(nd);
    ti_raw_t * mpdata;
    ti_val_t * val;
    ti_vup_t vup = {
            .isclient = true,
            .collection = query->collection,
            .up = &up,
    };

    if (!ti_val_is_mpdata(query->rval))
        return fn_call_try("load", query, nd, e);

    if (fn_nargs("load", DOC_MPDATA_LOAD, 0, nargs, e))
        return e->nr;

    mpdata = (ti_raw_t *) query->rval;
    mp_unp_init(&up, mpdata->data, mpdata->n);

    val = ti_val_from_vup_e(&vup, e);
    ti_val_unsafe_drop(query->rval);
    query->rval = val;

    return e->nr;
}
