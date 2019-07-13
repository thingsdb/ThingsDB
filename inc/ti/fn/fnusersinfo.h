#include <ti/fn/fn.h>

#define USERS_INFO_DOC_ TI_SEE_DOC("#users_info")

static int do__f_users_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (query->syntax.flags & TI_SYNTAX_FLAG_THINGSDB);
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    /* check for privileges */
    if (ti_access_check_err(
            ti()->access_thingsdb,
            query->user, TI_AUTH_GRANT, e))
        return e->nr;

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `users_info` takes 0 arguments but %d %s given"
                USERS_INFO_DOC_,
                nargs, nargs == 1 ? "was" : "were");
        return e->nr;
    }

    query->rval = ti_users_info_as_qpval();
    if (!query->rval)
        ex_set_alloc(e);

    return e->nr;
}
