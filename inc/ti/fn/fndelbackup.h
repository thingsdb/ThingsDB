#include <ti/fn/fn.h>

static int do__f_del_backup(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    int64_t backup_id;
    const int nargs = fn_get_nargs(nd);
    _Bool delete_files = false;

    if (fn_not_node_scope("del_backup", query, e) ||
        ti_access_check_err(ti.access_node, query->user, TI_AUTH_CHANGE, e) ||
        fn_nargs_range("del_backup", DOC_DEL_BACKUP, 1, 2, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_int_slow("del_backup", DOC_DEL_BACKUP, 1, query->rval, e))
        return e->nr;

    backup_id = ((ti_vint_t *) query->rval)->int_;
    if (backup_id < 0)
    {
        ex_set(e, EX_VALUE_ERROR,
            "function `del_backup` expects argument 1 to be a "
            "positive integer value"DOC_DEL_BACKUP);
        return e->nr;
    }

    if (nargs == 2)
    {
        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;

        if (ti_do_statement(query, nd->children->next->next, e) ||
            fn_arg_bool("del_backup", DOC_DEL_BACKUP, 2, query->rval, e))
            return e->nr;

        delete_files = ti_val_as_bool(query->rval);
    }

    ti_backups_del_backup((uint64_t) backup_id, delete_files, e);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
