#include <ti/fn/fn.h>
#include <util/iso8601.h>

static int do__f_restore(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    char * job;
    _Bool overwrite_access = false;
    uint32_t n;
    uint64_t cevid, sevid;
    ti_task_t * task;
    ti_raw_t * rname;
    ti_node_t * node;

    if (fn_not_thingsdb_scope("restore", query, e) ||
        ti_access_check_err(
                    ti.access_thingsdb,
                    query->user, TI_AUTH_MASK_FULL, e) ||
        fn_nargs_range("restore", DOC_RESTORE, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str_slow("restore", DOC_RESTORE, 1, query->rval, e))
        return e->nr;

    rname = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (nargs == 2)
    {
        if (ti_do_statement(query, nd->children->next->next->node, e) ||
            fn_arg_bool("restore", DOC_RESTORE, 2, query->rval, e))
            goto fail0;

        overwrite_access = ti_val_as_bool(query->rval);
        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }

    if ((node = ti_nodes_not_ready()))
    {
        ex_set(e, EX_OPERATION_ERROR,
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
        ex_set(e, EX_OPERATION_ERROR,
                "wait for "TI_NODE_ID" to have status `%s` and try again"
                DOC_RESTORE,
                query->via.stream->via.node->id,
                ti_node_status_str(TI_NODE_STAT_READY));
        goto fail0;
    }

    if (ti_restore_chk((const char *) rname->data, rname->n, e))
        goto fail0;

    job = ti_restore_job((const char *) rname->data, rname->n);
    if (!job)
    {
        ex_set_mem(e);
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

    if ((n = ti.events->queue->n - 1))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "restore requires an empty event queue; "
                "there %s still %"PRIu32" events%s pending"DOC_RESTORE,
                n == 1 ? "is" : "are",
                n,
                n == 1 ? "" : "s");
        goto fail1;
    }

    if (ti.events->next_event_id - 1 > query->ev->id)
    {
        ex_set(e, EX_OPERATION_ERROR,
                "restore is expecting to have the last event id but it seems "
                "another event is claiming the last event id"DOC_RESTORE);
        goto fail1;
    }

    cevid = ti_nodes_cevid();
    sevid = ti_nodes_sevid();

    if (cevid != sevid && ti.nodes->vec->n > 1)
    {
        ex_set(e, EX_OPERATION_ERROR,
                "restore requires the committed "TI_EVENT_ID
                "to be equal to the stored "TI_EVENT_ID""DOC_RESTORE,
                cevid, sevid);
        goto fail1;
    }

    if (cevid != ti.node->cevid)
    {
        ex_set(e, EX_OPERATION_ERROR,
                "restore requires the global committed "TI_EVENT_ID
                "to be equal to the local committed "TI_EVENT_ID""DOC_RESTORE,
                cevid, ti.node->cevid);
        goto fail1;
    }

    if (sevid != ti.node->sevid)
    {
        ex_set(e, EX_OPERATION_ERROR,
                "restore requires the global stored "TI_EVENT_ID
                "to be equal to the local stored "TI_EVENT_ID""DOC_RESTORE,
                sevid, ti.node->sevid);
        goto fail1;
    }

    if (fx_is_dir(ti.store->store_path))
    {
        log_warning("removing store directory: `%s`", ti.store->store_path);
        if (fx_rmdir(ti.store->store_path))
        {
            ex_set(e, EX_OPERATION_ERROR,
                        "failed to remove path: `%s`",
                        ti.store->store_path);
            goto fail1;
        }
    }

    if (ti_archive_rmdir())
    {
        ex_set(e, EX_OPERATION_ERROR,
                "failed to remove archives, check node log for more details");
        goto fail1;
    }

    if (ti_restore_is_busy())
    {
        ex_set(e, EX_OPERATION_ERROR,
                "another restore is busy; wait until the pending restore is "
                "finished and try again"DOC_RESTORE);
        goto fail1;
    }

    if (ti_restore_unp(job, e))
        goto fail1;

    if (ti_restore_master(overwrite_access ? query->user : NULL))
    {
        ex_set_internal(e);
        goto fail1;
    }

    task = ti_task_get_task(query->ev, ti.thing0, e);
    if (!task)
        goto fail1;

    if (ti_task_add_restore(task, overwrite_access ? query->user : NULL))
    {
        ex_set_mem(e);
        goto fail1;
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

fail1:
    free(job);
fail0:
    ti_val_unsafe_drop((ti_val_t *) rname);
    return e->nr;
}
