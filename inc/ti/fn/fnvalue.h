#include <ti/fn/fn.h>

static int do__f_value(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_val_t * val;

    if (!ti_val_is_enum(query->rval))
        return fn_call_try("value", query, nd, e);

    if (fn_nargs("value", DOC_ENUM_VALUE, 0, nargs, e))
        return e->nr;

    val = VMEMBER(query->rval);
    ti_incref(val);
    ti_val_drop(query->rval);
    query->rval = val;

    return 0;
}
