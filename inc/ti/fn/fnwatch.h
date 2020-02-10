#include <ti/fn/fn.h>

static int do__f_watch(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    int rc;
    const int nargs = langdef_nd_n_function_params(nd);
    ti_thing_t * thing;

    if (!ti_val_is_thing(query->rval))
        return fn_call_try("watch", query, nd, e);

    if (fn_nargs("watch", DOC_THING_WATCH, 0, nargs, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;

    if (!thing->id)
    {
        ex_set(e, EX_VALUE_ERROR,
                "thing has no `#ID`; "
                "if you really want to watch this `thing` then you need to "
                "assign it to a collection"DOC_THING_WATCH);
        return e->nr;
    }

    if (query->qbind.flags & TI_QBIND_FLAG_API)
    {
        log_debug(
            "function `watch()` with a HTTP API request has no effect");
        goto done;  /* Do nothings with HTTP API watch requests */
    }

    rc = ti_stream_is_client(query->via.stream)
        ? ti_thing_watch_init(thing, query->via.stream)
        : ti_thing_watch_fwd(thing, query->via.stream, query->qbind.pkg_id);

    if (rc)
        log_error("watch() has failed (%d)", rc);

done:
    ti_val_drop((ti_val_t *) thing);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
