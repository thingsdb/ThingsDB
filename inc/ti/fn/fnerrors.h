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

    if (fn_chained(fn_name, query, e))
        return e->nr;

    if (nargs > 1)
    {
        ex_set(e, EX_BAD_DATA,
                "function `%s` takes at most 1 argument but %d "
                "were given%s", fn_name, nargs, fn_doc);
        return e->nr;
    }

    if (nargs == 0)
    {
        query->rval = (ti_val_t *) ti_verror_from_code(code);
        return e->nr;
    }

    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `%s` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead%s",
            fn_name, ti_val_str(query->rval), fn_doc);
        return e->nr;
    }

    msg = (ti_raw_t *) query->rval;

    if (ti_verror_check_msg((const char *) msg->data, msg->n, e))
        return e->nr;

    verror = ti_verror_from_raw(code, msg);
    if (!verror)
    {
        ex_set_mem(e);
        return e->nr;
    }

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) verror;

    return e->nr;
}

static inline int do__f_overflow_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(
            query,
            nd,
            e,
            EX_OVERFLOW,
            "overflow_err",
            TI_SEE_DOC("#overflow_err"));
}

static inline int do__f_zero_div_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(
            query,
            nd,
            e,
            EX_ZERO_DIV,
            "zero_div_err",
            TI_SEE_DOC("#zero_div_err"));
}

static inline int do__f_max_quota_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_MAX_QUOTA,
            "max_quota_err",
            TI_SEE_DOC("#max_quota_err"));
}

static inline int do__f_auth_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_AUTH_ERROR,
            "auth_err",
            TI_SEE_DOC("#auth_err"));
}

static inline int do__f_forbidden_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_FORBIDDEN,
            "forbidden_err",
            TI_SEE_DOC("#forbidden_err"));
}

static inline int do__f_index_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_INDEX_ERROR,
            "index_err",
            TI_SEE_DOC("#index_err"));
}

static inline int do__f_bad_data_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_BAD_DATA,
            "bad_data_err",
            TI_SEE_DOC("#bad_data_err"));
}

static inline int do__f_syntax_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_SYNTAX_ERROR,
            "syntax_err",
            TI_SEE_DOC("#syntax_err"));
}

static inline int do__f_node_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_NODE_ERROR,
            "node_err",
            TI_SEE_DOC("#node_err"));
}

static inline int do__f_assert_err(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    return do__make_err(query, nd, e,
            EX_ASSERT_ERROR,
            "assert_err",
            TI_SEE_DOC("#assert_err"));
}
