/*
 * nodes.c
 */
#include <stdbool.h>
#include <assert.h>
#include <ti/nodes.h>
#include <ti/proto.h>
#include <ti.h>

#define NODES__UV_BACKLOG 64

static ti_nodes_t * nodes;

static void nodes__tcp_connection(uv_stream_t * uvstream, int status);
static void nodes__on_stats(ti_stream_t * stream, ti_pkg_t * pkg);
static void nodes__on_req_query(ti_stream_t * stream, ti_pkg_t * pkg);
static void nodes__on_req_connect(ti_stream_t * stream, ti_pkg_t * pkg);

int ti_nodes_create(void)
{
    nodes = malloc(sizeof(ti_nodes_t));
    if (!nodes)
        return -1;

    /* make sure data is set to null, we use this on close */
    nodes->tcp.data = NULL;

    nodes->vec = vec_new(0);
    memset(&nodes->addr, 0, sizeof(struct sockaddr_storage));
    ti()->nodes = nodes;

    return -(nodes == NULL);
}

void ti_nodes_destroy(void)
{
    if (!nodes)
        return;
    vec_destroy(nodes->vec, (vec_destroy_cb) ti_node_drop);
    free(nodes);
    nodes = ti()->nodes = NULL;
}

size_t ti_nodes_quorum(void)
{
    return (nodes->vec->n + 1) / 2;
}

_Bool ti_nodes_has_quorum(void)
{
    size_t quorum = ti_nodes_quorum();
    size_t q = 0;

    for (vec_each(nodes->vec, ti_node_t, node))
        if (node->status > TI_NODE_STAT_CONNECTING && ++q == quorum)
            return true;

    return false;
}

int ti_nodes_to_packer(qp_packer_t ** packer)
{
    if (qp_add_array(packer))
        return -1;

    for (vec_each(nodes->vec, ti_node_t, node))
    {
        if (qp_add_raw(
                *packer,
                (const unsigned char *) &node->addr,
                sizeof(struct sockaddr_storage)))
            return -1;
    }

    return qp_close_array(*packer);
}

int ti_nodes_from_qpres(qp_res_t * qpnodes)
{
    for (uint32_t i = 0, j = qpnodes->via.array->n; i < j; i++)
    {
        struct sockaddr_storage * addr;
        qp_res_t * qpaddr = qpnodes->via.array->values + i;
        if (    qpaddr->tp != QP_RES_RAW ||
                qpaddr->via.raw->n != sizeof(struct sockaddr_storage))
           return -1;

        addr = (struct sockaddr_storage *) qpaddr->via.raw->data;
        if (!ti_nodes_create_node(addr))
            return -1;
    }
    return 0;
}

ti_node_t * ti_nodes_create_node(struct sockaddr_storage * addr)
{
    ti_node_t * node = ti_node_create(nodes->vec->n, addr);
    if (!node || vec_push(&nodes->vec, node))
    {
        ti_node_drop(node);
        return NULL;
    }
    assert (node->id == nodes->vec->n - 1);
    return node;
}

ti_node_t * ti_nodes_node_by_id(uint8_t node_id)
{
    return node_id >= nodes->vec->n ? NULL : vec_get(nodes->vec, node_id);
}

int ti_nodes_listen(void)
{
    int rc;
    ti_cfg_t * cfg = ti()->cfg;
    _Bool is_ipv6 = false;
    char * ip;

    uv_tcp_init(ti()->loop, &nodes->tcp);

    if (cfg->bind_node_addr != NULL)
    {
        struct in6_addr sa6;
        if (inet_pton(AF_INET6, cfg->bind_node_addr, &sa6))
        {
            is_ipv6 = true;
        }
        ip = cfg->bind_client_addr;
    }
    else if (cfg->ip_support == AF_INET)
    {
        ip = "0.0.0.0";
    }
    else
    {
        ip = "::";
        is_ipv6 = true;
    }

    if (is_ipv6)
    {
        uv_ip6_addr(ip, cfg->node_port, (struct sockaddr_in6 *) &nodes->addr);
    }
    else
    {
        uv_ip4_addr(ip, cfg->node_port, (struct sockaddr_in *) &nodes->addr);
    }

    if ((rc = uv_tcp_bind(
            &nodes->tcp,
            (const struct sockaddr *) &nodes->addr,
            (cfg->ip_support == AF_INET6) ?
                    UV_TCP_IPV6ONLY : 0)) ||
        (rc = uv_listen(
            (uv_stream_t *) &nodes->tcp,
            NODES__UV_BACKLOG,
            nodes__tcp_connection)))
    {
        log_error("error listening for node connections on TCP port %d: `%s`",
                cfg->node_port,
                uv_strerror(rc));
        return -1;
    }

    log_info("start listening for node connections on TCP port %d",
            cfg->node_port);

    ti()->node->status = TI_NODE_STAT_READY;

    return 0;
}

/*
 * Returns another node with status READY.
 */
ti_node_t * ti_nodes_random_ready_node(void)
{
    ti_node_t * this_node = ti()->node;
    ti_node_t * online_nodes[nodes->vec->n-1];
    uint32_t i = 0, n = 0;
    for (vec_each(nodes->vec, ti_node_t, node), i++)
    {
        if (node == this_node || node->status != TI_NODE_STAT_READY)
            continue;
        online_nodes[n++] = node;
    }
    if (!n)
        return NULL;
    return online_nodes[rand() % n];
}

void ti_nodes_pkg_cb(ti_stream_t * stream, ti_pkg_t * pkg)
{
    switch (pkg->tp)
    {
    case TI_PROTO_NODE_STATS:
        nodes__on_stats(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_QUERY:
        nodes__on_req_query(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_CONNECT:
        nodes__on_req_connect(stream, pkg);
        break;
    case TI_PROTO_NODE_RES_QUERY:
    case TI_PROTO_NODE_RES_CONNECT:
    case TI_PROTO_NODE_ERR_MAX_QUOTA:
    case TI_PROTO_NODE_ERR_AUTH:
    case TI_PROTO_NODE_ERR_FORBIDDEN:
    case TI_PROTO_NODE_ERR_INDEX:
    case TI_PROTO_NODE_ERR_BAD_REQUEST:
    case TI_PROTO_NODE_ERR_QUERY:
    case TI_PROTO_NODE_ERR_NODE:
    case TI_PROTO_NODE_ERR_INTERNAL:
        ti_stream_on_response(stream, pkg);
        break;
    default:
        log_error(
                "got an unexpected package type %u from `%s`",
                pkg->tp,
                ti_stream_name(stream));
    }
}

static void nodes__tcp_connection(uv_stream_t * uvstream, int status)
{
    ti_stream_t * stream;

    if (status < 0)
    {
        log_error("node connection error: `%s`", uv_strerror(status));
        return;
    }

    log_debug("received a TCP node connection");

    stream = ti_stream_create(TI_STREAM_TCP_IN_NODE, &ti_nodes_pkg_cb);

    if (!stream)
        return;

    uv_tcp_init(ti()->loop, (uv_tcp_t *) &stream->uvstream);
    if (uv_accept(uvstream, &stream->uvstream) == 0)
    {
        uv_read_start(&stream->uvstream, ti_stream_alloc_buf, ti_stream_on_data);
    }
    else
    {
        ti_stream_drop(stream);
    }
}

static void nodes__on_stats(ti_stream_t * stream, ti_pkg_t * pkg)
{

}

static void nodes__on_req_query(ti_stream_t * stream, ti_pkg_t * pkg)
{

}

static void nodes__on_req_connect(ti_stream_t * stream, ti_pkg_t * pkg)
{

}
