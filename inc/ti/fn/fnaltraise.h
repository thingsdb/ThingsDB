#include <ti/fn/fn.h>

static int do__f_alt_raise(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ex_enum errnr;
    ti_vint_t * vcode;
    ex_t ex = {0};

    if (fn_nargs_range("alt_raise", DOC_ALT_RAISE, 2, 3, nargs, e))
        return e->nr;

    errnr = ti_do_statement(query, nd->children->node, e);

    if (errnr > EX_MAX_BUILD_IN_ERR && errnr <= EX_RETURN)
        return errnr;   /* do not catch success or internal errors */

    /* make sure the return value is dropped */
    ti_val_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next->node, &ex) ||
        fn_arg_int("alt_raise", DOC_ALT_RAISE, 2, query->rval, &ex))
        goto exerr;

    vcode = (ti_vint_t *) query->rval;
    if (vcode->int_ < EX_MIN_ERR || vcode->int_ > EX_MAX_BUILD_IN_ERR)
    {
        ex_set(&ex, EX_VALUE_ERROR,
            "function `alt_raise` expects an error code between %d and %d "
            "but got %"PRId64" instead"DOC_ERR,
            EX_MIN_ERR, EX_MAX_BUILD_IN_ERR, vcode->int_);
        goto exerr;
    }

    errnr = (ex_enum) vcode->int_;
    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (nargs == 3)
    {
        ti_raw_t * msg;
        if (ti_do_statement(
                query,
                nd->children->next->next->next->next->node,
                &ex) ||
            fn_arg_str("alt_raise", DOC_ALT_RAISE, 3, query->rval, &ex))
            goto exerr;

        msg = (ti_raw_t *) query->rval;
        if (ti_verror_check_msg((const char *) msg->data, msg->n, &ex))
            goto exerr;

        ti_raw_set_e(e, msg, errnr);
        return e->nr;
    }

    e->nr = errnr;
    return e->nr;

exerr:
    memcpy(e, &ex, sizeof(ex_t));
    return e->nr;
}
