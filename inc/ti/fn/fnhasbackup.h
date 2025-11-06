#include <ti/fn/fn.h>

static int do__f_has_backup(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    int64_t backup_id;
    const int nargs = fn_get_nargs(nd);

    if (fn_not_node_scope("has_backup", query, e) ||
        fn_nargs("has_backup", DOC_HAS_BACKUP, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_int_slow("has_backup", DOC_HAS_BACKUP, 1, query->rval, e))
        return e->nr;

    backup_id = VINT(query->rval);

    if (backup_id < 0)
    {
        ex_set(e, EX_VALUE_ERROR,
            "function `has_backup` expects argument 1 to be an "
            "integer value greater than or equal to 0"DOC_HAS_BACKUP);
        return e->nr;
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(
            ti_backups_has_backup((uint64_t) backup_id));

    return e->nr;
}
