#include <ti/fn/fn.h>

#define ISNAN_DOC_ TI_SEE_DOC("#isnan")

static int do__f_isnan(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    _Bool is_nan;

    if (fn_chained("isnan", query, e))
        return e->nr;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_NUM_ARGUMENTS,
                "function `isnan` takes 1 argument but %d were given"
                ISNAN_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_statement(query, nd->children->node, e))
        return e->nr;

    switch (query->rval->tp)
    {
    case TI_VAL_BOOL:
    case TI_VAL_INT:
        is_nan = false;
        break;
    case TI_VAL_FLOAT:
        is_nan = isnan(((ti_vfloat_t *) query->rval)->float_);
        break;
    default:
        is_nan = true;
    }

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_nan);

    return e->nr;
}
