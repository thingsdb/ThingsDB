#include <ti/fn/fn.h>

static int do__f_assert(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * msg;
    cleri_node_t * assert_node;

    if (fn_chained("assert", query, e) ||
        fn_nargs_max("assert", DOC_ASSERT, 1, 2, nargs, e))
        return e->nr;

    assert_node = nd->children->node;

    if (ti_do_statement(query, assert_node, e) || ti_val_as_bool(query->rval))
        return e->nr;

    if (nargs == 1)
    {
        ex_set(e, EX_ASSERT_ERROR,
                "assertion statement `%.*s` has failed",
                (int) assert_node->len, assert_node->str);
        return e->nr;
    }

    ti_val_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `assert` expects argument 2 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead"DOC_ASSERT,
                ti_val_str(query->rval));
        return e->nr;
    }

    msg = (ti_raw_t *) query->rval;

    if (ti_verror_check_msg((const char *) msg->data, msg->n, e))
        return e->nr;

    ex_setn(e, EX_ASSERT_ERROR, (const char *) msg->data, msg->n);
    return e->nr;
}
