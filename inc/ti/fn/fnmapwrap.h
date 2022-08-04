#include <ti/fn/fn.h>

typedef struct
{
    ti_type_t * type;
    ti_varr_t * varr;
    ex_t * e;
} map_wrap__walk_t;

static int map_wrap__walk_set(ti_thing_t * thing, map_wrap__walk_t * w)
{
    ti_wrap_t * wrap;

    if (w->type)
    {
        wrap = ti_wrap_create(thing, w->type->type_id);
    }
    else if (ti_thing_is_instance(thing))
    {
        wrap = ti_wrap_create(thing, thing->via.type->type_id);
    }
    else
    {
        ex_set(w->e, EX_NUM_ARGUMENTS,
            "function `map_wrap` on a set with a "
            "non-typed `"TI_VAL_THING_S"` "
            "requires 1 argument but 0 were given"DOC_SET_MAP_WRAP);
        return w->e->nr;
    }

    if (!wrap)
    {
        ex_set_mem(w->e);
        return w->e->nr;
    }

    VEC_push(w->varr->vec, wrap);
    return 0;
}

static int do__f_map_wrap(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const char * doc;
    const int nargs = fn_get_nargs(nd);
    size_t n;
    ti_varr_t * varr;
    ti_type_t * type = NULL;
    ti_val_t * iterable;

    doc = doc_map_wrap(query->rval);
    if (!doc)
        return fn_call_try("map_wrap", query, nd, e);

    if (fn_nargs_max("map_wrap", doc, 1, nargs, e))
        return e->nr;

    iterable = query->rval;
    query->rval = NULL;

    n = ti_val_get_len(iterable);
    if (nargs == 1)
    {
        if (ti_do_statement(query, nd->children, e) ||
            fn_arg_str("map_wrap", doc, 1, query->rval, e))
            goto fail0;

        type = query->collection
            ? ti_types_by_raw(
                    query->collection->types,
                    (ti_raw_t *) query->rval)
            : NULL;
        if (!type)
        {
            (void) ti_raw_err_not_found((ti_raw_t *) query->rval, "type", e);
            goto fail0;
        }

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }

    varr = ti_varr_create(n);
    if (!varr)
        goto fail0;

    switch (iterable->tp)
    {
    case TI_VAL_ARR:
        for (vec_each(VARR(iterable), ti_val_t, val))
        {
            ti_wrap_t * wrap;
            ti_thing_t * thing;

            if (!ti_val_is_thing(val))
            {
                ex_set(e, EX_TYPE_ERROR,
                        "function `map_wrap` requires a list with items of "
                        "type `thing` but found an item of type `%s` instead"
                        DOC_LIST_MAP_WRAP,
                        ti_val_str(val));
                goto fail1;
            }

            thing = (ti_thing_t *) val;

            if (type)
            {
                wrap = ti_wrap_create(thing, type->type_id);
            }
            else if (ti_thing_is_instance(thing))
            {
                wrap = ti_wrap_create(thing, thing->via.type->type_id);
            }
            else
            {
                ex_set(e, EX_NUM_ARGUMENTS,
                    "function `map_wrap` on a list with a "
                    "non-typed `"TI_VAL_THING_S"` "
                    "requires 1 argument but 0 were given"DOC_LIST_MAP_WRAP);
                goto fail1;
            }

            if (!wrap)
            {
                ex_set_mem(e);
                goto fail1;
            }
            VEC_push(varr->vec, wrap);
        }
        break;
    case TI_VAL_SET:
    {
        map_wrap__walk_t w = {
                .type=type,
                .varr=varr,
                .e=e,
        };
        if (imap_walk(VSET(iterable), (imap_cb) map_wrap__walk_set, &w))
            goto fail1;
        break;
    }
    }

    ti_val_unsafe_drop(iterable);
    query->rval = (ti_val_t *) varr;
    return e->nr;

fail1:
    ti_val_unsafe_drop((ti_val_t *) varr);
fail0:
    ti_val_unsafe_drop(iterable);
    return e->nr;
}
