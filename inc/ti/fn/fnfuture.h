#include <ti/fn/fn.h>

static int do__f_future(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    cleri_children_t * child = nd->children;

    if (fn_nargs_min("future", DOC_FUTURE, 1, nargs, e))
        return e->nr;

    if (ti_do_statement(query, child->node, e))
        return e->nr;


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
