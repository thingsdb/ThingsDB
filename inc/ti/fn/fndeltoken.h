#include <ti/fn/fn.h>

#define DEL_TOKEN_DOC_ TI_SEE_DOC("#del_token")

static int do__f_del_token(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (query->syntax.flags & TI_SYNTAX_FLAG_THINGSDB);
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    ti_token_t * token;
    ti_task_t * task;
    ti_raw_t * rkey;

    /* check for privileges */
    if (ti_access_check_err(
            ti()->access_thingsdb,
            query->user, TI_AUTH_GRANT, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `del_token` takes 1 argument but %d were given"
                DEL_TOKEN_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `del_token` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"DEL_TOKEN_DOC_,
            ti_val_str(query->rval));
        return e->nr;
    }

    rkey = (ti_raw_t *) query->rval;

    if (    rkey->n != sizeof(ti_token_key_t) ||
            !strx_is_asciin((const char *) rkey->data, rkey->n))
    {
        ex_set(e, EX_BAD_DATA,
            "function `del_token` expects argument 1 to be token string"
            DEL_TOKEN_DOC_,ti_val_str(query->rval));
        return e->nr;
    }

    token = ti_users_pop_token_by_key((ti_token_key_t *) rkey->data);
    if (!token)
    {
        ex_set(e, EX_INDEX_ERROR, "token `%.*s` not found",
                rkey->n, (char *) rkey->data);
        return e->nr;
    }

    ti_token_destroy(token);

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_del_token(task, (ti_token_key_t *) rkey->data))
        ex_set_alloc(e);  /* task cleanup is not required */

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
