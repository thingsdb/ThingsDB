#include <ti/fn/fn.h>

static int do__f_type_assert(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_val_t * type_str;
    cleri_children_t * child = nd->children;    /* first in argument list */

    if (fn_nargs_range("type_assert", DOC_TYPE_ASSERT, 2, 3, nargs, e) ||
        ti_do_statement(query, child->node, e))
        return e->nr;

    type_str = ti_val_strv(query->rval);
    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, (child = child->next->next)->node, e))
        goto done;

    if (ti_val_is_str(query->rval))
    {
        if (ti_raw_eq((ti_raw_t *) type_str, (ti_raw_t *) query->rval))
            goto success;
        goto invalid;
    }

    if (ti_val_is_array(query->rval))
    {
        vec_t * vec = VARR(query->rval);
        for (vec_each(vec, ti_val_t, val))
        {
            if (!ti_val_is_str(val))
            {
                ex_set(e, EX_TYPE_ERROR,
                        "function `type_assert` expects argument 2 to be a "
                        TI_VAL_LIST_S " or "TI_VAL_TUPLE_S" of "
                        "type `"TI_VAL_STR_S"` but got type `%s` instead"
                        DOC_TYPE_ASSERT,
                        ti_val_str(val));
                goto done;
            }

            if (ti_raw_eq((ti_raw_t *) type_str, (ti_raw_t *) val))
                goto success;
        }
        goto invalid;
    }

    ex_set(e, EX_TYPE_ERROR,
            "function `type_assert` expects argument 2 to be of "
            "type `"TI_VAL_STR_S"`, type `"TI_VAL_LIST_S"` or "
            "type `"TI_VAL_TUPLE_S"` but got type `%s` instead"DOC_TYPE_ASSERT,
            ti_val_str(query->rval));

    goto done;

invalid:
    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (nargs == 3)
    {
        ti_raw_t * msg;

        if (ti_do_statement(query, child->next->next->node, e) ||
            fn_arg_str("type_assert", DOC_STR_CONTAINS, 3, query->rval, e))
            goto done;

        msg = (ti_raw_t *) query->rval;

        if (ti_verror_check_msg((const char *) msg->data, msg->n, e))
            return e->nr;

        ex_setn(e, EX_TYPE_ERROR, (const char *) msg->data, msg->n);
        goto done;
    }

    ex_set(e, EX_TYPE_ERROR,
            "invalid type `%.*s`",
            (int) ((ti_raw_t *) type_str)->n,
            (const char *) ((ti_raw_t *) type_str)->data);
    goto done;

success:
    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

done:
    ti_val_unsafe_drop(type_str);
    return e->nr;
}
