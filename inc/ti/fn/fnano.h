#include <ti/fn/fn.h>

static int do__f_ano(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_raw_t * spec_raw;

    if (fn_not_collection_scope("ano", query, e) ||
        fn_nargs("ano", DOC_ANO, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_thing("ano", DOC_ANO, 1, query->rval, e))
        return e->nr;

    spec_raw = ti_type_spec_raw_from_thing(
            (ti_thing_t *) query->rval,
            query->rval,
            e);
    if (!spec_raw)
        return e->nr;

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_ano_from_raw(query->collection, spec_raw, e);
    ti_val_unsafe_drop((ti_val_t *) spec_raw);

    return e->nr;
}
