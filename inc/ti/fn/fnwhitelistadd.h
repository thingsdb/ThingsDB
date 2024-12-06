#include <ti/fn/fn.h>

static int do__f_whitelist_add(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    cleri_node_t * child = nd->children;
    ti_user_t * user;
    ti_task_t * task;
    ti_raw_t * raw;
    ti_val_t * val = NULL;
    int wid;

    if (fn_not_thingsdb_scope("whitelist_add", query, e) ||
        fn_nargs_range("whitelist_add", DOC_WHITELIST_ADD, 2, 3, nargs, e) ||
        ti_do_statement(query, child, e) ||
        fn_arg_str_slow("whitelist_add", DOC_WHITELIST_ADD, 1, query->rval, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    user = ti_users_get_by_namestrn((const char *) raw->data, raw->n);
    if (!user)
        return ti_raw_printable_not_found(raw, "user", e);

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    /* read whitelist */
    if (ti_do_statement(query, (child = child->next->next), e) ||
        fn_arg_str_slow("whitelist_add", DOC_WHITELIST_ADD, 2, query->rval, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    wid = ti_whitelist_from_strn((const char *) raw->data, raw->n, e);
    if (e->nr)
        return e->nr;

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (nargs == 3 && ti_do_statement(query, (child = child->next->next), e))
        return e->nr;

    if (ti_whitelist_add(&user->whitelists[wid], query->rval, e))
        return e->nr;

    task = ti_task_get_task(query->change, ti.thing0);
    if (!task || ti_task_whitelist_add(task, user, wid, query->rval))
        ex_set_mem(e);  /* task cleanup is not required */

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
