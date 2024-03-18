#include <ti/fn/fn.h>

static int do__f_is_email(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    _Bool is_email;

    if (fn_nargs("is_email", DOC_IS_EMAIL, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e))
        return e->nr;

    is_email = ti_val_is_str(query->rval) && ti_regex_test(
            (ti_regex_t *) ti_val_borrow_re_email(),
            (ti_raw_t *) query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_email);

    return 0;
}
