#include <ti/fn/fn.h>

#define ISUTF8_DOC_ TI_SEE_DOC("#isutf8")

static int do__f__isutf8(
        const char * fnname,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_utf8;
    ti_raw_t * raw;

    if (fn_chained(fnname, query, e))
        return e->nr;

    if (nargs != 1)
    {
        ex_set(e, EX_NUM_ARGUMENTS,
                "function `%s` takes 1 argument but %d were given"
                ISUTF8_DOC_, fnname, nargs);
        return e->nr;
    }

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    is_utf8 = ti_val_is_raw(query->rval) &&
            strx_is_utf8n((const char *) raw->data, raw->n);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_utf8);

    return e->nr;
}

static inline int do__f_isutf8(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    return do__f__isutf8("isutf8", query, nd, e);
}


static inline int do__f_isstr(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    return do__f__isutf8("isstr", query, nd, e);
}
