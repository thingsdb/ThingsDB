#include <ti/fn/fn.h>

static int map_id__walk_set(ti_thing_t * thing, ti_varr_t * varr)
{
    ti_val_t * val = thing->id
            ? (ti_val_t *) ti_vint_create((int64_t) thing->id)
            : (ti_val_t *) ti_nil_get();
    if (!val)
        return -1;

    VEC_push(varr->vec, val);
    return 0;
}

static int do__f_map_id(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const char * doc;
    const int nargs = fn_get_nargs(nd);
    size_t n;
    ti_varr_t * varr;

    doc = doc_map_id(query->rval);
    if (!doc)
        return fn_call_try("map_id", query, nd, e);

    if (fn_nargs("map_id", doc, 0, nargs, e))
        return e->nr;

    n = ti_val_get_len(query->rval);
    varr = ti_varr_create(n);
    if (!varr)
        return e->nr;

    switch (query->rval->tp)
    {
    case TI_VAL_ARR:
        for (vec_each(VARR(query->rval), ti_val_t, val))
        {
            uint64_t id;
            if (ti_val_is_thing(val))
            {
                ti_thing_t * thing = (ti_thing_t *) val;
                id = thing->id;
            }
            else if (ti_val_is_wrap(val))
            {
                ti_wrap_t * wtype = (ti_wrap_t *) val;
                id = wtype->thing->id;
            }
            else if (ti_val_is_wano(val))
            {
                ti_wano_t * wtype = (ti_wano_t *) val;
                id = wtype->thing->id;
            }
            else if (ti_val_is_room(val))
            {
                ti_room_t * room = (ti_room_t *) val;
                id = room->id;
            }
            else if (ti_val_is_task(val))
            {
                ti_vtask_t * vtask = (ti_vtask_t *) val;
                id = vtask->id;
            }
            else
            {
                ex_set(e, EX_LOOKUP_ERROR,
                        "type `%s` has no function `id`",
                        ti_val_str(val));
                goto fail0;
            }
            val = id
                    ? (ti_val_t *) ti_vint_create((int64_t) id)
                    : (ti_val_t *) ti_nil_get();
            if (!val)
            {
                ex_set_mem(e);
                goto fail0;
            }
            VEC_push(varr->vec, val);
        }
        break;
    case TI_VAL_SET:
        if (imap_walk(VSET(query->rval), (imap_cb) map_id__walk_set, varr))
        {
            ex_set_mem(e);
            goto fail0;
        }
        break;
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) varr;
    return e->nr;

fail0:
    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;
}
