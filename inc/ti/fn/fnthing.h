#include <ti/fn/fn.h>

static int do__f_thing(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    int nargs = fn_get_nargs(nd);

    if (fn_nargs_max("thing", DOC_THING, 1, nargs, e))
        return e->nr;

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

        id = VINT(query->rval);
        thing = ti_query_thing_from_id(query, id, e);
        if (!thing)
            return e->nr;

        ti_val_unsafe_drop(query->rval);
        query->rval = (ti_val_t *) thing;

        return e->nr;
    }

    query->rval = (ti_val_t *) ti_thing_o_create(0, 0, query->collection);

    if (!query->rval)
        ex_set_mem(e);

    return e->nr;


}
