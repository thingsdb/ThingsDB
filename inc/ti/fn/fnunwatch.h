#include <ti/fn/fn.h>

static int do__f_unwatch(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    int rc;
    const int nargs = langdef_nd_n_function_params(nd);
    ti_thing_t * thing;

    if (!ti_val_is_thing(query->rval))
        return fn_call_try("unwatch", query, nd, e);

    if (fn_nargs("unwatch", DOC_THING_UNWATCH, 0, nargs, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;

    if (query->qbind.flags & TI_QBIND_FLAG_API)
    {
        log_debug(
            "function `unwatch()` with a HTTP API request has no effect");
        goto done;  /* Do nothings with HTTP API watch requests */
    }

    rc = thing->id == 0
        ? 0
        : ti_stream_is_client(query->via.stream)
        ? ti_thing_unwatch(thing, query->via.stream)
        : ti_thing_unwatch_fwd(thing, query->via.stream, query->pkg_id);

    if (rc)
        log_error("unwatch() has failed (%d)", rc);

done:
    ti_val_unsafe_drop((ti_val_t *) thing);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
