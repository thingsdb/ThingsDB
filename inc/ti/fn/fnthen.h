#include <ti/fn/fn.h>

static int do__f_then(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_future_t * future;
    size_t n;

    if (!ti_val_is_future(query->rval))
        return fn_call_try("then", query, nd, e);

    future = query->rval;
    query->rval = NULL;

    if (fn_nargs("then", DOC_FUTURE_THEN, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_closure("then", DOC_FUTURE_THEN, 1, query->rval, e))
        goto fail;

    future->then = (ti_closure_t *) query->rval;
    query->rval = ti_nil_get();

fail:
    ti_val_unsafe_drop((ti_val_t *) future);
    return e->nr;
}

/*
THINGSDB_SIRIDB = TS
THINGSDB_MYSQL = SQL

TS_SERVERS = localhost:9000,localhost:9001
TS_DBNAME = dbtest
TS_USERNAME = iris
TS_PASSWORD = siri



*/
