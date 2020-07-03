#include <ti/fn/fn.h>

static int do__make_err(
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e,
        int8_t code,
        const char * fn_name,
        const char * fn_doc)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * msg;
    ti_verror_t * verror;

    if (fn_nargs_max(fn_name, fn_doc, 1, nargs, e))
        return e->nr;

    if (nargs == 0)
    {
        query->rval = (ti_val_t *) ti_verror_from_code(code);
        return e->nr;
    }

    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str(fn_name, fn_doc, 1, query->rval, e))
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

static inline int do__f_operation_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_OPERATION_ERROR,
            "operation_err",
            DOC_OPERATION_ERR);
}

static inline int do__f_num_arguments_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_NUM_ARGUMENTS,
            "num_arguments_err",
            DOC_NUM_ARGUMENTS_ERR);
}

static inline int do__f_type_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_TYPE_ERROR,
            "type_err",
            DOC_TYPE_ERR);
}

static inline int do__f_value_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_VALUE_ERROR,
            "value_err",
            DOC_VALUE_ERR);
}

static inline int do__f_overflow_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_OVERFLOW,
            "overflow_err",
            DOC_OVERFLOW_ERR);
}

static inline int do__f_zero_div_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_ZERO_DIV,
            "zero_div_err",
            DOC_ZERO_DIV_ERR);
}

static inline int do__f_max_quota_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_MAX_QUOTA,
            "max_quota_err",
            DOC_MAX_QUOTA_ERR);
}

static inline int do__f_auth_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_AUTH_ERROR,
            "auth_err",
            DOC_AUTH_ERR);
}

static inline int do__f_forbidden_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_FORBIDDEN,
            "forbidden_err",
            DOC_FORBIDDEN_ERR);
}

static inline int do__f_lookup_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_LOOKUP_ERROR,
            "lookup_err",
            DOC_LOOKUP_ERR);
}

static inline int do__f_bad_data_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_BAD_DATA,
            "bad_data_err",
            DOC_BAD_DATA_ERR);
}

static inline int do__f_syntax_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_SYNTAX_ERROR,
            "syntax_err",
            DOC_SYNTAX_ERR);
}

static inline int do__f_node_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_NODE_ERROR,
            "node_err",
            DOC_NODE_ERR);
}

static inline int do__f_assert_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_ASSERT_ERROR,
            "assert_err",
            DOC_ASSERT_ERR);
}
