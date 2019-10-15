#include <ti/fn/fn.h>

static int do__f_del_backup(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    int64_t backup_id;
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_not_node_scope("del_backup", query, e) ||
        fn_nargs("del_backup", DOC_DEL_BACKUP, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `del_backup` expects argument 1 to be of "
            "type `"TI_VAL_INT_S"` but got type `%s` instead"
            DOC_DEL_BACKUP,
            ti_val_str(query->rval));
        return e->nr;
    }


    backup_id = ((ti_vint_t *) query->rval)->int_;

    if (backup_id < 0)
    {
        ex_set(e, EX_VALUE_ERROR,
            "function `del_backup` expects argument 1 to be a "
            "positive integer value"DOC_DEL_BACKUP);
        return e->nr;
    }

    ti_backups_del_backup((uint64_t) backup_id, e);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
