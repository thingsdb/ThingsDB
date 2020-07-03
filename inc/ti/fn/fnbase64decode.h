#include <ti/fn/fn.h>

static int do__f_base64_decode(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * src;

    if (fn_nargs("base64_decode", DOC_BASE64_DECODE, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `base64_decode` expects argument 1 to be of "
            "type `"TI_VAL_STR_S"` or type `"TI_VAL_BYTES_S"` "
            "but got type `%s` instead"DOC_BASE64_DECODE,
            ti_val_str(query->rval));
        return e->nr;
    }

    src = (ti_raw_t *) query->rval;

    query->rval = (ti_val_t *) ti_bytes_from_base64(src->data, src->n);
    if (!query->rval)
        ex_set_mem(e);

    ti_val_unsafe_drop((ti_val_t *) src);
    return e->nr;
}
