/*
 * node.c
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ti.h>
#include <ti/lookup.h>
#include <ti/node.h>
#include <ti/nodes.h>
#include <ti/proto.h>
#include <ti/req.h>
#include <ti/tcp.h>
#include <ti/version.h>
#include <ti/write.h>
#include <util/logger.h>
#include <util/qpx.h>

static void node__on_connect(uv_connect_t * req, int status);
static void node__on_connect_req(ti_req_t * req, ex_enum status);

/*
 * Nodes are created ti_nodes_create_node() to ensure a correct id
 * is generated for each node.
 */
ti_node_t * ti_node_create(uint8_t id, struct sockaddr_storage * addr)
{
    ti_node_t * node = (ti_node_t *) malloc(sizeof(ti_node_t));
    if (!node)
        return NULL;

    node->ref = 1;
    node->flags = 0;
    node->cevid = 0;
    node->next_thing_id = 0;
    node->id = id;
    node->stream = NULL;
    node->status = TI_NODE_STAT_OFFLINE;
    node->addr = *addr;
    node->next_retry = 0;
    node->retry_counter = 0;

    return node;
}

void ti_node_drop(ti_node_t * node)
{
    if (node && !--node->ref)
    {
        ti_stream_drop(node->stream);
        free(node);
    }
}

const char * ti_node_name(ti_node_t * node)
{
    return ti()->node == node ? ti_name() : ti_stream_name(node->stream);
}

const char * ti_node_status_str(ti_node_status_t status)
{
    switch (status)
    {
    case TI_NODE_STAT_OFFLINE:          return "OFFLINE";
    case TI_NODE_STAT_SHUTTING_DOWN:    return "SHUTTING_DOWN";
    case TI_NODE_STAT_CONNECTING:       return "CONNECTING";
    case TI_NODE_STAT_SYNCHRONIZING:    return "SYNCHRONIZING";
    case TI_NODE_STAT_AWAY:             return "AWAY";
    case TI_NODE_STAT_AWAY_SOON:        return "AWAY_SOON";
    case TI_NODE_STAT_READY:            return "READY";
    }
    return "UNKNOWN";
}

int ti_node_connect(ti_node_t * node)
{
    assert (!node->stream);
    assert (node->status == TI_NODE_STAT_OFFLINE);

    uv_connect_t * req;

    node->stream = ti_stream_create(TI_STREAM_TCP_OUT_NODE, ti_nodes_pkg_cb);
    if (!node->stream)
        goto fail0;

    req = malloc(sizeof(uv_connect_t));
    if (!req)
        goto fail0;

    if (uv_tcp_init(ti()->loop, (uv_tcp_t *) node->stream))
        goto fail0;

    if (uv_tcp_connect(
            req,
            (uv_tcp_t *) node->stream,
            (const struct sockaddr*) &node->addr,
            node__on_connect))
    {
        ti_stream_close(node->stream);
        return -1;
    }

    node->status = TI_NODE_STAT_CONNECTING;

    return 0;

fail0:
    ti_stream_drop(node->stream);
    return -1;
}

ti_node_t * ti_node_winner(ti_node_t * node_a, ti_node_t * node_b, uint64_t u)
{
    ti_node_t * min = node_a->id < node_b->id ? node_a : node_b;
    ti_node_t * max = node_a->id > node_b->id ? node_a : node_b;

    return ti_lookup_node_is_ordered(min->id, max->id, u) ? min : max;
}

static void node__on_connect(uv_connect_t * req, int status)
{
    int rc;
    qpx_packer_t * packer;
    ti_pkg_t * pkg;
    ti_node_t * node = req->data, * ti_node = ti()->node;

    if (status)
    {
        log_error("connecting to node id %u has failed (%s)",
                node->id,
                uv_strerror(status));
        goto failed;
    }

    log_debug("node connection to `%s` created", ti_node_name(node));

    rc = uv_read_start(req->handle, ti_stream_alloc_buf, ti_stream_on_data);
    if (rc)
    {
        log_error("cannot read on socket: `%s`", uv_strerror(rc));
        goto failed;
    }

    packer = qpx_packer_create(1024, 1);
    if (!packer)
    {
        log_error(EX_ALLOC_S);
        goto failed;
    }

    (void) qp_add_array(&packer);
    (void) qp_add_int64(packer, ti_node->id);
    (void) qp_add_int64(packer, ti_node->status);
    (void) qp_add_raw_from_str(packer, TI_VERSION);
    (void) qp_add_raw_from_str(packer, TI_MINIMAL_VERSION);
    (void) qp_close_array(packer);

    pkg = qpx_packer_pkg(packer, TI_PROTO_NODE_REQ_CONNECT);

    if (ti_req_create(
            node->stream,
            pkg,
            TI_PROTO_NODE_REQ_CONNECT_TIMEOUT,
            node__on_connect_req,
            node))
    {
        log_error(EX_INTERNAL_S);
        goto failed;
    }

    goto done;

failed:
    ti_stream_close(node->stream);
done:
    free(req);
}


static void node__on_connect_req(ti_req_t * req, ex_enum status)
{
    qp_unpacker_t unpacker;
    ti_pkg_t * pkg = req->pkg_res;
    ti_node_t * node = req->data;
    qp_obj_t qp_node_id, qp_status;

    if (status)
        goto failed;  /* logging is done */

    qp_unpacker_init2(&unpacker, pkg->data, pkg->n, 0);

    if (    !qp_is_array(qp_next(&unpacker, NULL)) ||
            !qp_is_int(qp_next(&unpacker, &qp_node_id)) ||
            !qp_is_int(qp_next(&unpacker, &qp_status)))
    {
        log_error("invalid response from node id %u (package type: `%s`)",
                node->id, ti_proto_str(pkg->tp));
        goto failed;
    }

    node->status = qp_status.via.int64;

    /* reset the connection retry counters */
    node->next_retry = 0;
    node->retry_counter = 0;

    goto done;

failed:
    ti_stream_close(node->stream);
done:
    free(pkg);
    ti_req_destroy(req);
}
