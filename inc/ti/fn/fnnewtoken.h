#include <ti/fn/fn.h>
#include <util/iso8601.h>

#define MAX_USER_TOKENS 128U

static int do__f_new_token(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * uname;
    ti_user_t * user;
    ti_task_t * task;
    cleri_children_t * child = nd->children;
    uint64_t exp_time = 0;
    ti_token_t * token;
    size_t description_sz = 0;
    char * description = NULL;

    if (fn_not_thingsdb_scope("new_token", query, e) ||
        fn_nargs_range("new_token", DOC_NEW_TOKEN, 1, 3, nargs, e) ||
        ti_do_statement(query, child->node, e) ||
        fn_arg_str("new_token", DOC_NEW_TOKEN, 1, query->rval, e))
        return e->nr;

    uname = (ti_raw_t *) query->rval;
    user = ti_users_get_by_namestrn((const char *) uname->data, uname->n);
    if (!user)
        return ti_raw_err_not_found(uname, "user", e);

    if (user != query->user && ti_access_check_err(
            ti.access_thingsdb,
            query->user, TI_AUTH_GRANT, e))
        return e->nr;

    if (user->tokens->n > MAX_USER_TOKENS)
    {
        ex_set(e, EX_MAX_QUOTA,
            "user `%.*s` has reached the maximum of %u tokens"DOC_NEW_TOKEN,
            (int) uname->n, (char *) uname->data,
            MAX_USER_TOKENS);
        return e->nr;
    }

    if (nargs > 1)
    {
        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;

        if (ti_do_statement(query, (child = child->next->next)->node, e))
            return e->nr;

        if (ti_val_is_float(query->rval))
        {
            double now = util_now();
            double ts = VFLOAT(query->rval);
            if (ts < now)
                goto errpast;
            if (ts > TI_MAX_EXPIRATION_DOUDLE)
                goto errfuture;

            exp_time = (uint64_t) ts;
        }
        else if (ti_val_is_int(query->rval))
        {
            int64_t now = (int64_t) util_now_tsec();
            int64_t ts = VINT(query->rval);
            if (ts < now)
                goto errpast;
            if (ts > TI_MAX_EXPIRATION_LONG)
                goto errfuture;

            exp_time = (uint64_t) ts;
        }
        else if (ti_val_is_str(query->rval))
        {
            int64_t now = (int64_t) util_now_tsec();
            ti_raw_t * rt = (ti_raw_t *) query->rval;
            int64_t ts = iso8601_parse_date_n((const char *) rt->data, rt->n);
            if (ts < 0)
                goto errinvalid;
            if (ts < now)
                goto errpast;
            if (ts > TI_MAX_EXPIRATION_LONG)
                goto errfuture;

            exp_time = (uint64_t) ts;
        }
        else if (!ti_val_is_nil(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR,
                "function `new_token` expects argument 2 to be of "
                "type `"TI_VAL_STR_S"`, `"TI_VAL_INT_S"`, `"TI_VAL_FLOAT_S"` "
                "or `"TI_VAL_NIL_S"` but got type `%s` instead"DOC_NEW_TOKEN,
                ti_val_str(query->rval));
            return e->nr;
        }

    }

    if (nargs > 2)
    {
        ti_raw_t * raw;

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;

        if (ti_do_statement(query, (child = child->next->next)->node, e))
            return e->nr;

        if (!ti_val_is_str(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR,
                    "function `new_token` expects argument 3 to be of "
                    "type `"TI_VAL_STR_S"` but got type `%s` instead"
                    DOC_NEW_TOKEN,
                    ti_val_str(query->rval));
            return e->nr;
        }

        raw = (ti_raw_t *) query->rval;

        if (!strx_is_utf8n((const char *) raw->data, raw->n))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "function `new_token` expects a description "
                    "to have valid UTF8 encoding"DOC_NEW_TOKEN);
            return e->nr;
        }

        description = (char *) raw->data;
        description_sz = raw->n;
    }

    token = ti_token_create(
            NULL,
            exp_time,
            util_now_tsec(),
            description,
            description_sz);
    if (!token || ti_user_add_token(user, token))
    {
        ti_token_destroy(token);
        ex_set_mem(e);
        return e->nr;
    }

    task = ti_task_get_task(query->ev, ti.thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_new_token(task, user, token))
        ex_set_mem(e);  /* task cleanup is not required */

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_str_create(
            token->key,
            sizeof(ti_token_key_t));

    if (!query->rval)
        ex_set_mem(e);

    return e->nr;

errpast:
    ex_set(e, EX_VALUE_ERROR,
        "cannot set an expiration time in the past"DOC_NEW_TOKEN);
    return e->nr;

errfuture:
    ex_set(e, EX_VALUE_ERROR,
        "expiration time is too far in the future"DOC_NEW_TOKEN);
    return e->nr;

errinvalid:
    ex_set(e, EX_VALUE_ERROR,
        "invalid date/time string as expiration time"DOC_NEW_TOKEN);
    return e->nr;
}
