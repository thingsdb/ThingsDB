#include <ti/fn/fn.h>

static int id_list__walk_set(ti_thing_t * thing, ti_varr_t * varr)
{
    ti_val_t * val = thing->id
            ? (ti_val_t *) ti_vint_create((int64_t) thing->id)
            : (ti_val_t *) ti_nil_get();
    if (!val)
        return -1;

    VEC_push(varr->vec, val);
    return 0;
}

static int do__f_id_list(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const char * doc;
    const int nargs = fn_get_nargs(nd);
    size_t n;
    ti_varr_t * varr;

    doc = doc_id_list(query->rval);
    if (!doc)
        return fn_call_try("id_list", query, nd, e);

    if (fn_nargs("id_list", doc, 0, nargs, e))
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
            ti_thing_t * thing;
            if (!ti_val_is_thing(val))
            {
                ex_set(e, EX_TYPE_ERROR,
                        "function `id_list` requires a list with items of "
                        "type `thing` only but got type `%s` instead"
                        DOC_LIST_ID_LIST,
                        ti_val_str(val));
                goto fail0;
            }

            thing = (ti_thing_t *) val;
            val = thing->id
                    ? (ti_val_t *) ti_vint_create((int64_t) thing->id)
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
        if (imap_walk(VSET(query->rval), (imap_cb) id_list__walk_set, varr))
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
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}
