#include <ti/fn/fn.h>

static int do__f_type_all(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    imap_t * imap;
    ti_type_t * type;
    ti_vset_t * vset;

    if (fn_not_collection_scope("type_all", query, e) ||
        fn_nargs("type_all", DOC_TYPE_ALL, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_str("type_all", DOC_TYPE_ALL, 1, query->rval, e))
        return e->nr;

    type = ti_types_by_raw(query->collection->types, (ti_raw_t *) query->rval);
    if (!type)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "type", e);

    imap = ti_type_collect_things(query, type);
    if (!imap)
    {
        ex_set_mem(e);
        return e->nr;
    }

    vset = ti_vset_create_imap(imap);
    if (!vset)
    {
        ex_set_mem(e);
        imap_destroy(imap, (imap_destroy_cb) ti_val_unsafe_drop);
        return e->nr;
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) vset;
    return e->nr;
}
