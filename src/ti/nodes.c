/*
 * nodes.c
 */
#include <stdbool.h>
#include <assert.h>
#include <ti/nodes.h>
#include <ti/proto.h>
#include <ti/version.h>
#include <ti/access.h>
#include <ti/auth.h>
#include <ti/away.h>
#include <ti/args.h>
#include <ti/syncfull.h>
#include <ti/syncarchive.h>
#include <ti/syncevents.h>
#include <ti.h>
#include <util/cryptx.h>
#include <util/qpx.h>

#define NODES__UV_BACKLOG 64

typedef ti_pkg_t * (*nodes__part_cb) (ti_pkg_t *, ex_t *);

static ti_node_t * nodes__o[63];    /* other zone */
static ti_node_t * nodes__z[63];    /* same zone */
static ti_nodes_t * nodes;

static void nodes__tcp_connection(uv_stream_t * uvstream, int status);
static void nodes__on_req_connect(ti_stream_t * stream, ti_pkg_t * pkg);
static void nodes__on_req_event_id(ti_stream_t * stream, ti_pkg_t * pkg);
static void nodes__on_req_away(ti_stream_t * stream, ti_pkg_t * pkg);
static void nodes__on_req_query(ti_stream_t * stream, ti_pkg_t * pkg);
static void nodes__on_req_setup(ti_stream_t * stream, ti_pkg_t * pkg);
static void nodes__on_req_sync(ti_stream_t * stream, ti_pkg_t * pkg);
static void nodes__on_req_syncpart(
        ti_stream_t * stream,
        ti_pkg_t * pkg,
        nodes__part_cb part_cb);
static void nodes__on_req_syncfdone(ti_stream_t * stream, ti_pkg_t * pkg);
static void nodes__on_req_syncadone(ti_stream_t * stream, ti_pkg_t * pkg);
static void nodes__on_req_syncedone(ti_stream_t * stream, ti_pkg_t * pkg);
static void nodes__on_event(ti_stream_t * stream, ti_pkg_t * pkg);
static void nodes__on_info(ti_stream_t * stream, ti_pkg_t * pkg);

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
    ti()->nodes = nodes;

    return -(nodes == NULL);
}

void ti_nodes_destroy(void)
{
    if (!nodes)
        return;
    vec_destroy(nodes->vec, (vec_destroy_cb) ti_node_drop);
    free(nodes);
    ti()->nodes = nodes = NULL;
}

/*
 * Number of nodes required, `this` node excluded.
 */
uint8_t ti_nodes_quorum(void)
{
    return (uint8_t) (nodes->vec->n / 2);
}

_Bool ti_nodes_has_quorum(void)
{
    size_t quorum = ti_nodes_quorum() + 1;  /* include `this` node */
    size_t q = 0;

    for (vec_each(nodes->vec, ti_node_t, node))

        if (node->status > TI_NODE_STAT_CONNECTING && ++q == quorum)
            return true;
    return false;
}

/* increases with a new reference as long as required */
void ti_nodes_write_rpkg(ti_rpkg_t * rpkg)
{
    ti_node_t * this_node = ti()->node;
    for (vec_each(nodes->vec, ti_node_t, node))
    {
        ti_node_status_t status = node->status;

        if (node == this_node)
            continue;

        if (    status != TI_NODE_STAT_READY &&
                status != TI_NODE_STAT_AWAY_SOON &&
                status != TI_NODE_STAT_AWAY &&
                status != TI_NODE_STAT_SYNCHRONIZING)
            continue;

        if (ti_stream_write_rpkg(node->stream, rpkg))
            log_error(EX_INTERNAL_S);
    }
}

int ti_nodes_to_packer(qp_packer_t ** packer)
{
    if (qp_add_array(packer))
        return -1;

    for (vec_each(nodes->vec, ti_node_t, node))
    {
        if (qp_add_array(packer) ||
            qp_add_int(*packer, node->zone) ||
            qp_add_int(*packer, node->port) ||
            qp_add_raw_from_str(*packer, node->addr) ||
            qp_add_raw(*packer, (const uchar *) node->secret, CRYPTX_SZ) ||
            qp_close_array(*packer))
            return -1;
    }

    return qp_close_array(*packer);
}

int ti_nodes_from_qpres(qp_res_t * qpnodes)
{
    for (uint32_t i = 0, j = qpnodes->via.array->n; i < j; i++)
    {
        char addr[INET6_ADDRSTRLEN];
        uint16_t port;
        uint8_t zone;
        ti_node_t * node;
        const char * secret;
        qp_res_t * qpzone, * qpport, * qpaddr, * qpsecret;
        qp_res_t * qparray = qpnodes->via.array->values + i;

        if (qparray->tp != QP_RES_ARRAY || qparray->via.array->n != 4)
            return -1;

        qpzone = qparray->via.array->values + 0;
        qpport = qparray->via.array->values + 1;
        qpaddr = qparray->via.array->values + 2;
        qpsecret = qparray->via.array->values + 3;

        if (    qpzone->tp != QP_RES_INT64 ||
                qpport->tp != QP_RES_INT64 ||
                qpaddr->tp != QP_RES_RAW ||
                qpaddr->via.raw->n >= INET6_ADDRSTRLEN ||
                qpsecret->tp != QP_RES_RAW ||
                qpsecret->via.raw->n != CRYPTX_SZ)
            return -1;

        zone = (uint8_t) qpzone->via.int64;
        port = (uint16_t) qpport->via.int64;

        memcpy(addr, qpaddr->via.raw->data, qpaddr->via.raw->n);
        addr[qpaddr->via.raw->n] = '\0';

        secret = (const char *) qpsecret->via.raw->data;

        node = ti_nodes_new_node(zone, port, addr, secret);
        if (!node)
            return -1;

    }
    return 0;
}

_Bool ti_nodes_ignore_sync(void)
{
    uint64_t m = *ti()->events->cevid;
    uint8_t n = 0;

    for (vec_each(nodes->vec, ti_node_t, node))
    {
        if (node->cevid > m || node->status > TI_NODE_STAT_SYNCHRONIZING)
            return false;

        if (node->status == TI_NODE_STAT_SYNCHRONIZING)
            ++n;
    }
    return n > ti_nodes_quorum();
}

_Bool ti_nodes_require_sync(void)
{
    for (vec_each(nodes->vec, ti_node_t, node))
        if (node->status == TI_NODE_STAT_SYNCHRONIZING)
            return true;
    return false;
}

uint64_t ti_nodes_cevid(void)
{
    uint64_t m = *ti()->events->cevid;

    for (vec_each(nodes->vec, ti_node_t, node))
        if (node->cevid < m)
            m = node->cevid;

    if (m > nodes->cevid)
        nodes->cevid = m;

    return nodes->cevid;
}

uint64_t ti_nodes_sevid(void)
{
    uint64_t m = *ti()->archive->sevid;

    for (vec_each(nodes->vec, ti_node_t, node))
        if (node->sevid < m)
            m = node->sevid;

    if (m > nodes->sevid)
        nodes->sevid = m;

    return nodes->sevid;
}

ti_node_t * ti_nodes_new_node(
        uint8_t zone,
        uint16_t port,
        const char * addr,
        const char * secret)
{
    ti_node_t * node = ti_node_create(nodes->vec->n, zone, port, addr, secret);
    if (!node || vec_push(&nodes->vec, node))
    {
        ti_node_drop(node);
        return NULL;
    }
    assert (node->id == nodes->vec->n - 1);
    return node;
}

void ti_nodes_pop_node(void)
{
    ti_node_drop(vec_pop(nodes->vec));
}

ti_node_t * ti_nodes_node_by_id(uint8_t node_id)
{
    return node_id >= nodes->vec->n ? NULL : vec_get(nodes->vec, node_id);
}

int ti_nodes_listen(void)
{
    struct sockaddr_storage addr = {0};
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
        uv_ip6_addr(ip, cfg->node_port, (struct sockaddr_in6 *) &addr);
    }
    else
    {
        uv_ip4_addr(ip, cfg->node_port, (struct sockaddr_in *) &addr);
    }

    if ((rc = uv_tcp_bind(
            &nodes->tcp,
            (const struct sockaddr *) &addr,
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
 * Returns another borrowed node with status READY if possible from the same
 * zone of NULL if no ready node is found. (not thread safe)
 */
ti_node_t * ti_nodes_random_ready_node(void)
{
    ti_node_t * this_node = ti()->node;
    uint32_t zn = 0, on = 0;

    for (vec_each(nodes->vec, ti_node_t, node))
    {
        if (node == this_node || node->status != TI_NODE_STAT_READY)
            continue;

        if (this_node->zone == node->zone)
            nodes__z[zn++] = node;
        else
            nodes__o[on++] = node;
    }
    return zn ? nodes__z[rand() % zn] : on ? nodes__o[rand() % on] : NULL;
}

void ti_nodes_pkg_cb(ti_stream_t * stream, ti_pkg_t * pkg)
{
    switch (pkg->tp)
    {
    case TI_PROTO_CLIENT_RES_QUERY:
    case TI_PROTO_CLIENT_ERR_OVERFLOW:
    case TI_PROTO_CLIENT_ERR_ZERO_DIV:
    case TI_PROTO_CLIENT_ERR_MAX_QUOTA:
    case TI_PROTO_CLIENT_ERR_AUTH:
    case TI_PROTO_CLIENT_ERR_FORBIDDEN:
    case TI_PROTO_CLIENT_ERR_INDEX:
    case TI_PROTO_CLIENT_ERR_BAD_REQUEST:
    case TI_PROTO_CLIENT_ERR_QUERY:
    case TI_PROTO_CLIENT_ERR_NODE:
    case TI_PROTO_CLIENT_ERR_INTERNAL:
        ti_stream_on_response(stream, pkg);
        break;
    case TI_PROTO_NODE_EVENT:
        nodes__on_event(stream, pkg);
        break;
    case TI_PROTO_NODE_INFO:
        nodes__on_info(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_QUERY:
        nodes__on_req_query(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_CONNECT:
        nodes__on_req_connect(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_EVENT_ID:
        nodes__on_req_event_id(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_AWAY:
        nodes__on_req_away(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_SETUP:
        nodes__on_req_setup(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_SYNC:
        nodes__on_req_sync(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_SYNCFPART:
        nodes__on_req_syncpart(stream, pkg, ti_syncfull_on_part);
        break;
    case TI_PROTO_NODE_REQ_SYNCFDONE:
        nodes__on_req_syncfdone(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_SYNCAPART:
        nodes__on_req_syncpart(stream, pkg, ti_syncarchive_on_part);
        break;
    case TI_PROTO_NODE_REQ_SYNCADONE:
        nodes__on_req_syncadone(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_SYNCEPART:
        nodes__on_req_syncpart(stream, pkg, ti_syncevents_on_part);
        break;
    case TI_PROTO_NODE_REQ_SYNCEDONE:
        nodes__on_req_syncedone(stream, pkg);
        break;
    case TI_PROTO_NODE_RES_CONNECT:
    case TI_PROTO_NODE_RES_EVENT_ID:
    case TI_PROTO_NODE_RES_AWAY:
    case TI_PROTO_NODE_RES_SETUP:
    case TI_PROTO_NODE_RES_SYNC:
    case TI_PROTO_NODE_RES_SYNCFPART:
    case TI_PROTO_NODE_RES_SYNCFDONE:
    case TI_PROTO_NODE_RES_SYNCAPART:
    case TI_PROTO_NODE_RES_SYNCADONE:
    case TI_PROTO_NODE_RES_SYNCEPART:
    case TI_PROTO_NODE_RES_SYNCEDONE:
    case TI_PROTO_NODE_ERR_RES:
    case TI_PROTO_NODE_ERR_EVENT_ID:
    case TI_PROTO_NODE_ERR_AWAY:
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
            qp_add_raw_from_str(*packer, "node_id") ||
            qp_add_int(*packer, node->id) ||
            qp_add_raw_from_str(*packer, "status") ||
            qp_add_raw_from_str(*packer, ti_node_status_str(node->status)) ||
            qp_add_raw_from_str(*packer, "commited_event_id") ||
            qp_add_int(*packer, node->cevid) ||
            qp_add_raw_from_str(*packer, "stored_event_id") ||
            qp_add_int(*packer, node->sevid) ||
            qp_add_raw_from_str(*packer, "address") ||
            qp_add_raw_from_str(
                    *packer,
                    node == this_node ? ti_name() : node->addr) ||
            qp_add_raw_from_str(*packer, "port") ||
            qp_add_int(*packer, node->port))
            return -1;

        if (!ti_stream_is_closed(node->stream) && (
                qp_add_raw_from_str(*packer, "stream") ||
                qp_add_raw_from_str(*packer, ti_stream_name(node->stream))))
            return -1;

        if (qp_close_map(*packer))
            return -1;
    }

    return qp_close_array(*packer);
}

ti_val_t * ti_nodes_info_as_qpval(void)
{
    ti_raw_t * raw = NULL;
    qp_packer_t * packer = qp_packer_create2(nodes->vec->n * 64, 2);
    if (!packer)
        return NULL;

    if (ti_nodes_info_to_packer(&packer))
        goto fail;

    raw = ti_raw_from_packer(packer);
    if (!raw)
        goto fail;

fail:
    qp_packer_destroy(packer);
    return (ti_val_t *) raw;
}

static void nodes__tcp_connection(uv_stream_t * uvstream, int status)
{
    int rc;
    ti_stream_t * stream;

    if (status < 0)
    {
        log_error("node connection error: `%s`", uv_strerror(status));
        return;
    }

    log_debug("received a TCP node connection");

    stream = ti_stream_create(TI_STREAM_TCP_IN_NODE, &ti_nodes_pkg_cb);
    if (!stream)
    {
        log_critical(EX_ALLOC_S);
        return;
    }

    rc = uv_accept(uvstream, stream->uvstream);
    if (rc)
        goto failed;

    rc = uv_read_start(
            stream->uvstream,
            ti_stream_alloc_buf,
            ti_stream_on_data);
    if (rc)
        goto failed;

    return;

failed:
    log_error("cannot read node TCP stream: `%s`", uv_strerror(rc));
    ti_stream_drop(stream);
}

static void nodes__on_req_connect(ti_stream_t * stream, ti_pkg_t * pkg)
{
    assert (stream->tp == TI_STREAM_TCP_IN_NODE);
    assert (stream->via.node == NULL);

    ti_pkg_t * resp = NULL;
    qp_unpacker_t unpacker;
    qp_obj_t
        qp_this_node_id,
        qp_secret,
        qp_from_node_id,
        qp_from_node_port,
        qp_version,
        qp_min_ver,
        qp_next_thing_id,
        qp_cevid,
        qp_sevid,
        qp_status,
        qp_zone;

    uint8_t
        this_node_id,
        from_node_id,
        from_node_status,
        from_node_zone;

    uint16_t from_node_port;
    ti_node_t * node, * this_node = ti()->node;
    char * min_ver = NULL;
    char * version = NULL;
    qpx_packer_t * packer = NULL;

    qp_unpacker_init2(&unpacker, pkg->data, pkg->n, 0);

    if (    !qp_is_array(qp_next(&unpacker, NULL)) ||
            !qp_is_int(qp_next(&unpacker, &qp_this_node_id)) ||
            !qp_is_raw(qp_next(&unpacker, &qp_secret)) ||
            qp_secret.len != CRYPTX_SZ ||
            qp_secret.via.raw[qp_secret.len-1] != '\0' ||
            !qp_is_int(qp_next(&unpacker, &qp_from_node_id)) ||
            !qp_is_int(qp_next(&unpacker, &qp_from_node_port)) ||
            !qp_is_raw(qp_next(&unpacker, &qp_version)) ||
            !qp_is_raw(qp_next(&unpacker, &qp_min_ver)) ||

            !qp_is_array(qp_next(&unpacker, NULL)) ||
            !qp_is_int(qp_next(&unpacker, &qp_next_thing_id)) ||
            !qp_is_int(qp_next(&unpacker, &qp_cevid)) ||
            !qp_is_int(qp_next(&unpacker, &qp_sevid)) ||
            !qp_is_int(qp_next(&unpacker, &qp_status)) ||
            !qp_is_int(qp_next(&unpacker, &qp_zone)))
    {
        log_error(
                "invalid connection request from `%s`",
                ti_stream_name(stream));
        goto failed;
    }

    this_node_id = (uint8_t) qp_this_node_id.via.int64;
    from_node_id = (uint8_t) qp_from_node_id.via.int64;
    from_node_port = (uint16_t) qp_from_node_port.via.int64;
    from_node_status = (uint8_t) qp_status.via.int64;
    from_node_zone = (uint8_t) qp_zone.via.int64;

    if (from_node_id == this_node_id)
    {
        log_error(
            "got a connection request from `%s` "
            "with the same source and target: "TI_NODE_ID,
            ti_stream_name(stream),
            from_node_id);
        goto failed;
    }

    version = strndup((const char *) qp_version.via.raw, qp_version.len);
    min_ver = strndup((const char *) qp_min_ver.via.raw, qp_min_ver.len);

    if (!version || !min_ver)
    {
        log_critical(EX_ALLOC_S);
        goto failed;
    }

    if (ti_version_cmp(version, TI_MINIMAL_VERSION) < 0)
    {
        log_error(
            "connection request received from `%s` using version `%s` but at "
            "least version `%s` is required",
            ti_stream_name(stream),
            version,
            TI_MINIMAL_VERSION);
        goto failed;
    }

    if (ti_version_cmp(TI_VERSION, min_ver) < 0)
    {
        log_error(
            "connection request received from `%s` which requires at "
            "least version `%s` but this node is running version `%s`",
            ti_stream_name(stream),
            min_ver,
            TI_VERSION);
        goto failed;
    }

    if (!ti()->node)
    {
        assert (*ti()->args->secret);
        assert (ti()->build);

        if (ti()->build->status == TI_BUILD_REQ_SETUP)
        {
            log_info(
                    "ignore connection request from `%s` since this node is ",
                    "busy building ThingsDB",
                    ti_stream_name(stream));
            goto failed;
        }

        char validate[CRYPTX_SZ];

        cryptx(ti()->args->secret, (const char *) qp_secret.via.raw, validate);
        if (memcmp(qp_secret.via.raw, validate, CRYPTX_SZ))
        {
            log_error(
                "connection request received from `%s` with an invalid secret",
                ti_stream_name(stream));
            goto failed;
        }

        (void) ti_build_setup(
                this_node_id,
                from_node_id,
                from_node_status,
                from_node_zone,
                from_node_port,
                stream);

        packer = qpx_packer_create(32, 1);
        if (!packer)
        {
            log_critical(EX_ALLOC_S);
            goto failed;
        }

        (void) qp_add_array(&packer);
        (void) qp_add_int(packer, 0);                       /* next_thing_id */
        (void) qp_add_int(packer, 0);                       /* cevid */
        (void) qp_add_int(packer, 0);                       /* sevid */
        (void) qp_add_int(packer, TI_NODE_STAT_BUILDING);   /* status */
        (void) qp_add_int(packer, ti_args_get_zone());      /* zone */
        (void) qp_close_array(packer);

        goto send;
    }

    if (this_node_id != this_node->id)
    {
        log_error(
            "this is "TI_NODE_ID" but got a connection request "
            "from `%s` who thinks this is "TI_NODE_ID,
            this_node->id,
            ti_stream_name(stream),
            this_node_id);
        goto failed;
    }

    if (memcmp(qp_secret.via.raw, this_node->secret, CRYPTX_SZ))
    {
        log_error(
            "connection request received from `%s` with an invalid secret",
            ti_stream_name(stream));
        goto failed;
    }

    node = ti_nodes_node_by_id(from_node_id);
    if (!node)
    {
        log_error(
            "cannot accept connection request received from `%s` "
            "because "TI_NODE_ID" is not found",
            ti_stream_name(stream),
            from_node_id);
        goto failed;
    }

    if (node->status > TI_NODE_STAT_CONNECTING)
    {
        log_error(
            "cannot accept connection request received from `%s` "
            "because "TI_NODE_ID" is already connected",
            ti_stream_name(stream),
            from_node_id);
        goto failed;
    }

    node->status = from_node_status;
    node->zone = from_node_zone;
    node->cevid = (uint64_t) qp_cevid.via.int64;
    node->sevid = (uint64_t) qp_sevid.via.int64;
    node->next_thing_id = (uint64_t) qp_next_thing_id.via.int64;

    if (node->stream)
    {
        assert (node->stream->via.node == node);

        if (node->id < this_node->id)
        {
            log_warning(
                    "connection request from `%s` rejected since a connection "
                    "with "TI_NODE_ID" is already established",
                    ti_stream_name(stream),
                    node->id);
            goto failed;
        }

        assert (node->id > this_node->id);
        log_warning("changing stream for "TI_NODE_ID" from `%s` to `%s",
                node->id,
                ti_stream_name(node->stream),
                ti_stream_name(stream));
        /*
         * We leave the old stream alone since it will be closed once a
         * connection response is received.
         */
        ti_node_drop(node);

        node->stream->via.node = NULL;
        node->stream = NULL;
    }

    ti_stream_set_node(stream, node);

    /* try to update the address and port information if required */
    (void) ti_node_upd_addr_from_stream(node, stream, from_node_port);

    packer = qpx_packer_create(32, 1);
    if (!packer)
    {
        log_critical(EX_ALLOC_S);
        goto failed;
    }

    (void) ti_node_info_to_packer(this_node, &packer);

send:
    resp = qpx_packer_pkg(packer, TI_PROTO_NODE_RES_CONNECT);
    resp->id = pkg->id;

    if (ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_INTERNAL_S);
    }

    goto done;

failed:
    qpx_packer_destroy(packer);

done:
    free(version);
    free(min_ver);
}

static void nodes__on_req_event_id(ti_stream_t * stream, ti_pkg_t * pkg)
{
    _Bool accepted;
    ex_t * e = ex_use();
    qp_unpacker_t unpacker;
    ti_pkg_t * resp = NULL;
    ti_node_t * other_node = stream->via.node;
    ti_node_t * this_node = ti()->node;
    qp_obj_t qp_event_id;

    if (!this_node)
    {
        ex_set(e, EX_AUTH_ERROR,
                "got an `%s` request from an unauthorized connection: `%s`",
                ti_proto_str(pkg->id), ti()->hostname);
        goto finish;
    }

    if (this_node->status < TI_NODE_STAT_SYNCHRONIZING)
    {
        ex_set(e, EX_NODE_ERROR,
                "node `%s` is not ready to handle `%s` requests",
                ti()->hostname, ti_proto_str(pkg->id));
        goto finish;
    }

    qp_unpacker_init2(&unpacker, pkg->data, pkg->n, 0);
    if (!qp_is_int(qp_next(&unpacker, &qp_event_id)))
    {
        ex_set(e, EX_BAD_DATA,
                "invalid `%s` request from "TI_NODE_ID" to "TI_NODE_ID,
                ti_proto_str(pkg->id), other_node->id, this_node->id);
        goto finish;
    }

    accepted = ti_events_slave_req(
            other_node,
            (uint64_t) qp_event_id.via.int64);

    assert (e->nr == 0);
    resp = ti_pkg_new(
            pkg->id,
            accepted ? TI_PROTO_NODE_RES_EVENT_ID : TI_PROTO_NODE_ERR_EVENT_ID,
            NULL,
            0);

finish:
    if (e->nr)
        resp = ti_pkg_client_err(pkg->id, e);

    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_ALLOC_S);
    }
}

static void nodes__on_req_away(ti_stream_t * stream, ti_pkg_t * pkg)
{
    _Bool accepted;
    ex_t * e = ex_use();
    ti_pkg_t * resp = NULL;
    ti_node_t * node = stream->via.node;

    if (!node)
    {
        ex_set(e, EX_AUTH_ERROR,
                "got an away request from an unauthorized connection: `%s`",
                ti()->hostname);
        goto finish;
    }

    if (node->status < TI_NODE_STAT_SYNCHRONIZING)
    {
        ex_set(e, EX_NODE_ERROR,
                "node `%s` is not ready to handle away requests",
                ti()->hostname);
        goto finish;
    }

    accepted = ti_away_accept(node->id);

    assert (e->nr == 0);
    resp = ti_pkg_new(
            pkg->id,
            accepted ? TI_PROTO_NODE_RES_AWAY : TI_PROTO_NODE_ERR_AWAY,
            NULL,
            0);

finish:
    if (e->nr)
        resp = ti_pkg_client_err(pkg->id, e);

    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_ALLOC_S);
    }
}

static void nodes__on_req_query(ti_stream_t * stream, ti_pkg_t * pkg)
{
    uint64_t user_id;
    ex_t * e = ex_use();
    vec_t * access_;
    ti_user_t * user;
    qp_unpacker_t unpacker;
    ti_pkg_t * resp = NULL;
    ti_query_t * query = NULL;
    ti_node_t * other_node = stream->via.node;
    ti_node_t * this_node = ti()->node;
    qp_obj_t qp_user_id, qp_query;

    if (!other_node)
    {
        ex_set(e, EX_AUTH_ERROR,
                "got a forwarded query from an unauthorized connection: `%s`",
                ti()->hostname);
        goto finish;
    }

    if (this_node->status != TI_NODE_STAT_READY &&
        this_node->status != TI_NODE_STAT_AWAY_SOON)
    {
        ex_set(e, EX_NODE_ERROR,
                "node `%s` is not ready to handle query requests",
                ti()->hostname);
        goto finish;
    }

    qp_unpacker_init2(&unpacker, pkg->data, pkg->n, 0);

    if (    !qp_is_array(qp_next(&unpacker, NULL)) ||
            !qp_is_int(qp_next(&unpacker, &qp_user_id)) ||
            !qp_is_raw(qp_next(&unpacker, &qp_query)))
    {
        ex_set(e, EX_BAD_DATA,
                "invalid query request from "TI_NODE_ID" to "TI_NODE_ID,
                other_node->id, this_node->id);
        goto finish;
    }

    user_id = (uint64_t) qp_user_id.via.int64;
    user = ti_users_get_by_id(user_id);

    if (!user)
    {
        ex_set(e, EX_INDEX_ERROR,
                "cannot find "TI_USER_ID" which is used by a query from "
                TI_NODE_ID" to "TI_NODE_ID,
                user_id, other_node->id, this_node->id);
        goto finish;
    }

    query = ti_query_create(stream);
    if (!query)
    {
        ex_set_alloc(e);
        goto finish;
    }

    if (ti_query_unpack(query, pkg->id, qp_query.via.raw, qp_query.len, e))
        goto finish;

    access_ = query->target ? query->target->access : ti()->access;
    if (ti_access_check_err(access_, user, TI_AUTH_READ, e))
        goto finish;

    if (ti_query_parse(query, e))
        goto finish;

    if (ti_query_investigate(query, e))
        goto finish;

    if (ti_query_will_update(query))
    {
        if (ti_access_check_err(access_, user, TI_AUTH_MODIFY, e))
            goto finish;

        if (ti_events_create_new_event(query, e))
            goto finish;

        return;
    }

    ti_query_run(query);
    return;

finish:
    ti_query_destroy(query);

    if (e->nr)
        resp = ti_pkg_client_err(pkg->id, e);

    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_ALLOC_S);
    }
}

static void nodes__on_req_setup(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ti_pkg_t * resp;
    qpx_packer_t * packer;
    ti_node_t * node = stream->via.node;

    if (!node)
    {
        log_error(
                "got a setup request from an unauthorized connection: `%s`",
                ti_stream_name(stream));
        return;
    }

    packer = qpx_packer_create(48 + ti_.nodes->vec->n * 224, 3);
    if (!packer || ti_to_packer(&packer))
    {
        qpx_packer_destroy(packer);
        log_critical(EX_ALLOC_S);
        return;
    }

    resp = qpx_packer_pkg(packer, TI_PROTO_NODE_RES_SETUP);
    resp->id = pkg->id;

    if (ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_critical(EX_ALLOC_S);
    }
}

static void nodes__on_req_sync(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ex_t * e = ex_use();
    ti_pkg_t * resp = NULL;
    ti_node_t * node = stream->via.node;
    qp_unpacker_t unpacker;
    qp_obj_t qp_start, qp_until;
    uint64_t start, until;

    if (!node)
    {
        log_error(
                "got a sync request from an unauthorized connection: `%s`",
                ti_stream_name(stream));
        return;
    }

    if (ti()->node->status != TI_NODE_STAT_AWAY_SOON &&
        ti()->node->status != TI_NODE_STAT_AWAY)
    {
        log_error(
                "got a sync request from `%s` "
                "but this node is not in `away` mode",
                ti_stream_name(stream));
        ex_set(e, EX_NODE_ERROR,
                "node `%s` is not in `away` mode and therefore cannot handle "
                "sync requests",
                ti_name());
        goto finish;
    }

    qp_unpacker_init2(&unpacker, pkg->data, pkg->n, 0);

    if (!qp_is_array(qp_next(&unpacker, NULL)) ||
        !qp_is_int(qp_next(&unpacker, &qp_start)) ||
        !qp_is_int(qp_next(&unpacker, &qp_until)) ||
        qp_start.via.int64 < 0 ||
        qp_until.via.int64 < 0 ||
        (qp_until.via.int64 && qp_start.via.int64 >= qp_until.via.int64))
    {
        log_error(
                "got an invalid sync request from `%s`",
                ti_stream_name(stream));
        ex_set(e, EX_BAD_DATA, "invalid sync request");
        goto finish;
    }

    start = (uint64_t) qp_start.via.int64;
    until = (uint64_t) qp_until.via.int64;

    if (ti_away_syncer(stream, start, until))
    {
        ex_set_alloc(e);
        goto finish;
    }

    resp = ti_pkg_new(pkg->id, TI_PROTO_NODE_RES_SYNC, NULL, 0);

finish:
    if (e->nr)
        resp = ti_pkg_node_err(pkg->id, e);

    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_ALLOC_S);
    }
}

static void nodes__on_req_syncpart(
        ti_stream_t * stream,
        ti_pkg_t * pkg,
        nodes__part_cb part_cb)
{
    ex_t * e = ex_use();
    ti_pkg_t * resp = NULL;
    ti_node_t * node = stream->via.node;

    if (!node)
    {
        log_error(
            "got a `%s` from an unauthorized connection: `%s`",
            ti_proto_str(pkg->tp), ti_stream_name(stream));
        return;
    }

    if (ti()->node->status != TI_NODE_STAT_SYNCHRONIZING)
    {
        log_error(
                "got a `%s` from `%s` "
                "but this node is not in `synchronizing` mode",
                ti_proto_str(pkg->tp), ti_stream_name(stream));

        ex_set(e, EX_NODE_ERROR,
                "node `%s` is not in `synchronizing` mode and therefore "
                "cannot accept the request",
                ti_name());
        goto finish;
    }

    resp = part_cb(pkg, e);
    assert (!resp ^ !e->nr);

finish:
    if (e->nr)
        resp = ti_pkg_node_err(pkg->id, e);

    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_ALLOC_S);
    }
}

static void nodes__on_req_syncfdone(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ex_t * e = ex_use();
    ti_pkg_t * resp = NULL;
    ti_node_t * node = stream->via.node;

    if (!node)
    {
        log_error(
            "got a `%s` from an unauthorized connection: `%s`",
            ti_proto_str(pkg->tp), ti_stream_name(stream));
        return;
    }

    if (ti()->node->status != TI_NODE_STAT_SYNCHRONIZING)
    {
        log_error(
                "got a `%s` from `%s` "
                "but this node is not in `synchronizing` mode",
                ti_proto_str(pkg->tp), ti_stream_name(stream));
        ex_set(e, EX_NODE_ERROR,
                "node `%s` is not in `synchronizing` mode and therefore "
                "cannot accept the request",
                ti_name());
        goto finish;
    }


    ti_store_restore();
    resp = ti_pkg_new(pkg->id, TI_PROTO_NODE_RES_SYNCFDONE, NULL, 0);

finish:
    if (e->nr)
        resp = ti_pkg_node_err(pkg->id, e);

    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_ALLOC_S);
    }
}

static void nodes__on_req_syncadone(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ex_t * e = ex_use();
    ti_pkg_t * resp = NULL;
    ti_node_t * node = stream->via.node;

    if (!node)
    {
        log_error(
            "got a `%s` from an unauthorized connection: `%s`",
            ti_proto_str(pkg->tp), ti_stream_name(stream));
        return;
    }

    if (ti()->node->status != TI_NODE_STAT_SYNCHRONIZING)
    {
        log_error(
                "got a `%s` from `%s` "
                "but this node is not in `synchronizing` mode",
                ti_proto_str(pkg->tp), ti_stream_name(stream));
        ex_set(e, EX_NODE_ERROR,
                "node `%s` is not in `synchronizing` mode and therefore "
                "cannot accept the request",
                ti_name());
        goto finish;
    }

    ti_archive_load();

    resp = ti_pkg_new(pkg->id, TI_PROTO_NODE_RES_SYNCADONE, NULL, 0);

finish:
    if (e->nr)
        resp = ti_pkg_node_err(pkg->id, e);

    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_ALLOC_S);
    }
}

static void nodes__on_req_syncedone(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ex_t * e = ex_use();
    ti_pkg_t * resp = NULL;
    ti_node_t * node = stream->via.node;

    if (!node)
    {
        log_error(
            "got a `%s` from an unauthorized connection: `%s`",
            ti_proto_str(pkg->tp), ti_stream_name(stream));
        return;
    }

    if (ti()->node->status != TI_NODE_STAT_SYNCHRONIZING)
    {
        log_error(
                "got a `%s` from `%s` "
                "but this node is not in `synchronizing` mode",
                ti_proto_str(pkg->tp), ti_stream_name(stream));
        ex_set(e, EX_NODE_ERROR,
                "node `%s` is not in `synchronizing` mode and therefore "
                "cannot accept the request",
                ti_name());
        goto finish;
    }

    ti_sync_stop();
    ti_set_and_broadcast_node_status(TI_NODE_STAT_READY);

    resp = ti_pkg_new(pkg->id, TI_PROTO_NODE_RES_SYNCEDONE, NULL, 0);

finish:
    if (e->nr)
        resp = ti_pkg_node_err(pkg->id, e);

    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_ALLOC_S);
    }
}

static void nodes__on_event(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ti_node_t * other_node = stream->via.node;

    if (!other_node)
    {
        log_error(
                "got a `%s` from an unauthorized connection: `%s`",
                ti_proto_str(pkg->tp), ti_stream_name(stream));
        return;
    }

    ti_events_on_event(other_node, pkg);
}

static void nodes__on_info(ti_stream_t * stream, ti_pkg_t * pkg)
{
    qp_unpacker_t unpacker;
    ti_node_t * other_node = stream->via.node;

    if (!other_node)
    {
        log_error(
                "got a `%s` from an unauthorized connection: `%s`",
                ti_proto_str(pkg->tp), ti_stream_name(stream));
        return;
    }

    qp_unpacker_init2(&unpacker, pkg->data, pkg->n, 0);

    if (ti_node_info_from_unp(other_node, &unpacker))
    {
        log_error("invalid `%s` from `%s`",
                ti_proto_str(pkg->tp), ti_stream_name(stream));
        return;
    }

    log_debug("got a `%s` package from "TI_NODE_ID" (%s)",
            ti_proto_str(pkg->tp),
            other_node->id,
            ti_stream_name(stream));
}
