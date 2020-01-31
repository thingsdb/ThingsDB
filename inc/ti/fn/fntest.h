#include <ti/fn/fn.h>

static int do__f_test(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * raw;
    _Bool has_match;

    if (!ti_val_is_str(query->rval))
        return fn_call_try("test", query, nd, e);

    if (fn_nargs("test", DOC_STR_TEST, 1, nargs, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e))
        goto failed;

    if (!ti_val_is_regex(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `test` expects argument 1 to be "
            "of type `"TI_VAL_REGEX_S"` but got type `%s` instead"DOC_STR_TEST,
            ti_val_str(query->rval));
        goto failed;
    }

    has_match = ti_regex_test((ti_regex_t *) query->rval, raw);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(has_match);

failed:
    ti_val_drop((ti_val_t *) raw);
    return e->nr;
}
