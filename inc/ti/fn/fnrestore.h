#include <ti/fn/fn.h>
#include <util/iso8601.h>

static int do__f_restore(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    char * restore_task;
    _Bool overwrite_access = false;
    uint32_t n;
    uint64_t ccid, scid;
    ti_task_t * task;
    ti_raw_t * rname;
    ti_node_t * node;

    if (fn_not_thingsdb_scope("restore", query, e) ||
        ti_access_check_err(
                    ti.access_thingsdb,
                    query->user, TI_AUTH_MASK_FULL, e) ||
        fn_nargs_range("restore", DOC_RESTORE, 1, 2, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_str_slow("restore", DOC_RESTORE, 1, query->rval, e))
        return e->nr;

    rname = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (nargs == 2)
    {
        if (ti_do_statement(query, nd->children->next->next, e) ||
            fn_arg_bool("restore", DOC_RESTORE, 2, query->rval, e))
            goto fail0;

        overwrite_access = ti_val_as_bool(query->rval);
        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }

    if ((node = ti_nodes_not_ready()))
    {
        ex_set(e, EX_OPERATION,
                TI_NODE_ID" is having status `%s`, "
                "wait for the status to become `%s` and try again"
                DOC_RESTORE,
                node->id,
                ti_node_status_str(node->status),
                ti_node_status_str(TI_NODE_STAT_READY));
        goto fail0;
    }

    if (ti_query_is_fwd(query))
    {
        ex_set(e, EX_OPERATION,
                "wait for "TI_NODE_ID" to have status `%s` and try again"
                DOC_RESTORE,
                query->via.stream->via.node->id,
                ti_node_status_str(TI_NODE_STAT_READY));
        goto fail0;
    }

    if (ti_restore_chk((const char *) rname->data, rname->n, e))
        goto fail0;

    restore_task = ti_restore_task((const char *) rname->data, rname->n);
    if (!restore_task)
    {
        ex_set_mem(e);
        goto fail1;
    }

    /* check for tasks in the @thingsdb scope, bug #249 */
    if ((n = ti.tasks->vtasks->n))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "restore requires all existing tasks to be removed; "
                "there %s still %"PRIu32" task%s found"DOC_RESTORE,
                n == 1 ? "is" : "are",
                n,
                n == 1 ? "" : "s");
        goto fail1;
    }

    if ((n = ti.modules->n))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "restore requires all existing modules to be removed; "
                "there %s still %"PRIu32" module%s found"DOC_RESTORE,
                n == 1 ? "is" : "are",
                n,
                n == 1 ? "" : "s");
        goto fail1;
    }

    if ((n = ti.futures_count))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "restore requires all existing futures to be finished; "
                "there %s still %"PRIu32" running future%s found"DOC_RESTORE,
                n == 1 ? "is" : "are",
                n,
                n == 1 ? "" : "s");
        goto fail1;
    }

    if ((n = ti.collections->vec->n))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "restore requires all existing collections to be removed; "
                "there %s still %"PRIu32" collection%s found"DOC_RESTORE,
                n == 1 ? "is" : "are",
                n,
                n == 1 ? "" : "s");
        goto fail1;
    }

    if ((n = ti.changes->queue->n - 1))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "restore requires an empty change queue; "
                "there %s still %"PRIu32" change%s pending"DOC_RESTORE,
                n == 1 ? "is" : "are",
                n,
                n == 1 ? "" : "s");
        goto fail1;
    }

    if (ti.changes->next_change_id - 1 > query->change->id)
    {
        ex_set(e, EX_OPERATION,
                "restore is expecting to have the last change id but it seems "
                "another change is claiming the last change id"DOC_RESTORE);
        goto fail1;
    }

    ccid = ti_nodes_ccid();
    scid = ti_nodes_scid();

    if (ccid != scid && ti.nodes->vec->n > 1)
    {
        ex_set(e, EX_OPERATION,
                "restore requires the committed "TI_CHANGE_ID
                "to be equal to the stored "TI_CHANGE_ID""DOC_RESTORE,
                ccid, scid);
        goto fail1;
    }

    if (ccid != ti.node->ccid)
    {
        ex_set(e, EX_OPERATION,
                "restore requires the global committed "TI_CHANGE_ID
                "to be equal to the local committed "TI_CHANGE_ID""DOC_RESTORE,
                ccid, ti.node->ccid);
        goto fail1;
    }

    if (scid != ti.node->scid)
    {
        ex_set(e, EX_OPERATION,
                "restore requires the global stored "TI_CHANGE_ID
                "to be equal to the local stored "TI_CHANGE_ID""DOC_RESTORE,
                scid, ti.node->scid);
        goto fail1;
    }

    if (ti_restore_is_busy())
    {
        ex_set(e, EX_OPERATION,
                "another restore is busy; wait until the pending restore is "
                "finished and try again"DOC_RESTORE);
        goto fail1;
    }

    if (fx_is_dir(ti.store->store_path))
    {
        log_warning("removing store directory: `%s`", ti.store->store_path);
        if (fx_rmdir(ti.store->store_path))
        {
            ex_set(e, EX_OPERATION,
                        "failed to remove path: `%s`",
                        ti.store->store_path);
            goto fail1;
        }
    }

    if (ti_archive_rmdir())
    {
        ex_set(e, EX_OPERATION,
                "failed to remove archives, check node log for more details");
        goto fail1;
    }

    /*
     * Unpacking is "reasonable" tested by `ti_restore_chk(..)` so at this
     * point, unpacking the tar file is not expected to fail unless there is
     * not enough disk space or other serious error.
     */
    if (ti_restore_unp(restore_task, e))
        goto fail1;

    if (ti_restore_master(overwrite_access ? query->user : NULL))
    {
        ex_set_internal(e);
        goto fail1;
    }

    task = ti_task_get_task(query->change, ti.thing0);
    if (!task || ti_task_add_restore(task))
    {
        ex_set_mem(e);
        goto fail1;
    }

    query->rval = (ti_val_t *) ti_nil_get();

fail1:
    free(restore_task);
fail0:
    ti_val_unsafe_drop((ti_val_t *) rname);
    return e->nr;
}
