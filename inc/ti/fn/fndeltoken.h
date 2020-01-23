#include <ti/fn/fn.h>

static int do__f_del_token(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_token_t * token;
    ti_task_t * task;
    ti_raw_t * rkey;

    if (fn_not_thingsdb_scope("del_token", query, e) ||
        fn_nargs("del_token", DOC_DEL_TOKEN, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `del_token` expects argument 1 to be of "
            "type `"TI_VAL_STR_S"` but got type `%s` instead"DOC_DEL_TOKEN,
            ti_val_str(query->rval));
        return e->nr;
    }

    rkey = (ti_raw_t *) query->rval;

    if (rkey->n != sizeof(ti_token_key_t) ||
        !strx_is_asciin((const char *) rkey->data, rkey->n))
    {
        ex_set(e, EX_VALUE_ERROR,
            "function `del_token` expects argument 1 to be token string"
            DOC_DEL_TOKEN);
        return e->nr;
    }

    token = ti_access_check(ti()->access_thingsdb, query->user, TI_AUTH_GRANT)
            ? ti_users_pop_token_by_key((ti_token_key_t *) rkey->data)
            : ti_user_pop_token_by_key(query->user, (ti_token_key_t *) rkey->data);
    if (!token)
        return ti_raw_err_not_found(rkey, "token", e);

    ti_token_destroy(token);

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_del_token(task, (ti_token_key_t *) rkey->data))
        ex_set_mem(e);  /* task cleanup is not required */

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
