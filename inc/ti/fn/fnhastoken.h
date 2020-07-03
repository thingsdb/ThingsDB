#include <ti/fn/fn.h>

static int do__f_has_token(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool has_token;
    ti_raw_t * rkey;

    if (fn_not_thingsdb_scope("has_token", query, e) ||
        ti_access_check_err(
            ti.access_thingsdb,
            query->user, TI_AUTH_GRANT, e) ||
        fn_nargs("has_token", DOC_HAS_TOKEN, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("has_token", DOC_HAS_TOKEN, 1, query->rval, e))
        return e->nr;

    rkey = (ti_raw_t *) query->rval;

    if (rkey->n != sizeof(ti_token_key_t) ||
        !strx_is_asciin((const char *) rkey->data, rkey->n))
    {
        ex_set(e, EX_VALUE_ERROR,
            "function `has_token` expects argument 1 to be token string"
            DOC_HAS_TOKEN);
        return e->nr;
    }

    has_token = ti_users_has_token((ti_token_key_t *) rkey->data);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(has_token);

    return e->nr;
}
