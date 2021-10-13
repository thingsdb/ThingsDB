#include <ti/fn/fn.h>

static int do__f_log(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    int rc;
    ti_raw_t * data;

    if (fn_nargs("log", DOC_LOG, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        ti_val_convert_to_str(&query->rval, e))
        return e->nr;

    data = (ti_raw_t *) query->rval;

    log_warning("Log (%.*s): %.*s",
            query->user->name->n,
            query->user->name->data,
            data->n,
            data->data);

    rc = (query->flags & TI_QUERY_FLAG_API)
            ? 0
            : ti_stream_is_client(query->via.stream)
            ? ti_warn_log_client(query->via.stream, data->data, data->n)
            : ti_warn_log_fwd(
                        query->via.stream,
                        query->pkg_id,
                        data->data,
                        data->n);

    if (rc)
        log_error("failed to write log");

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();
    return e->nr;
}
