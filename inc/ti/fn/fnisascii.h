#include <ti/fn/fn.h>

static int do__f_isascii(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool is_ascii;
    ti_raw_t * raw;

    if (fn_nargs("isascii", DOC_ISASCII, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    is_ascii = ti_val_is_str(query->rval) &&
            strx_is_asciin((const char *) raw->data, raw->n);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_ascii);

    return e->nr;
}
