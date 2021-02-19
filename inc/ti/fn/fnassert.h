#include <ti/fn/fn.h>

static int do__f_assert(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_raw_t * msg;
    cleri_node_t * assert_node;

    if (fn_nargs_range("assert", DOC_ASSERT, 1, 2, nargs, e))
        return e->nr;

    assert_node = nd->children->node;

    if (ti_do_statement(query, assert_node, e))
        return e->nr;

    if (ti_val_as_bool(query->rval))
    {
        ti_val_unsafe_drop(query->rval);
        query->rval = (ti_val_t *) ti_nil_get();
        return e->nr;  /* success */
    }

    if (nargs == 1)
    {
        ex_set(e, EX_ASSERT_ERROR,
                "assertion statement `%.*s` has failed",
                (int) assert_node->len, assert_node->str);
        return e->nr;
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next->node, e) ||
        fn_arg_str("assert", DOC_ASSERT, 2, query->rval, e))
        return e->nr;

    msg = (ti_raw_t *) query->rval;

    if (ti_verror_check_msg((const char *) msg->data, msg->n, e))
        return e->nr;

    ex_setn(e, EX_ASSERT_ERROR, (const char *) msg->data, msg->n);
    return e->nr;
}
