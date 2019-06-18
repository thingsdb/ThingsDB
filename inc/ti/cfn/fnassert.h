#include <ti/cfn/fn.h>

#define ASSERT_DOC_ TI_SEE_DOC("#assert")

static int cq__f_assert(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    cleri_children_t * child = nd->children;    /* first in argument list */
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * msg;
    ti_vint_t * code;

    if (nargs < 1)
    {
        ex_set(e, EX_BAD_DATA,
                "function `assert` requires at least 1 argument but 0 "
                "were given"ASSERT_DOC_);
        return e->nr;
    }

    if (nargs > 3)
    {
        ex_set(e, EX_BAD_DATA,
                "function `assert` takes at most 3 arguments but %d "
                "were given"ASSERT_DOC_, nargs);
        return e->nr;
    }

    if (ti_cq_scope(query, child->node, e) || ti_val_as_bool(query->rval))
        return e->nr;

    if (nargs == 1)
    {
        ex_set(e, EX_ASSERT_ERROR,
                "assertion statement `%.*s` has failed",
                (int) child->node->len, child->node->str);
        return e->nr;
    }

    child = child->next->next;
    ti_val_drop(query->rval);
    query->rval = NULL;

    if (ti_cq_scope(query, child->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `assert` expects argument 2 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead"ASSERT_DOC_,
                ti_val_str(query->rval));
        return e->nr;
    }

    msg = (ti_raw_t *) query->rval;
    if (!strx_is_utf8n((const char *) msg->data, msg->n))
    {
        ex_set(e, EX_BAD_DATA,
                "function `assert` expects a message "
                "to have valid UTF8 encoding"ASSERT_DOC_);
        return e->nr;
    }

    ex_setn(e, EX_ASSERT_ERROR, (const char *) msg->data, msg->n);

    if (nargs == 2)
        return e->nr;

    child = child->next->next;
    ti_val_drop(query->rval);
    query->rval = NULL;
    e->nr = 0;

    if (ti_cq_scope(query, child->node, e))
        return e->nr;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `assert` expects argument 3 to be of "
                "type `"TI_VAL_INT_S"` but got type `%s` instead"ASSERT_DOC_,
                ti_val_str(query->rval));
        return e->nr;
    }

    code = (ti_vint_t *) query->rval;

    if (code->int_ < 1 || code->int_ > 32)
    {
        ex_set(e, EX_BAD_DATA,
                "function `assert` expects a custom error_code between "
                "1 and 32 but got %"PRId64" instead"ASSERT_DOC_, code->int_);
        return e->nr;
    }

    e->nr = (ex_enum) code->int_;
    return e->nr;
}
