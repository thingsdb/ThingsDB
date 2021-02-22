#include <ti/fn/fn.h>

/*
 * TODO: This is an experimental function. Maybe we want to change the export
 *       and include data as an optional. This could be achieved by moving the
 *       export function to the @node scope, and schedule an export just like
 *       a backup. The export(..) function should then export all scopes,
 *       or defined by a configuration, should accept a Google Storage or file
 *       path, and should have an import(..) function to read an exported file.
 */
static int do__f_export(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    if (fn_not_collection_scope("export", query, e) ||
        fn_nargs("export", "; Warning: `export(..)` is experimental", 0, nargs, e))
        return e->nr;

    log_warning("function `export(..)` is experimental");

    query->rval = (ti_val_t *) ti_export_collection(query->collection);
    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
