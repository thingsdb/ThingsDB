#include <ti/fn/fn.h>

#define THING_DOC_ TI_SEE_DOC("#thing")

static int do__f_thing(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    int nargs = langdef_nd_n_function_params(nd);

    if (fn_chained("thing", query, e))
        return e->nr;

    if (nargs > 1)
    {
        ex_set(e, EX_NUM_ARGUMENTS,
                "function `thing` takes at most 1 argument but %d were given"
                THING_DOC_, nargs);
        return e->nr;
    }

    if (nargs == 1)
    {
        int64_t id;
        ti_thing_t * thing;

        if (ti_do_statement(query, nd->children->node, e) ||
            ti_val_is_thing(query->rval))
            return e->nr;

        if (!ti_val_is_int(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR,
                    "cannot convert type `%s` to `"TI_VAL_THING_S"`",
                    ti_val_str(query->rval));
            return e->nr;
        }

        id = ((ti_vint_t *) query->rval)->int_;
        thing = ti_query_thing_from_id(query, id, e);
        if (!thing)
            return e->nr;

        ti_val_drop(query->rval);
        query->rval = (ti_val_t *) thing;

        return e->nr;
    }

    if (query->collection && ti_quota_things(
            query->collection->quota,
            query->collection->things->n,
            e))
        return e->nr;

    query->rval = (ti_val_t *) ti_thing_create(
            0,
            query->collection ? query->collection->things : NULL);

    if (!query->rval)
        ex_set_mem(e);

    return e->nr;


}
