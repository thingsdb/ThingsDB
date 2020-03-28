#include <ti/fn/fn.h>
#include <util/iso8601.h>

static int do__f_restore(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    uint32_t n;
    ti_task_t * task;
    ti_user_t * user;
    ti_raw_t * rname;
    ti_node_t * node;
    ti_raw_t * tar_gz_str = (ti_raw_t *) ti_val_borrow_tar_gz_str();

    if (fn_not_thingsdb_scope("restore", query, e) ||
        ti_access_check_err(
                    ti()->access_thingsdb,
                    query->user, TI_AUTH_MASK_FULL, e) ||
        fn_nargs("restore", DOC_RESTORE, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_str("restore", DOC_RESTORE, 1, query->rval, e))
        return e->nr;

    rname = (ti_raw_t *) query->rval;

    if (!ti_raw_endswith(rname, tar_gz_str))
    {
        ex_set(e, EX_VALUE_ERROR,
            "expecting a backup file-name to end with `%.*s`"
            DOC_RESTORE, (int) tar_gz_str->n, (char *) tar_gz_str->data);
        return e->nr;
    }

    if ((n = ti()->collections->vec->n))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "restore requires all existing collections to be removed; "
                "there %s still %"PRIu32" collection%s found"DOC_RESTORE,
                n == 1 ? "is" : "are",
                n,
                n == 1 ? "" : "s");
        return e->nr;
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
        return e->nr;
    }

    if (ti_query_is_fwd(query))
    {
        ex_set(e, EX_OPERATION_ERROR,
                "wait for "TI_NODE_ID" to have status `%s` and try again"
                DOC_RESTORE,
                query->via.stream->via.node->id,
                ti_node_status_str(TI_NODE_STAT_READY));
        return e->nr;
    }

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        goto fail0;

    if (ti_task_add_restore(task))
    {
        ex_set_mem(e);
        return e->nr;
    }

    return e->nr;

fail0:
    ti_val_drop((ti_val_t *) rname);
    return e->nr;
}
