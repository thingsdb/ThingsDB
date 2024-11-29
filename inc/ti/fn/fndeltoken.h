#include <ti/fn/fn.h>

static int do__f_del_token(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_token_t * token;
    ti_task_t * task;
    ti_raw_t * rkey;

    if (fn_not_thingsdb_scope("del_token", query, e) ||
        fn_nargs("del_token", DOC_DEL_TOKEN, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_str("del_token", DOC_DEL_TOKEN, 1, query->rval, e))
        return e->nr;

    rkey = (ti_raw_t *) query->rval;

    if (rkey->n != sizeof(ti_token_key_t) ||
        !strx_is_asciin((const char *) rkey->data, rkey->n))
    {
        ex_set(e, EX_VALUE_ERROR,
            "function `del_token` expects argument 1 to be token string"
            DOC_DEL_TOKEN);
        return e->nr;
    }

    token = ti_access_check(ti.access_thingsdb, query->user, TI_AUTH_GRANT)
            ? ti_users_pop_token_by_key((ti_token_key_t *) rkey->data)
            : ti_user_pop_token_by_key(query->user, (ti_token_key_t *) rkey->data);
    if (!token)
        return ti_raw_printable_not_found(rkey, "token", e);

    ti_token_destroy(token);

    task = ti_task_get_task(query->change, ti.thing0);
    if (!task || ti_task_add_del_token(task, (ti_token_key_t *) rkey->data))
        ex_set_mem(e);  /* task cleanup is not required */

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
