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
static void nodes__on_req_connect(ti_stream_t * stream, ti_pkg_t * pkg);
static void nodes__on_req_query(ti_stream_t * stream, ti_pkg_t * pkg);
static void nodes__on_status(ti_stream_t * stream, ti_pkg_t * pkg);

int ti_nodes_create(void)
{
    nodes = malloc(sizeof(ti_nodes_t));
    if (!nodes)
        return -1;

    /* make sure data is set to null, we use this on close */
    nodes->tcp.data = NULL;
    nodes->cevid = 0;
    nodes->sevid = 0;

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

uint8_t ti_nodes_quorum(void)
{
    return nodes->vec->n / 2;
}

_Bool ti_nodes_has_quorum(void)
{
    size_t quorum = ti_nodes_quorum();
    size_t q = 0;

    for (vec_each(nodes->vec, ti_node_t, node), ++q)
        if (node->status > TI_NODE_STAT_CONNECTING && q == quorum)
            return true;

    return false;
}

/* increases with a new reference as long as required */
void ti_nodes_write_rpkg(ti_rpkg_t * rpkg)
{
    for (vec_each(ti()->nodes->vec, ti_node_t, node))
    {
        if (node != ti()->node && !ti_stream_is_closed(node->stream))
        {
            if (ti_stream_write_rpkg(node->stream, rpkg))
                log_error(EX_INTERNAL_S);
        }
    }
}

int ti_nodes_to_packer(qp_packer_t ** packer)
{
    if (qp_add_array(packer))
        return -1;

    for (vec_each(nodes->vec, ti_node_t, node))
    {
        if (qp_add_array(packer) ||
            qp_add_raw(
                *packer,
                (const uchar *) &node->addr,
                sizeof(struct sockaddr_storage)) ||
            qp_add_int64(*packer, node->flags) ||
            qp_close_array(*packer))
            return -1;
    }

    return qp_close_array(*packer);
}

int ti_nodes_from_qpres(qp_res_t * qpnodes)
{
    for (uint32_t i = 0, j = qpnodes->via.array->n; i < j; i++)
    {
        ti_node_t * node;
        struct sockaddr_storage * addr;
        qp_res_t * qpflags, * qpaddr;
        qp_res_t * qparray = qpnodes->via.array->values + i;

        if (qparray->tp != QP_RES_ARRAY || qparray->via.array->n != 2)
            return -1;

        qpaddr = qparray->via.array->values + 0;
        qpflags = qparray->via.array->values + 1;

        if (    qpaddr->tp != QP_RES_RAW ||
                qpaddr->via.raw->n != sizeof(struct sockaddr_storage) ||
                qpflags->tp != QP_RES_INT64)
            return -1;

        addr = (struct sockaddr_storage *) qpaddr->via.raw->data;

        node = ti_nodes_create_node(addr);
        if (!node)
            return -1;

        node->flags = (uint8_t) qpflags->via.int64;
    }
    return 0;
}

uint64_t ti_nodes_cevid(void)
{
    uint64_t m = *ti()->events->cevid;

    for (vec_each(ti()->nodes->vec, ti_node_t, node))
        if (node->cevid < m)
            m = node->cevid;

    if (m > nodes->cevid)
        nodes->cevid = m;

    return nodes->cevid;
}

uint64_t ti_nodes_sevid(void)
{
    uint64_t m = *ti()->archive->sevid;

    for (vec_each(ti()->nodes->vec, ti_node_t, node))
        if (node->sevid < m)
            m = node->sevid;

    if (m > nodes->sevid)
        nodes->sevid = m;

    return nodes->sevid;
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

    return 0;
}

/*
 * Returns a borrowed node in away mode or NULL if none is found
 */
ti_node_t * ti_nodes_get_away(void)
{
    for (vec_each(nodes->vec, ti_node_t, node))
        if (node->status == TI_NODE_STAT_AWAY)
            return node;
    return NULL;
}

/*
 * Returns a borrowed node in away or soon mode or NULL if none is found
 */
ti_node_t * ti_nodes_get_away_or_soon(void)
{
    for (vec_each(nodes->vec, ti_node_t, node))
        if (node->status == TI_NODE_STAT_AWAY ||
            node->status == TI_NODE_STAT_AWAY_SOON)
            return node;
    return NULL;
}

/*
 * Returns another borrowed node with status READY.
 */
ti_node_t * ti_nodes_random_ready_node(void)
{
    ti_node_t * this_node = ti()->node;
    ti_node_t * online_nodes[nodes->vec->n-1];
    uint32_t n = 0;
    for (vec_each(nodes->vec, ti_node_t, node))
    {
        if (node == this_node || node->status != TI_NODE_STAT_READY)
            continue;
        online_nodes[n++] = node;
    }
    if (!n)
        return NULL;
    return online_nodes[rand() % n];
}

/*
 * Returns another borrowed node which manages id with status READY.
 */
ti_node_t * ti_nodes_random_ready_node_for_id(uint64_t id)
{
    ti_node_t * this_node = ti()->node;
    ti_node_t * online_nodes[nodes->vec->n-1];
    uint32_t n = 0;
    for (vec_each(nodes->vec, ti_node_t, node))
    {
        if (    node == this_node ||
                node->status != TI_NODE_STAT_READY ||
                !ti_node_manages_id(node, ti()->lookup, id))
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
    case TI_PROTO_NODE_STATUS:
        nodes__on_status(stream, pkg);
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

int ti_nodes_info_to_packer(qp_packer_t ** packer)
{
    ti_node_t * this_node = ti()->node;
    if (qp_add_array(packer))
        return -1;

    for (vec_each(nodes->vec, ti_node_t, node))
    {
        if (qp_add_map(packer) ||
            qp_add_raw_from_str(*packer, "id") ||
            qp_add_int64(*packer, node->id) ||
            qp_add_raw_from_str(*packer, "status") ||
            qp_add_raw_from_str(*packer, ti_node_status_str(node->status)) ||
            qp_add_raw_from_str(*packer, "commited_event_id") ||
            qp_add_int64(*packer, node->cevid) ||
            qp_add_raw_from_str(*packer, "stored_event_id") ||
            qp_add_int64(*packer, node->sevid))
            return -1;

        if ((this_node == node || node->stream) && (
                qp_add_raw_from_str(*packer, "name") ||
                qp_add_raw_from_str(*packer, ti_node_name(node))))
            return -1;


        if (node->flags && (
                qp_add_raw_from_str(*packer, "flags") ||
                qp_add_int64(*packer, node->flags)))
            return -1;

        if (qp_close_map(*packer))
            return -1;
    }

    return qp_close_array(*packer);
}

ti_val_t * ti_nodes_info_as_qpval(void)
{
    ti_raw_t * raw;
    ti_val_t * qpval = NULL;
    qp_packer_t * packer = qp_packer_create2(nodes->vec->n * 64, 2);
    if (!packer)
        return NULL;

    if (ti_nodes_info_to_packer(&packer))
        goto fail;

    raw = ti_raw_from_packer(packer);
    if (!raw)
        goto fail;

    qpval = ti_val_weak_create(TI_VAL_QP, raw);
    if (!qpval)
        ti_raw_drop(raw);

fail:
    qp_packer_destroy(packer);
    return qpval;
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
        uv_read_start(
                &stream->uvstream,
                ti_stream_alloc_buf,
                ti_stream_on_data);
    }
    else
    {
        ti_stream_drop(stream);
    }
}

static void nodes__on_req_query(ti_stream_t * stream, ti_pkg_t * pkg)
{

}

static void nodes__on_req_connect(ti_stream_t * stream, ti_pkg_t * pkg)
{

}

static void nodes__on_status(ti_stream_t * stream, ti_pkg_t * pkg)
{

}
