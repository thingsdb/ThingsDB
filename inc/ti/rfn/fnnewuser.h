#include <ti/rfn/fn.h>

#define NEW_USER_DOC_ TI_SEE_DOC("#new_user")

static int rq__f_new_user(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (!rq__is_not_thingsdb(query, nd, e));
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    char * passstr = NULL;
    int nargs;
    ti_user_t * nuser;
    ti_raw_t * rname;
    ti_task_t * task;

    nargs = langdef_nd_n_function_params(nd);
    if (nargs != 2)
    {
        ex_set(e, EX_BAD_DATA,
            "function `new_user` requires 2 arguments but %d %s given"
            NEW_USER_DOC_, nargs, nargs == 1 ? "was" : "were");
        return e->nr;
    }

    if (rq__scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `new_user` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"NEW_USER_DOC_,
            ti_val_str(query->rval));
        return e->nr;
    }

    rname = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (rq__scope(query, nd->children->next->next->node, e))
        goto done;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `new_user` expects argument 2 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"NEW_USER_DOC_,
            ti_val_str(query->rval));
        goto done;
    }

    passstr = ti_raw_to_str((ti_raw_t *) query->rval);
    if (!passstr)
    {
        ex_set_alloc(e);
        goto done;
    }

    nuser = ti_users_new_user(
            (const char *) rname->data, rname->n,
            passstr, e);

    if (!nuser)
        goto done;

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        goto done;

    if (ti_task_add_new_user(task, nuser))
        ex_set_alloc(e);  /* task cleanup is not required */

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vint_create((int64_t) nuser->id);
    if (!query->rval)
        ex_set_alloc(e);

done:
    free(passstr);
    ti_val_drop((ti_val_t *) rname);

    return e->nr;
}
