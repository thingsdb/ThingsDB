#include <ti/fn/fn.h>
#include <util/iso8601.h>

static int do__f_new_backup(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    cleri_children_t * child = nd->children;
    ti_raw_t * rname;
    uint64_t backup_id;
    uint64_t timestamp = 0;
    uint64_t repeat = 0;
    ti_backup_t * backup;
    ti_raw_t * tar_gz_str = (ti_raw_t *) ti_val_borrow_tar_gz_str();

    if (fn_not_node_scope("new_backup", query, e) ||
        ti_access_check_err(ti()->access_node,
            query->user, TI_AUTH_MODIFY, e) ||
        fn_nargs_range("new_backup", DOC_NEW_BACKUP, 1, 3, nargs, e) ||
        ti_do_statement(query, child->node, e))
        return e->nr;

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `new_backup` expects argument 1 to be of "
            "type `"TI_VAL_STR_S"` but got type `%s` instead"
            DOC_NEW_BACKUP, ti_val_str(query->rval));
        return e->nr;
    }

    if (ti()->nodes->vec->n == 1)
    {
        ex_set(e, EX_OPERATION_ERROR,
            "at least 2 nodes are required to make a backup"
            DOC_NEW_BACKUP);
        return e->nr;
    }

    rname = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (!ti_raw_endswith(rname, tar_gz_str))
    {
        ex_set(e, EX_VALUE_ERROR,
            "expecting a backup file-name to end with `%.*s`"
            DOC_NEW_BACKUP, (int) tar_gz_str->n, (char *) tar_gz_str->data);
        goto fail0;
    }

    if (nargs >= 2)
    {
        if (ti_do_statement(query, (child = child->next->next)->node, e))
            goto fail0;

        if (ti_val_is_float(query->rval))
        {
            double ts = VFLOAT(query->rval);
            if (ts < 0.0)
                ts = 0.0;
            timestamp = (uint64_t) ts;
        }
        else if (ti_val_is_int(query->rval))
        {
            int64_t ts = VINT(query->rval);
            if (ts < 0)
                ts = 0;
            timestamp = (uint64_t) ts;
        }
        else if (ti_val_is_str(query->rval))
        {
            ti_raw_t * rt = (ti_raw_t *) query->rval;
            int64_t ts = iso8601_parse_date_n((const char *) rt->data, rt->n);
            if (ts < 0)
            {
                ex_set(e, EX_VALUE_ERROR,
                        "invalid date/time string as backup start time"
                        DOC_NEW_BACKUP);
                goto fail0;
            }
            timestamp = (uint64_t) ts;
        }
        else if (!ti_val_is_nil(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR,
                "function `new_backup` expects argument 2 to be of "
                "type `"TI_VAL_STR_S"`, `"TI_VAL_INT_S"`, `"TI_VAL_FLOAT_S"` "
                "or `"TI_VAL_NIL_S"` but got type `%s` instead"DOC_NEW_BACKUP,
                ti_val_str(query->rval));
            goto fail0;
        }

        ti_val_drop(query->rval);
        query->rval = NULL;
    }

    if (nargs == 3)
    {
        int64_t repeat_ts;
        if (ti_do_statement(query, (child = child->next->next)->node, e))
            goto fail0;

        if (!ti_val_is_int(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR,
                "function `new_backup` expects argument 3 to be of "
                "type `"TI_VAL_INT_S"` but got type `%s` instead"
                DOC_NEW_BACKUP, ti_val_str(query->rval));
            return e->nr;
        }

        repeat_ts = VINT(query->rval);
        if (repeat_ts < 0)
            repeat_ts = 0;

        repeat = (uint64_t) repeat_ts;

        ti_val_drop(query->rval);
        query->rval = NULL;
    }

    backup_id = ti()->backups->next_id;

    backup = ti_backups_new_backup(
            backup_id,
            (const char *) rname->data,
            rname->n,
            timestamp,
            repeat,
            util_now_tsec());

    if (!backup)
    {
        ex_set_mem(e);
        goto fail0;
    }

    query->rval = (ti_val_t *) ti_vint_create((int64_t) backup_id);
    if (!query->rval)
        ex_set_mem(e);

fail0:
    ti_val_drop((ti_val_t *) rname);
    return e->nr;
}
