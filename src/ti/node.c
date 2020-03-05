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
    node->addr = strdup(addr);
    memcpy(node->secret, secret, CRYPTX_SZ);

    if (!node->addr)
    {
        free(node);
        return NULL;
    }

    return node;
}

void ti_node_drop(ti_node_t * node)
{
    if (node && !--node->ref)
    {
        if (node->stream)
        {
            /* usually the stream should already be closed */
            node->stream->via.node = NULL;
            ti_stream_close(node->stream);
        }
        free(node->addr);
        free(node);
    }
}

void ti_node_upd_port(ti_node_t * node, uint16_t port)
{
    if (node->port != port)
    {
        node->port = port;
        (void) ti_save();
    }
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

static void node__connect(ti_node_t * node, struct sockaddr_storage * sockaddr)
{
    int rc;
    ti_stream_t * stream;
    uv_connect_t * req;

    if (node->stream)
        return;

    stream = ti_stream_create(TI_STREAM_TCP_OUT_NODE, ti_nodes_pkg_cb);
    if (!stream)
    {
        log_error(EX_INTERNAL_S);
        goto fail0;
    }

    ti_stream_set_node(stream, node);

    req = malloc(sizeof(uv_connect_t));
    if (!req)
    {
        log_error(EX_MEMORY_S);
        goto fail0;
    }

    req->data = node;
    node->status = TI_NODE_STAT_CONNECTING;
    ti_incref(node);

    rc = uv_tcp_connect(
            req,
            (uv_tcp_t *) node->stream->uvstream,
            (const struct sockaddr *) sockaddr,
            node__on_connect);

    if (rc)
    {
        log_error(
                "TCP connect to "TI_NODE_ID" has failed (%s)",
                node->id, uv_strerror(rc));
        node->status = TI_NODE_STAT_OFFLINE;
        goto fail1;
    }

    return;

fail1:
    ti_node_drop(node);
    free(req);

fail0:
    ti_stream_close(stream);
}

/*
 * Return 0 if successful, < 0 in case of error, > 0 for lookup required
 */
static int node__addr(
        ti_node_t * node,
        const char * addr,
        struct sockaddr_storage * sockaddr)
{
    struct in_addr sa;
    struct in6_addr sa6;
    if (inet_pton(AF_INET, addr, &sa))  /* try IPv4 */
    {
        if (uv_ip4_addr(
                addr,
                node->port,
                (struct sockaddr_in *) sockaddr))
        {
            log_error(
                    "cannot create IPv4 address from `%s:%d` for "TI_NODE_ID,
                    addr, node->port, node->id);
            return -1;
        }
        return 0;
    }

    if (inet_pton(AF_INET6, addr, &sa6))  /* try IPv6 */
    {
        if (uv_ip6_addr(
                addr,
                node->port,
                (struct sockaddr_in6 *) sockaddr))
        {
            log_error(
                    "cannot create IPv6 address from `[%s]:%d` for "TI_NODE_ID,
                    addr, node->port, node->id);
            return -1;
        }
        return 0;
    }
    return 1;
}

static void node__on_resolved(
        uv_getaddrinfo_t * resolver,
        int status,
        struct addrinfo * res)
{
    char addr[INET6_ADDRSTRLEN];
    struct sockaddr_storage sockaddr;
    ti_node_t * node = resolver->data;

    if (status < 0)
    {
        log_error("cannot resolve address for `%s` (error: %s)",
                node->addr,
                uv_err_name(status));
        goto done;
    }

    if (ti_tcp_addrstr(addr, res))
    {
        log_error("failed to read address for `%s`", node->addr);
        goto done;
    }

    if (node->stream)
    {
        log_info(
                "resolved address `%s` for `%s` but "
                "a connection to "TI_NODE_ID" has already been made",
                addr, node->addr, node->id);
        goto done;
    }

    log_info("resolved address `%s` for `%s`", addr, node->addr);

    if (node__addr(node, addr, &sockaddr)== 0)
        node__connect(node, &sockaddr);

done:
    ti_node_drop(node);
    uv_freeaddrinfo(res);
    free(resolver);
}

static void node__resolve_dns(ti_node_t * node)
{
    int rc;
    char port[6]= {0};
    struct addrinfo hints = {
            .ai_family = AF_UNSPEC,
            .ai_socktype = SOCK_STREAM,
            .ai_protocol = IPPROTO_TCP,
            .ai_flags = AI_NUMERICSERV,
    };
    uv_getaddrinfo_t * resolver = malloc(sizeof(uv_getaddrinfo_t));
    if (!resolver)
        return;

    ti_incref(node);
    resolver->data = node;
    sprintf(port, "%u", node->port);

    rc = uv_getaddrinfo(
            ti()->loop,
            resolver,
            node__on_resolved,
            node->addr,
            port,
            &hints);

    if (rc)
    {
        log_error("uv_getaddrinfo call error %s", uv_err_name(rc));
        free(resolver);
        ti_node_drop(node);
    }
}

void ti_node_connect(ti_node_t * node)
{
    int rc;
    struct sockaddr_storage sockaddr;

    log_debug("connecting to "TI_NODE_ID, node->id);

    rc = node__addr(node, node->addr, &sockaddr);
    if (rc == 0)
        node__connect(node, &sockaddr);
    else if (rc > 0)
        node__resolve_dns(node);
}

int ti_node_info_to_pk(ti_node_t * node, msgpack_packer * pk)
{
    static char syntax_buf[7]; /* vXXXXX_ */
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

        mp_pack_str(pk, "node_name") ||
        mp_pack_str(pk, node->addr) ||

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
                    "got an unexpected syntax version update from "TI_NODE_ID
                    " (current "TI_QBIND", received "TI_QBIND")",
                    node->id, node->syntax_ver, syntax_ver);
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
                "cancel connect; connection to "TI_NODE_ID" (%s:%u) already  "
                "established from the other side",
                node->id, node->addr, node->port);
        goto failed;
    }

    log_debug(
            "connection to "TI_NODE_ID" (%s:%u) created",
            node->id, node->addr, node->port);

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
                "ignore connect request; connection to "TI_NODE_ID" (%s:%u) "
                "already established from the other side",
                node->id, node->addr, node->port);
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

