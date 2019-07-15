#include <ti/fn/fn.h>

#define ERROR_DOC_ TI_SEE_DOC("#error")

static int do__f_error(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * msg;
    ti_verror_t * verror;

    if (nargs > 2)
    {
        ex_set(e, EX_BAD_DATA,
                "function `error` takes at most 2 arguments but %d "
                "were given"ERROR_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `error` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"
            ERROR_DOC_,
            ti_val_str(query->rval));
        return e->nr;
    }

    msg = (ti_raw_t *) query->rval;

    if (ti_verror_check_msg((const char *) msg->data, msg->n, e))
        return e->nr;

    verror = ti_verror_create((const char *) msg->data, msg->n, 1);
    if (!verror)
    {
        ex_set_mem(e);
        return e->nr;
    }

    if (nargs == 2)
    {
        ti_vint_t * code;

        ti_val_drop(query->rval);
        query->rval = NULL;

        if (ti_do_scope(query, nd->children->next->next->node, e))
            goto failed;

        if (!ti_val_is_int(query->rval))
        {
            ex_set(e, EX_BAD_DATA,
                "function `error` expects argument 2 to be of "
                "type `"TI_VAL_INT_S"` but got type `%s` instead"ERROR_DOC_,
                ti_val_str(query->rval));
            goto failed;
        }

        code = (ti_vint_t *) query->rval;

        if (code->int_ < 1 || code->int_ > 32)
        {
            ex_set(e, EX_BAD_DATA,
                "function `error` expects a custom error_code between "
                "1 and 32 but got %"PRId64" instead"ERROR_DOC_, code->int_);
            goto failed;
        }

        verror->code = (uint8_t) code->int_;
    }

    ti_val_drop(query->rval);
    query->rval = verror;

    return e->nr;

failed:
    ti_val_drop((ti_val_t *) verror);
    return e->nr;
}
