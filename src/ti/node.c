/*
 * node.c
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ti.h>
#include <ti/node.h>
#include <ti/nodes.h>
#include <ti/proto.h>
#include <ti/req.h>
#include <ti/tcp.h>
#include <ti/version.h>
#include <ti/write.h>
#include <util/logger.h>
#include <util/qpx.h>
#include <util/cryptx.h>

static void node__on_connect(uv_connect_t * req, int status);
static void node__on_connect_req(ti_req_t * req, ex_enum status);
static void node__clear_sockaddr(ti_node_t * node);

/*
 * Nodes are created ti_nodes_new_node() to ensure a correct id
 * is generated for each node.
 */
ti_node_t * ti_node_create(
        uint8_t id,
        uint8_t zone,
        uint16_t port,
        const char * addr,
        const char * secret)
{
    assert (strlen(secret) == CRYPTX_SZ - 1);
    assert (strlen(addr) < INET6_ADDRSTRLEN);

    ti_node_t * node = (ti_node_t *) malloc(sizeof(ti_node_t));
    if (!node)
        return NULL;

    node->ref = 1;
    node->id = id;
    node->status = TI_NODE_STAT_OFFLINE;
    node->zone = zone;
    node->syntax_ver = 0;
    node->next_retry = 0;
    node->retry_counter = 0;
    node->cevid = 0;
    node->sevid = 0;
    node->next_thing_id = 0;
    node->stream = NULL;
    node->port = port;
    node->sockaddr_ = NULL;     /* must be updated using
                                   node__update_sockaddr()
                                */
    strcpy(node->addr, addr);
    memcpy(node->secret, secret, CRYPTX_SZ);

    return node;
}

void ti_node_drop(ti_node_t * node)
{
    if (node && !--node->ref)
    {
        ti_stream_close(node->stream);
        free(node->sockaddr_);
        free(node);
    }
}

int ti_node_upd_addr_from_stream(
        ti_node_t * node,
        ti_stream_t * stream,
        uint16_t port)
{
    char addr[INET6_ADDRSTRLEN];

    /* try to update the address and port information if required */
    if (ti_stream_tcp_address(stream, addr))
    {
        log_warning("cannot read address from node stream");
        return -1;
    }

    if (strncmp(node->addr, addr, INET6_ADDRSTRLEN) || node->port != port)
    {
        node->port = port;
        node__clear_sockaddr(node);
        (void) strcpy(node->addr, addr);
        (void) ti_save();
    }
    return 0;
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
    case TI_NODE_STAT_CONNECTING:       return "CONNECTING";
    case TI_NODE_STAT_BUILDING:         return "BUILDING";
    case TI_NODE_STAT_SYNCHRONIZING:    return "SYNCHRONIZING";
    case TI_NODE_STAT_AWAY:             return "AWAY";
    case TI_NODE_STAT_AWAY_SOON:        return "AWAY_SOON";
    case TI_NODE_STAT_SHUTTING_DOWN:    return "SHUTTING_DOWN";
    case TI_NODE_STAT_READY:            return "READY";
    }
    return "UNKNOWN";
}

int ti_node_connect(ti_node_t * node)
{
    assert (!node->stream);
    assert (node->status == TI_NODE_STAT_OFFLINE);

    log_debug("connecting to "TI_NODE_ID, node->id);

    ti_stream_t * stream;
    uv_connect_t * req;

    if (!node->sockaddr_)
    {
        ex_t * e = ex_use();
        if (ti_node_update_sockaddr(node, e))
        {
            log_error(e->msg);
            return -1;
        }
    }

    stream = ti_stream_create(TI_STREAM_TCP_OUT_NODE, ti_nodes_pkg_cb);
    if (!stream)
        goto fail0;

    ti_stream_set_node(stream, node);

    req = malloc(sizeof(uv_connect_t));
    if (!req)
        goto fail0;

    req->data = ti_grab(node);
    node->status = TI_NODE_STAT_CONNECTING;

    if (uv_tcp_connect(
            req,
            (uv_tcp_t *) node->stream->uvstream,
            (const struct sockaddr*) node->sockaddr_,
            node__on_connect))
    {
        node->status = TI_NODE_STAT_OFFLINE;
        goto fail1;
    }

    return 0;

fail1:
    ti_node_drop(node);  /* break down req->data */
fail0:
    ti_stream_close(stream);
    return -1;
}

int ti_node_info_to_packer(ti_node_t * node, qp_packer_t ** packer)
{
    return (
        qp_add_array(packer) ||
        qp_add_int(*packer, node->next_thing_id) ||
        qp_add_int(*packer, node->cevid) ||
        qp_add_int(*packer, node->sevid) ||
        qp_add_int(*packer, node->status) ||
        qp_add_int(*packer, node->zone) ||
        qp_add_int(*packer, TI_VERSION_SYNTAX) ||
        qp_close_array(*packer)
    );
}

int ti_node_info_from_unp(ti_node_t * node, qp_unpacker_t * unp)
{
    qp_obj_t qpnext_thing_id, qpcevid, qpsevid, qpstatus, qpzone, qpsyntax;
    uint8_t syntax_ver;

    if (    !qp_is_array(qp_next(unp, NULL)) ||
            !qp_is_int(qp_next(unp, &qpnext_thing_id)) ||
            !qp_is_int(qp_next(unp, &qpcevid)) ||
            !qp_is_int(qp_next(unp, &qpsevid)) ||
            !qp_is_int(qp_next(unp, &qpstatus)) ||
            !qp_is_int(qp_next(unp, &qpzone)) ||
            !qp_is_int(qp_next(unp, &qpsyntax)))
        return -1;

    node->next_thing_id = (uint64_t) qpnext_thing_id.via.int64;
    node->cevid = (uint64_t) qpcevid.via.int64;
    node->sevid = (uint64_t) qpsevid.via.int64;
    node->status = (uint8_t) qpstatus.via.int64;
    node->zone = (uint8_t) qpzone.via.int64;
    syntax_ver = (uint8_t) qpsyntax.via.int64;

    if (syntax_ver != node->syntax_ver)
    {
        if (syntax_ver < node->syntax_ver)
            log_warning(
                    "got an unexpected syntax version update for `%s` "
                    "(current "TI_SYNTAX", received "TI_SYNTAX")",
                    ti_node_name(node), node->syntax_ver, syntax_ver);
        ti_nodes_update_syntax_ver(syntax_ver);
        node->syntax_ver = (uint8_t) qpsyntax.via.int64;
    }

    return 0;
}

int ti_node_update_sockaddr(ti_node_t * node, ex_t * e)
{
    assert (e->nr == 0);

    struct in_addr sa;
    struct in6_addr sa6;
    struct sockaddr_storage * tmp_sockaddr;

    tmp_sockaddr = malloc(sizeof(struct sockaddr_storage));
    if (!tmp_sockaddr)
    {
        ex_set_alloc(e);
        return e->nr;
    }

    if (inet_pton(AF_INET, node->addr, &sa))  /* try IPv4 */
    {
        if (uv_ip4_addr(
                node->addr,
                node->port,
                (struct sockaddr_in *) tmp_sockaddr))
        {
            ex_set(e, EX_INTERNAL,
                    "cannot create IPv4 address from `%s:%d`",
                    node->addr, node->port);
            goto failed;
        }
    }
    else if (inet_pton(AF_INET6, node->addr, &sa6))  /* try IPv6 */
    {
        if (uv_ip6_addr(
                node->addr,
                node->port,
                (struct sockaddr_in6 *) tmp_sockaddr))
        {
            ex_set(e, EX_INTERNAL,
                    "cannot create IPv6 address from `[%s]:%d`",
                    node->addr, node->port);
            goto failed;
        }
    }
    else
    {
        ex_set(e, EX_BAD_DATA, "invalid IPv4/6 address: `%s`", node->addr);
        goto failed;
    }

    assert (e->nr == 0);

    free(node->sockaddr_);
    node->sockaddr_ = tmp_sockaddr;

    return 0;

failed:
    free(tmp_sockaddr);
    return e->nr;
}

static void node__on_connect(uv_connect_t * req, int status)
{
    int rc;
    qpx_packer_t * packer;
    ti_pkg_t * pkg;
    ti_node_t * node = req->data;
    ti_node_t * ti_node = ti()->node;

    if (status)
    {
        log_error("connecting to "TI_NODE_ID" has failed (%s)",
                node->id,
                uv_strerror(status));
        goto failed;
    }

    if (node->stream != (ti_stream_t *) req->handle->data)
    {
        log_warning(
                "connection to "TI_NODE_ID" (%s) already established "
                "from the other side",
                node->id, ti_node_name(node));
        goto failed;
    }

    log_debug(
            "connection to "TI_NODE_ID" (%s) created",
            node->id,
            ti_node_name(node));

    rc = uv_read_start(req->handle, ti_stream_alloc_buf, ti_stream_on_data);
    if (rc)
    {
        log_error("cannot read TCP stream for "TI_NODE_ID": `%s`",
                node->id, uv_strerror(rc));
        goto failed;
    }

    packer = qpx_packer_create(192, 2);
    if (!packer)
    {
        log_error(EX_ALLOC_S);
        goto failed;
    }

    (void) qp_add_array(&packer);
    (void) qp_add_int(packer, node->id);
    (void) qp_add_raw(packer, (const uchar *) node->secret, CRYPTX_SZ);
    (void) qp_add_int(packer, ti_node->id);
    (void) qp_add_int(packer, ti_node->port);
    (void) qp_add_raw_from_str(packer, TI_VERSION);
    (void) qp_add_raw_from_str(packer, TI_MINIMAL_VERSION);
    (void) ti_node_info_to_packer(ti_node, &packer);
    (void) qp_close_array(packer);

    pkg = qpx_packer_pkg(packer, TI_PROTO_NODE_REQ_CONNECT);

    if (ti_req_create(
            node->stream,
            pkg,
            TI_PROTO_NODE_REQ_CONNECT_TIMEOUT,
            node__on_connect_req,
            node))
    {
        free(pkg);
        log_error(EX_INTERNAL_S);
        goto failed;
    }

    goto done;

failed:
    if (!uv_is_closing((uv_handle_t *) req->handle))
        ti_stream_close((ti_stream_t *) req->handle->data);
    ti_node_drop(node);

done:
    free(req);
}


static void node__on_connect_req(ti_req_t * req, ex_enum status)
{
    qp_unpacker_t unpacker;
    ti_pkg_t * pkg = req->pkg_res;
    ti_node_t * node = req->data;

    if (status)
        goto failed;

    if (pkg->tp != TI_PROTO_NODE_RES_CONNECT)
    {
        ti_pkg_log(pkg);
        goto failed;
    }

    if (node->stream != req->stream)
    {
        log_warning(
                "connection to "TI_NODE_ID" (%s) already established "
                "from the other side",
                node->id, ti_node_name(node));
        goto failed;
    }

    qp_unpacker_init2(&unpacker, pkg->data, pkg->n, 0);

    if (ti_node_info_from_unp(node, &unpacker))
    {
        log_error("invalid connect response from "TI_NODE_ID, node->id);
        goto failed;
    }

    /* reset the connection retry counters */
    node->next_retry = 0;
    node->retry_counter = 0;

    goto done;

failed:
    ti_stream_close(req->stream);
done:
    /* drop the request node reference */
    ti_node_drop(node);
    ti_req_destroy(req);
}

static void node__clear_sockaddr(ti_node_t * node)
{
    free(node->sockaddr_);
    node->sockaddr_ = NULL;
}

