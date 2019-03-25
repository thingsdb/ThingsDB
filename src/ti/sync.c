/*
 * ti/sync.c
 */
#include <assert.h>
#include <ti/sync.h>
#include <ti/proto.h>
#include <ti.h>
#include <ti/nodes.h>
#include <util/qpx.h>

enum
{
    SYNC__STAT_INIT,
    SYNC__STAT_FIND_AWAY_NODE,
};

static ti_sync_t * sync_;

static void sync__find_away_node_cb(uv_timer_t * UNUSED(repeat));
static void sync__destroy(uv_handle_t * UNUSED(handle));
static void sync__on_res_sync(ti_req_t * req, ex_enum status);
static void sync__finish(void);

int ti_sync_create(void)
{
    sync_ = malloc(sizeof(ti_nodes_t));
    if (!sync_)
        return -1;

    sync_->repeat = malloc(sizeof(uv_timer_t));
    sync_->status = SYNC__STAT_INIT;

    if (!sync_->repeat)
    {
        sync__destroy(NULL);
        return -1;
    }

    ti()->sync = sync_;
    return 0;
}

int ti_sync_start(void)
{
    assert (sync_->status == SYNC__STAT_INIT);

    if (uv_timer_init(ti()->loop, sync_->repeat))
        goto fail0;

    sync_->status = SYNC__STAT_FIND_AWAY_NODE;

    if (uv_timer_start(sync_->repeat,
            sync__find_away_node_cb,
            500,
            1000))
        goto fail1;

    return 0;

fail1:
    sync_->status = SYNC__STAT_INIT;
    uv_close((uv_handle_t *) sync_->repeat, sync__destroy);
fail0:
    return -1;
}

void ti_sync_stop(void)
{
    if (!sync_)
        return;

    if (sync_->status == SYNC__STAT_INIT)
    {
        sync__destroy(NULL);
    }
    else
    {
        (void) uv_timer_stop(sync_->repeat);
        uv_close((uv_handle_t *) sync_->repeat, sync__destroy);
    }
}

static void sync__destroy(uv_handle_t * UNUSED(handle))
{
    if (sync_)
    {
        free(sync_->repeat);
        free(sync_);
    }
    sync_ = ti()->sync = NULL;
}

static void sync__find_away_node_cb(uv_timer_t * UNUSED(repeat))
{
    assert (sync_->status == SYNC__STAT_FIND_AWAY_NODE);
    ti_node_t * node;
    qp_packer_t * packer;
    ti_event_t * event;
    ti_pkg_t * pkg;

    event = queue_first(ti()->events->queue);
    if (event && event->id == ti()->node->cevid + 1)
    {
        log_info("no events are missing, skip synchronization");
        sync__finish();
        return;
    }

    node = ti_nodes_get_away_or_soon();
    if (node == NULL)
    {
        if (ti_nodes_ignore_sync())
        {
            log_warning(
                    "ignore the synchronize step because no node is available "
                    "to synchronize with, and the quorum of nodes is in "
                    "synchronization mode, and this node has the highest "
                    "known committed event id");
            sync__finish();
        }
        else
        {
            log_info("waiting for a node to enter `away` mode");
        }
        return;
    }

    packer = qpx_packer_create(128, 1);
    if (!packer)
    {
        log_critical(EX_ALLOC_S);
        return;
    }

    (void) qp_add_array(&packer);
    (void) qp_add_int(packer, ti()->node->cevid + 1);
    (void) qp_add_int(packer, event ? event->id : 0);
    (void) qp_close_array(packer);

    pkg = qpx_packer_pkg(packer, TI_PROTO_NODE_REQ_SYNC);
    if (ti_req_create(
            node->stream,
            pkg,
            TI_PROTO_NODE_REQ_SYNC_TIMEOUT,
            sync__on_res_sync,
            NULL))
    {
        log_critical(EX_ALLOC_S);
        free(pkg);
        return;
    }

    uv_timer_stop(sync_->repeat);
}

static void sync__on_res_sync(ti_req_t * req, ex_enum status)
{
    if (status)
        goto failed;

    if (req->pkg_res->tp != TI_PROTO_NODE_RES_SYNC)
    {
        ti_pkg_log(req->pkg_res);
        goto failed;
    }

    if (uv_timer_start(sync_->repeat,
            sync__find_away_node_cb,
            5000,
            10000))
    {
        log_error("failed to start `sync__find_away_node_cb` timer");
    }

    goto done;

failed:
    /* make sure the timer is not running */
    (void) uv_timer_stop(sync_->repeat);

    if (uv_timer_start(sync_->repeat,
            sync__find_away_node_cb,
            500,
            1000))
    {
        log_critical("failed to start `sync__find_away_node_cb` timer");
        ti_term(SIGTERM);
    }

done:
    free(req->pkg_req);
    ti_req_destroy(req);
}


static void sync__finish(void)
{
    ti_sync_stop();
    if (ti()->node->status == TI_NODE_STAT_SYNCHRONIZING)
    {
        ti_set_and_broadcast_node_status(TI_NODE_STAT_READY);
    }
}
