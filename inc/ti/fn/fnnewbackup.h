#include <ti/fn/fn.h>
#include <util/iso8601.h>

static int do__f_new_backup(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    cleri_node_t * child = nd->children;
    ti_raw_t * rname;
    uint64_t backup_id;
    uint64_t timestamp = 0;
    uint64_t repeat = 0;
    uint64_t max_files = 1;
    ti_backup_t * backup;
    ti_raw_t * tar_gz_str = (ti_raw_t *) ti_val_borrow_tar_gz_str();
    ti_raw_t * gs_str = (ti_raw_t *) ti_val_borrow_gs_str();
    queue_t * files_queue;

    if (fn_not_node_scope("new_backup", query, e) ||
        ti_access_check_err(ti.access_node, query->user, TI_AUTH_CHANGE, e) ||
        fn_nargs_range("new_backup", DOC_NEW_BACKUP, 1, 4, nargs, e) ||
        ti_do_statement(query, child, e) ||
        fn_arg_str("new_backup", DOC_NEW_BACKUP, 1, query->rval, e))
        return e->nr;

    rname = (ti_raw_t *) query->rval;
    query->rval = NULL;


    if (!ti_raw_endswith(rname, tar_gz_str))
    {
        ex_set(e, EX_VALUE_ERROR,
            "expecting a full backup file-name to end with `%.*s`"
            DOC_NEW_BACKUP, tar_gz_str->n, (char *) tar_gz_str->data);
        goto fail0;
    }

    if (ti_raw_startswith(rname, gs_str) && !ti.cfg->gcloud_key_file)
    {
        ex_set(e, EX_OPERATION,
            "a key file must be configured to use Google Cloud "
            "storage; set `gcloud_key_file` in the configuration file or set "
            "the environment variable `THINGSDB_GCLOUD_KEY_FILE`"
            DOC_NEW_BACKUP, tar_gz_str->n, (char *) tar_gz_str->data);
        goto fail0;
    }

    if (nargs >= 2)
    {
        if (ti_do_statement(query, (child = child->next->next), e))
            goto fail0;

        max_files = TI_BACKUP_DEFAULT_MAX_FILES;

        if (ti_val_is_datetime(query->rval))
        {
            int64_t ts = DATETIME(query->rval);
            timestamp = ts < 0 ? 0 : (uint64_t ) ts;
        }
        else if (!ti_val_is_nil(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR,
                "function `new_backup` expects argument 2 to be of "
                "type `"TI_VAL_DATETIME_S"`, `"TI_VAL_TIMEVAL_S"` "
                "or `"TI_VAL_NIL_S"` but got type `%s` instead"DOC_NEW_BACKUP,
                ti_val_str(query->rval));
            goto fail0;
        }

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }

    if (nargs >= 3)
    {
        if (ti_do_statement(query, (child = child->next->next), e) ||
            fn_arg_int("new_backup",
                       DOC_NEW_BACKUP, 3, query->rval, e))
            goto fail0;

        repeat = VINT(query->rval) < 0 ? 0 : (uint64_t) VINT(query->rval);

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }

    if (nargs == 4)
    {
        if (ti_do_statement(query, (child = child->next->next), e) ||
            fn_arg_int("new_backup",
                       DOC_NEW_BACKUP, 4, query->rval, e))
            goto fail0;

        max_files = VINT(query->rval) < 1 ? 1 : (uint64_t) VINT(query->rval);

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }

    backup_id = ti.backups->next_id;

    files_queue = queue_new(max_files);
    if (!files_queue)
    {
        ex_set_mem(e);
        goto fail0;
    }

    backup = ti_backups_new_backup(
            backup_id,
            (const char *) rname->data,
            rname->n,
            timestamp,
            repeat,
            max_files,
            util_now_usec(),
            files_queue);

    if (!backup)
    {
        free(files_queue);  /* still empty so we can simply use free */
        ex_set_mem(e);
        goto fail0;
    }

    query->rval = (ti_val_t *) ti_vint_create((int64_t) backup_id);
    if (!query->rval)
        ex_set_mem(e);

fail0:
    ti_val_unsafe_drop((ti_val_t *) rname);
    return e->nr;
}
