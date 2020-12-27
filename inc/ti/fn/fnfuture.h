#include <ti/fn/fn.h>

static int do__f_future(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    cleri_children_t * child = nd->children;
    ti_future_t * future;
    ti_ext_t * ext;

    if (fn_nargs_min("future", DOC_FUTURE, 1, nargs, e))
        return e->nr;

    if (ti_do_statement(query, child->node, e))
        return e->nr;

    if (ti_val_is_nil(query->rval))
    {
        ext = ti_ext_async();
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    future = ti_future_create(query, ext, nargs-1);
    if (!future)
    {
        ex_set_mem(e);
        return e->nr;
    }

    while ((child = child->next) && (child = child->next))
    {
        if (ti_do_statement(query, child->node, e))
            goto fail;

        VEC_push(future->args, query->rval);
        query->rval = NULL;
    }

    if (ti_future_register(future))
    {
        ex_set_mem(e);
        goto fail;
    }

    query->rval = future;
    return e->nr;

fail:
    ti_val_unsafe_drop(future);
    return e->nr;
}

/*
 TODO: implement configuration like below:

THINGSDB_EXT_SIRIDB = TS
THINGSDB_EXT_MYSQL = SQL


TS_SERVERS = localhost:9000,localhost:9001
TS_DBNAME = dbtest
TS_USERNAME = iris
TS_PASSWORD = siri



*/
