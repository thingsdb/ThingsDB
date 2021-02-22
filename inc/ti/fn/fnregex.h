#include <ti/fn/fn.h>

static int do__f_regex(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_raw_t * pattern, * flags;

    if (fn_nargs_range("regex", DOC_REGEX, 1, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("regex", DOC_REGEX, 1, query->rval, e))
        return e->nr;

    pattern = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (nargs == 2)
    {
        if (ti_do_statement(query, nd->children->next->next->node, e) ||
            fn_arg_str("regex", DOC_REGEX, 2, query->rval, e))
            goto fail0;
        flags = (ti_raw_t *) query->rval;
        query->rval = NULL;

    }
    else
        flags = (ti_raw_t *) ti_val_empty_str();

    query->rval = (ti_val_t *) ti_regex_create(pattern, flags, e);

    ti_val_unsafe_drop((ti_val_t * ) flags);
fail0:
    ti_val_unsafe_drop((ti_val_t * ) pattern);
    return e->nr;
}
