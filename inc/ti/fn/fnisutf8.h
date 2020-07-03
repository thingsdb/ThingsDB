#include <ti/fn/fn.h>

static int do__f_isutf8(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_utf8;
    ti_raw_t * raw;

    if (fn_nargs("isutf8", DOC_ISUTF8, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    is_utf8 = ti_val_is_str(query->rval) &&
            strx_is_utf8n((const char *) raw->data, raw->n);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_utf8);

    return e->nr;
}

