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
#include <ti/write.h>
#include <util/logger.h>
#include <util/cryptx.h>

static void node__on_connect(uv_connect_t * req, int status);
static void node__on_connect_req(ti_req_t * req, ex_enum status);
static void node__clear_sockaddr(ti_node_t * node);

/*
 * Nodes are created ti_nodes_new_node() to ensure a correct id
 * is generated for each node.
 */
ti_node_t * ti_node_create(
        uint32_t id,
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

int ti_node_connect(ti_node_t * node, ex_t * e)
{
    assert (!node->stream);
    assert (node->status == TI_NODE_STAT_OFFLINE);

    log_debug("connecting to "TI_NODE_ID, node->id);

    int rc;
    ti_stream_t * stream;
    uv_connect_t * req;

    if (!node->sockaddr_ && ti_node_update_sockaddr(node, e))
        return e->nr;

    stream = ti_stream_create(TI_STREAM_TCP_OUT_NODE, ti_nodes_pkg_cb);
    if (!stream)
    {
        ex_set_internal(e);
        goto fail0;
    }

    ti_stream_set_node(stream, node);

    req = malloc(sizeof(uv_connect_t));
    if (!req)
    {
        ex_set_mem(e);
        goto fail0;
    }

    req->data = ti_grab(node);
    node->status = TI_NODE_STAT_CONNECTING;

    rc = uv_tcp_connect(
            req,
            (uv_tcp_t *) node->stream->uvstream,
            (const struct sockaddr*) node->sockaddr_,
            node__on_connect);

    if (rc)
    {
        ex_set(e, EX_INTERNAL,
                "TCP connect to "TI_NODE_ID" has failed (%s)",
                node->id, uv_strerror(rc));
        node->status = TI_NODE_STAT_OFFLINE;
        goto fail1;
    }

    return 0;

fail1:
    ti_node_drop(node);
    free(req);

fail0:
    ti_stream_close(stream);
    return e->nr;
}

int ti_node_info_to_pk(ti_node_t * node, msgpack_packer * pk)
{
    static char syntax_buf[7]; /* vXXXXX_ */
    ti_node_t * this_node = ti()->node;
    (void) sprintf(syntax_buf, "v%"PRIu16, node->syntax_ver);

    return -(
        msgpack_pack_map(pk, 10) ||

        mp_pack_str(pk, "node_id") ||
        msgpack_pack_uint32(pk, node->id) ||

        mp_pack_str(pk, "syntax_version") ||
        mp_pack_str(pk, syntax_buf) ||

        mp_pack_str(pk, "status") ||
        mp_pack_str(pk, ti_node_status_str(node->status)) ||

        mp_pack_str(pk, "zone") ||
        msgpack_pack_uint8(pk, node->zone) ||

        mp_pack_str(pk, "committed_event_id") ||
        msgpack_pack_uint64(pk, node->cevid) ||

        mp_pack_str(pk, "stored_event_id") ||
        msgpack_pack_uint64(pk, node->sevid) ||

        mp_pack_str(pk, "next_thing_id") ||
        msgpack_pack_uint64(pk, node->next_thing_id) ||

        mp_pack_str(pk, "address") ||
        mp_pack_str(
                pk,
                node == this_node ? ti_name() : node->addr) ||

        mp_pack_str(pk, "port") ||
        msgpack_pack_uint16(pk, node->port) ||

        mp_pack_str(pk, "stream") ||
        (ti_stream_is_closed(node->stream)
            ? msgpack_pack_nil(pk)
            : mp_pack_str(pk, ti_stream_name(node->stream)))
    );
}

ti_val_t * ti_node_as_mpval(ti_node_t * node)
{
    ti_raw_t * raw;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_raw_t), sizeof(ti_raw_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (ti_node_info_to_pk(node, &pk))
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    raw = (ti_raw_t *) buffer.data;
    ti_raw_init(raw, TI_VAL_MP, buffer.size);

    return (ti_val_t *) raw;
}

int ti_node_status_from_unp(ti_node_t * node, mp_unp_t * up)
{
    mp_obj_t obj,
             mp_next_thing_id,
             mp_cevid,
             mp_sevid,
             mp_status,
             mp_zone,
             mp_port,
             mp_syntax_ver;
    uint16_t syntax_ver;
    uint16_t node_port;

    if (mp_next(up, &obj) != MP_ARR || obj.via.sz != 7 ||
        mp_next(up, &mp_next_thing_id) != MP_U64 ||
        mp_next(up, &mp_cevid) != MP_U64 ||
        mp_next(up, &mp_sevid) != MP_U64 ||
        mp_next(up, &mp_status) != MP_U64 ||
        mp_next(up, &mp_zone) != MP_U64 ||
        mp_next(up, &mp_port) != MP_U64 ||
        mp_next(up, &mp_syntax_ver) != MP_U64
    ) return -1;

    node->next_thing_id = mp_next_thing_id.via.u64;
    node->cevid = mp_cevid.via.u64;
    node->sevid = mp_sevid.via.u64;
    node->status = mp_status.via.u64;
    node->zone = mp_zone.via.u64;
    syntax_ver = mp_syntax_ver.via.u64;
    node_port = mp_port.via.u64;

    if (syntax_ver != node->syntax_ver)
    {
        if (syntax_ver < node->syntax_ver)
            log_warning(
                    "got an unexpected syntax version update for `%s` "
                    "(current "TI_QBIND", received "TI_QBIND")",
                    ti_node_name(node), node->syntax_ver, syntax_ver);
        ti_nodes_update_syntax_ver(syntax_ver);
        node->syntax_ver = mp_syntax_ver.via.u64;
    }

    if (node_port != node->port)
    {
        node->port = node_port;
        ti()->flags |= TI_FLAG_NODES_CHANGED;
    }

    if (node->status == TI_NODE_STAT_AWAY)
        ti_away_set_away_node_id(node->id);

    if (node->status == TI_NODE_STAT_SHUTTING_DOWN)
        ti_away_reschedule();  /* reschedule away */

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
        ex_set_mem(e);
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
                    "cannot create IPv4 address from `%s:%d` for "TI_NODE_ID,
                    node->addr, node->port, node->id);
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
                    "cannot create IPv6 address from `[%s]:%d` for "TI_NODE_ID,
                    node->addr, node->port, node->id);
            goto failed;
        }
    }
    else
    {
        ex_set(e, EX_VALUE_ERROR,
                "invalid IPv4/6 address for "TI_NODE_ID
                ": `%s`", node->id, node->addr);
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

static void node__upd_address(ti_node_t * node)
{
    char addr[INET6_ADDRSTRLEN];

    if (ti_tcp_addr(addr, (uv_tcp_t *) node->stream->uvstream))
        return;

    if (strcmp(addr, node->addr))
    {
        (void) strcpy(node->addr, addr);
        ti()->flags |= TI_FLAG_NODES_CHANGED;
    }
}

static void node__on_connect(uv_connect_t * req, int status)
{
    int rc;
    msgpack_packer pk;
    msgpack_sbuffer buffer;
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
                "cancel connect; connection to "TI_NODE_ID" (%s) already  "
                "established from the other side",
                node->id, ti_node_name(node));
        goto failed;
    }

    log_debug(
            "connection to "TI_NODE_ID" (%s) created",
            node->id,
            ti_node_name(node));

    node__upd_address(node);

    rc = uv_read_start(req->handle, ti_stream_alloc_buf, ti_stream_on_data);
    if (rc)
    {
        log_error("cannot read TCP stream for "TI_NODE_ID": `%s`",
                node->id, uv_strerror(rc));
        goto failed;
    }

    if (mp_sbuffer_alloc_init(&buffer, 512, sizeof(ti_pkg_t)))
    {
        log_critical(EX_MEMORY_S);
        goto failed;
    }

    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 6);
    msgpack_pack_uint32(&pk, node->id);
    mp_pack_strn(&pk, node->secret, CRYPTX_SZ);
    msgpack_pack_uint32(&pk, ti_node->id);
    mp_pack_str(&pk, TI_VERSION);
    mp_pack_str(&pk, TI_MINIMAL_VERSION);
    ti_node_status_to_pk(ti_node, &pk);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_NODE_REQ_CONNECT, buffer.size);

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
    mp_unp_t up;
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
                "ignore connect request; connection to "TI_NODE_ID" (%s) "
                "already established from the other side",
                node->id, ti_node_name(node));
        goto failed;
    }

    mp_unp_init(&up, pkg->data, pkg->n);

    if (ti_node_status_from_unp(node, &up))
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

