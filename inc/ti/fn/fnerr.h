#include <ti/fn/fn.h>

static int do__f_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * msg;
    ti_verror_t * verror;
    ti_vint_t * vcode;
    int8_t code;

    if (fn_nargs_max("err", DOC_ERR, 2, nargs, e))
        return e->nr;

    if (nargs == 0)
    {
        query->rval = (ti_val_t *) ti_verror_from_code(-100);
        return e->nr;
    }

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_int("err", DOC_ERR, 1, query->rval, e))
        return e->nr;

    vcode = (ti_vint_t *) query->rval;
    if (vcode->int_ < EX_MIN_ERR || vcode->int_ > EX_MAX_BUILD_IN_ERR)
    {
        ex_set(e, EX_VALUE_ERROR,
            "function `err` expects an error code between %d and %d "
            "but got %"PRId64" instead"DOC_ERR,
            EX_MIN_ERR, EX_MAX_BUILD_IN_ERR, vcode->int_);
        return e->nr;
    }

    code = (int8_t) vcode->int_;
    ti_val_unsafe_drop(query->rval);

    if (nargs == 1)
    {
        query->rval = (ti_val_t *) ti_verror_from_code(code);
        return e->nr;
    }

    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next->node, e) ||
        fn_arg_str("err", DOC_ERR, 2, query->rval, e))
        return e->nr;

    msg = (ti_raw_t *) query->rval;

    if (ti_verror_check_msg((const char *) msg->data, msg->n, e))
        return e->nr;

    verror = ti_verror_from_raw(code, msg);
    if (!verror)
    {
        ex_set_mem(e);
        return e->nr;
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) verror;

    return e->nr;
}
