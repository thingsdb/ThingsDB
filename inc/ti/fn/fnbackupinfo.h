#include <ti/fn/fn.h>

static int do__f_backup_info(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    int64_t backup_id;
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_not_node_scope("backup_info", query, e) ||
        fn_nargs("backup_info", DOC_BACKUP_INFO, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_int("backup_info", DOC_BACKUP_INFO, 1, query->rval, e))
        return e->nr;

    backup_id = VINT(query->rval);

    if (backup_id < 0)
    {
        ex_set(e, EX_VALUE_ERROR,
            "function `backup_info` expects argument 1 to be a "
            "positive integer value"DOC_BACKUP_INFO);
        return e->nr;
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = ti_backups_backup_as_mpval((uint64_t) backup_id, e);

    return e->nr;
}
