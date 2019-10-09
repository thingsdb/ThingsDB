/*
 * ti/sync.c
 */
#include <assert.h>
#include <ti/sync.h>
#include <ti/proto.h>
#include <ti.h>
#include <ti/nodes.h>
#include <util/mpack.h>

enum
{
    SYNC__STAT_INIT,
    SYNC__STAT_STARTED,
    SYNC__STAT_STOPPED,
};

static ti_sync_t * sync_;
static ti_sync_t sync__;

static void sync__find_away_node_cb(uv_timer_t * UNUSED(repeat));
static void sync__destroy(uv_handle_t * UNUSED(handle));
static _Bool sync__set_node(ti_node_t * node);
static void sync__on_res_sync(ti_req_t * req, ex_enum status);
static void sync__finish(void);

int ti_sync_create(void)
{
    sync_ = &sync__;

    sync_->repeat = malloc(sizeof(uv_timer_t));
    sync_->status = SYNC__STAT_INIT;
    sync_->node = NULL;

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

    sync_->status = SYNC__STAT_STARTED;

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
    if (!sync_ || sync_->status == SYNC__STAT_STOPPED)
        return;

    if (sync_->status == SYNC__STAT_INIT)
    {
        sync__destroy(NULL);
    }
    else
    {
        sync_->status = SYNC__STAT_STOPPED;

        (void) uv_timer_stop(sync_->repeat);
        uv_close((uv_handle_t *) sync_->repeat, sync__destroy);
    }
}

static void sync__destroy(uv_handle_t * UNUSED(handle))
{
    if (sync_)
    {
        ti_node_drop(sync_->node);
        free(sync_->repeat);
    }
    sync_ = ti()->sync = NULL;
}

static void sync__find_away_node_cb(uv_timer_t * UNUSED(repeat))
{
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_node_t * node;
    ti_pkg_t * pkg;

    node = ti_nodes_get_away_or_soon();
    if (!sync__set_node(node) && node)
        return;

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

    if (mp_sbuffer_alloc_init(&buffer, 32, sizeof(ti_pkg_t)))
    {
        log_critical(EX_MEMORY_S);
        return;
    }

    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_uint64(&pk, ti()->node->cevid + 1);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_NODE_REQ_SYNC, buffer.size);

    if (ti_req_create(
            node->stream,
            pkg,
            TI_PROTO_NODE_REQ_SYNC_TIMEOUT,
            sync__on_res_sync,
            NULL))
    {
        log_critical(EX_MEMORY_S);
        free(pkg);
        return;
    }
}

static _Bool sync__set_node(ti_node_t * node)
{
    if (node == sync_->node)
        return false;
    ti_node_drop(sync_->node);
    sync_->node = ti_grab(node);
    return true;
}

static void sync__on_res_sync(ti_req_t * req, ex_enum status)
{
    if (status == 0 && req->pkg_res->tp != TI_PROTO_NODE_RES_SYNC)
    {
        ti_pkg_log(req->pkg_res);
    }

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
