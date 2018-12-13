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
#include <ti.h>
#include <util/cryptx.h>
#include <util/qpx.h>

#define NODES__UV_BACKLOG 64

static ti_nodes_t * nodes;

static void nodes__tcp_connection(uv_stream_t * uvstream, int status);
static void nodes__on_req_connect(ti_stream_t * stream, ti_pkg_t * pkg);
static void nodes__on_req_query(ti_stream_t * stream, ti_pkg_t * pkg);
static void nodes__on_req_setup(ti_stream_t * stream, ti_pkg_t * pkg);
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

//void ti_nodes_close(void)
//{
//    for (vec_each(nodes->vec, ti_node_t, node))
//        ti_stream_close(node->stream);
//}

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
        if (node != ti()->node && (
                node->status == TI_NODE_STAT_SYNCHRONIZING ||
                node->status == TI_NODE_STAT_AWAY ||
                node->status == TI_NODE_STAT_AWAY_SOON ||
                node->status == TI_NODE_STAT_READY))
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
            qp_add_int64(*packer, node->port) ||
            qp_add_raw_from_str(*packer, node->addr) ||
            qp_add_raw(*packer, (const uchar *) node->secret, CRYPTX_SZ) ||
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
        char addr[INET6_ADDRSTRLEN];
        uint16_t port;
        ti_node_t * node;
        const char * secret;
        qp_res_t * qpflags, * qpport, * qpaddr, * qpsecret;
        qp_res_t * qparray = qpnodes->via.array->values + i;

        if (qparray->tp != QP_RES_ARRAY || qparray->via.array->n != 4)
            return -1;

        qpport = qparray->via.array->values + 0;
        qpaddr = qparray->via.array->values + 1;
        qpsecret = qparray->via.array->values + 2;
        qpflags = qparray->via.array->values + 3;

        if (    qpport->tp != QP_RES_INT64 ||
                qpaddr->tp != QP_RES_RAW ||
                qpaddr->via.raw->n >= INET6_ADDRSTRLEN ||
                qpsecret->tp != QP_RES_RAW ||
                qpsecret->via.raw->n != CRYPTX_SZ ||
                qpflags->tp != QP_RES_INT64)
            return -1;

        port = (uint16_t) qpport->via.int64;

        memcpy(addr, qpaddr->via.raw->data, qpaddr->via.raw->n);
        addr[qpaddr->via.raw->n] = '\0';

        secret = (const char *) qpsecret->via.raw->data;

        node = ti_nodes_new_node(port, addr, secret);
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

ti_node_t * ti_nodes_new_node(
        uint16_t port,
        const char * addr,
        const char * secret)
{
    ti_node_t * node = ti_node_create(nodes->vec->n, port, addr, secret);
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
    case TI_PROTO_CLIENT_RES_QUERY:
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
    case TI_PROTO_NODE_STATUS:
        nodes__on_status(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_QUERY:
        nodes__on_req_query(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_CONNECT:
        nodes__on_req_connect(stream, pkg);
        break;
    case TI_PROTO_NODE_REQ_SETUP:
        nodes__on_req_setup(stream, pkg);
        break;
    case TI_PROTO_NODE_RES_CONNECT:
    case TI_PROTO_NODE_RES_EVENT_ID:
    case TI_PROTO_NODE_RES_AWAY_ID:
    case TI_PROTO_NODE_RES_SETUP:
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
        qp_flags;

    uint8_t this_node_id, from_node_id;
    uint16_t from_node_port;
    ti_node_t * node, * this_node = ti()->node;
    char * min_ver = NULL;
    char * version = NULL;
    qpx_packer_t * packer;

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
            !qp_is_int(qp_next(&unpacker, &qp_next_thing_id)) ||
            !qp_is_int(qp_next(&unpacker, &qp_cevid)) ||
            !qp_is_int(qp_next(&unpacker, &qp_sevid)) ||
            !qp_is_int(qp_next(&unpacker, &qp_status)) ||
            !qp_is_int(qp_next(&unpacker, &qp_flags)))
    {
        log_error(
                "invalid connection request from `%s`",
                ti_stream_name(stream));
        goto failed;
    }

    this_node_id = (uint8_t) qp_this_node_id.via.int64;
    from_node_id = (uint8_t) qp_from_node_id.via.int64;
    from_node_port = (uint16_t) qp_from_node_port.via.int64;

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
            goto done;
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

        (void) ti_build_setup(this_node_id, from_node_id, stream);
        goto done;
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

    if (node->stream)
    {
        if (node->id > this_node->id)
        {
            log_warning("changing stream for "TI_NODE_ID" from `%s` to `%s",
                    node->id,
                    ti_stream_name(node->stream),
                    ti_stream_name(stream));
            /*
             * Do nothing with the `old` stream because it will be closed when
             * a response to the connection request is received.
             */
        }
        else
        {
            assert (node->id < this_node->id);
            log_warning(
                    "connection request from `%s` rejected since a connection "
                    "with "TI_NODE_ID" is already established",
                    ti_stream_name(stream),
                    node->id);
            goto failed;
        }
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

    (void) qp_add_array(&packer);
    (void) qp_add_int64(packer, this_node->next_thing_id);
    (void) qp_add_int64(packer, this_node->cevid);
    (void) qp_add_int64(packer, this_node->sevid);
    (void) qp_add_int64(packer, this_node->status);
    (void) qp_add_int64(packer, this_node->flags);
    (void) qp_close_array(packer);

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

static void nodes__on_req_query(ti_stream_t * stream, ti_pkg_t * pkg)
{
    uint64_t user_id;
    ex_t * e = ex_use();
    ti_pkg_t * resp;
    vec_t * access_;
    ti_user_t * user;
    ti_query_t * query;
    qp_unpacker_t unpacker;
    ti_node_t * node = stream->via.node;
    qp_obj_t qp_user_id, qp_query;

    if (!node)
    {
        ex_set(e, EX_AUTH_ERROR,
                "got a forwarded query from an unauthorized connection: `%s`",
                ti()->hostname);
        goto finish;
    }

    if (node->status != TI_NODE_STAT_READY)
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
                node->id, ti()->node->id);
        goto finish;
    }

    user_id = (uint64_t) qp_user_id.via.int64;
    user = ti_users_get_by_id(user_id);

    if (!user)
    {
        ex_set(e, EX_INDEX_ERROR,
                "cannot find "TI_USER_ID" which is used by a query from "
                TI_NODE_ID" to "TI_NODE_ID,
                user_id, node->id, ti()->node->id);
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

static void nodes__on_status(ti_stream_t * stream, ti_pkg_t * pkg)
{
    qp_unpacker_t unpacker;
    ti_node_t * node = stream->via.node;
    qp_obj_t qp_next_thing_id, qp_cevid, qp_sevid, qp_status, qp_flags;

    if (!node)
    {
        log_error(
                "got a status update from an unauthorized connection: `%s`",
                ti_stream_name(stream));
        return;
    }

    qp_unpacker_init2(&unpacker, pkg->data, pkg->n, 0);

    if (    !qp_is_array(qp_next(&unpacker, NULL)) ||
            !qp_is_int(qp_next(&unpacker, &qp_next_thing_id)) ||
            !qp_is_int(qp_next(&unpacker, &qp_cevid)) ||
            !qp_is_int(qp_next(&unpacker, &qp_sevid)) ||
            !qp_is_int(qp_next(&unpacker, &qp_status)) ||
            !qp_is_int(qp_next(&unpacker, &qp_flags)))
    {
        log_error("invalid status package from `%s`", ti_stream_name(stream));
        return;
    }

    node->next_thing_id = (uint64_t) qp_next_thing_id.via.int64;
    node->cevid = (uint64_t) qp_cevid.via.int64;
    node->sevid = (uint64_t) qp_sevid.via.int64;
    node->status = (uint8_t) qp_status.via.int64;
    node->flags = (uint8_t) qp_status.via.int64;

    log_debug("got a status update for "TI_NODE_ID" (%s)",
            node->id,
            ti_stream_name(stream));
}
