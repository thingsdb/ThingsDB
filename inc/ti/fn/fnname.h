#include <ti/fn/fn.h>

static int do__f_name(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_name_t * name;

    if (!ti_val_is_member(query->rval))
        return fn_call_try("name", query, nd, e);

    if (fn_nargs("name", DOC_ENUM_NAME, 0, nargs, e))
        return e->nr;

    name = ((ti_member_t *) query->rval)->name;
    ti_incref(name);
    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) name;

    return 0;
}
