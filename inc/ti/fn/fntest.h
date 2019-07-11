#include <ti/fn/fn.h>

#define TEST_DOC_ TI_SEE_DOC("#test")

static int do__f_test(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = ti_query_val_pop(query);
    _Bool has_match;

    if (!ti_val_is_raw(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `test`"TEST_DOC_,
                ti_val_str(val));
        goto done;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `test` takes 1 argument but %d were given"TEST_DOC_,
                nargs);
        goto done;
    }

    if (ti_do_scope(query, nd->children->node, e))
        goto done;

    if (!ti_val_is_regex(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `test` expects argument 1 to be "
            "of type `"TI_VAL_REGEX_S"` but got type `%s` instead"TEST_DOC_,
            ti_val_str(query->rval));
        goto done;
    }

    has_match = ti_regex_test((ti_regex_t *) query->rval, (ti_raw_t *) val);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(has_match);

done:
    ti_val_drop(val);
    return e->nr;
}
