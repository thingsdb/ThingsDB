/*
 * clients.c
 */
#include <ti/node.h>
#include <ti/write.h>
#include <ti/proto.h>
#include <stdbool.h>
#include <assert.h>
#include <ti/clients.h>
#include <ti/users.h>
#include <ti.h>
#include <ti/query.h>
#include <ti/wareq.h>
#include <ti/auth.h>
#include <ti/ex.h>
#include <ti/access.h>
#include <ti/req.h>
#include <ti/fwd.h>
#include <ti/write.h>
#include <util/qpx.h>


#define CLIENTS__UV_BACKLOG 64

static ti_clients_t * clients;

static void clients__tcp_connection(uv_stream_t * uvstream, int status);
static void clients__pipe_connection(uv_stream_t * uvstream, int status);
static void clients__pkg_cb(ti_stream_t * stream, ti_pkg_t * pkg);
static void clients__on_ping(ti_stream_t * stream, ti_pkg_t * pkg);
static void clients__on_auth(ti_stream_t * stream, ti_pkg_t * pkg);
static void clients__on_query(ti_stream_t * stream, ti_pkg_t * pkg);
static void clients__on_watch(ti_stream_t * stream, ti_pkg_t * pkg);
static void clients__write_cb(ti_write_t * req, ex_enum status);
static int clients__fwd_query(
        ti_node_t * to_node,
        ti_stream_t * src_stream,
        ti_pkg_t * orig_pkg);
static void clients__fwd_query_cb(ti_req_t * req, ex_enum status);


int ti_clients_create(void)
{
    clients = malloc(sizeof(ti_clients_t));
    if (!clients)
        return -1;

    /* make sure data is set to null, we use this on close */
    clients->tcp.data = NULL;
    clients->pipe.data = NULL;

    ti()->clients = clients;

    return 0;
}

void ti_clients_destroy(void)
{
    free(clients);
    clients = ti()->clients = NULL;
}

int ti_clients_listen(void)
{
    int rc;
    ti_cfg_t * cfg = ti()->cfg;
    struct sockaddr_storage addr = {0};
    _Bool is_ipv6 = false;
    char * ip;

    uv_tcp_init(ti()->loop, &clients->tcp);
    uv_pipe_init(ti()->loop, &clients->pipe, 0);

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
        uv_ip6_addr(ip, cfg->client_port, (struct sockaddr_in6 *) &addr);
    }
    else
    {
        uv_ip4_addr(ip, cfg->client_port, (struct sockaddr_in *) &addr);
    }

    if ((rc = uv_tcp_bind(
            &clients->tcp,
            (const struct sockaddr *) &addr,
            (cfg->ip_support == AF_INET6) ?
                    UV_TCP_IPV6ONLY : 0)) ||
        (rc = uv_listen(
            (uv_stream_t *) &clients->tcp,
            CLIENTS__UV_BACKLOG,
            clients__tcp_connection)))
    {
        log_error(
                "error listening for client connections on TCP port %d: `%s`",
                cfg->client_port,
                uv_strerror(rc));
        return -1;
    }

    log_info("start listening for client connections on TCP port %d",
            cfg->client_port);

    if (!cfg->pipe_client_name)
        return 0;

    if ((rc = uv_pipe_bind(&clients->pipe, cfg->pipe_client_name)) ||
        (rc = uv_listen(
                (uv_stream_t *) &clients->pipe,
                CLIENTS__UV_BACKLOG,
                clients__pipe_connection)))
    {
        log_error(
            "error listening for client connections on named pipe `%s`: `%s`",
            cfg->pipe_client_name,
            uv_strerror(rc));
        return -1;
    }

    log_info("start listening for client connections on named pipe `%s`",
            cfg->pipe_client_name);

    return 0;
}

int ti_clients_write(ti_stream_t * stream, ti_pkg_t * pkg)
{
    return ti_write(stream, pkg, NULL, clients__write_cb);
}

static void clients__tcp_connection(uv_stream_t * uvstream, int status)
{
    ti_stream_t * stream;

    if (status < 0)
    {
        log_error("client connection error: `%s`", uv_strerror(status));
        return;
    }

    log_debug("received a TCP client connection");

    stream = ti_stream_create(TI_STREAM_TCP_IN_CLIENT, &clients__pkg_cb);

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

static void clients__pipe_connection(uv_stream_t * uvstream, int status)
{
    ti_stream_t * stream;

    if (status < 0)
    {
        log_error("client connection error: `%s`", uv_strerror(status));
        return;
    }

    log_debug("received a PIPE client connection");

    stream = ti_stream_create(TI_STREAM_PIPE_IN_CLIENT, &clients__pkg_cb);

    if (!stream)
        return;

    uv_pipe_init(ti()->loop, (uv_pipe_t *) &stream->uvstream, 0);
    if (uv_accept(uvstream, &stream->uvstream) == 0)
    {
        uv_read_start(&stream->uvstream, ti_stream_alloc_buf, ti_stream_on_data);
    }
    else
    {
        ti_stream_drop(stream);
    }
}

static void clients__pkg_cb(ti_stream_t * stream, ti_pkg_t * pkg)
{
    switch (pkg->tp)
    {
    case TI_PROTO_CLIENT_REQ_PING:
        clients__on_ping(stream, pkg);
        break;
    case TI_PROTO_CLIENT_REQ_AUTH:
        clients__on_auth(stream, pkg);
        break;
    case TI_PROTO_CLIENT_REQ_QUERY:
        clients__on_query(stream, pkg);
        break;
    case TI_PROTO_CLIENT_REQ_WATCH:
        clients__on_watch(stream, pkg);
        break;
    default:
        log_error(
                "unexpected package type `%u` from `%s`)",
                pkg->tp,
                ti_stream_name(stream));
    }
}

static void clients__on_ping(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ti_pkg_t * resp = ti_pkg_new(pkg->id, TI_PROTO_CLIENT_RES_PING, NULL, 0);
    if (!resp || ti_clients_write(stream, resp))
    {
        free(resp);
        log_error(EX_ALLOC_S);
    }
}

static void clients__on_auth(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ti_pkg_t * resp;
    qp_unpacker_t unpacker;
    qp_obj_t name, pass;
    ti_user_t * user;
    ex_t * e = ex_use();
    qp_unpacker_init(&unpacker, pkg->data, pkg->n);
    if (    !qp_is_array(qp_next(&unpacker, NULL)) ||
            !qp_is_raw(qp_next(&unpacker, &name)) ||
            !qp_is_raw(qp_next(&unpacker, &pass)))
    {
        ex_set(e, EX_BAD_DATA, "invalid authentication request");
        log_error("%s from `%s`", e->msg, ti_stream_name(stream));
        resp = ti_pkg_err(pkg->id, e);
        goto finish;
    }
    user = ti_users_auth(&name, &pass, e);
    if (e->nr)
    {
        assert (user == NULL);
        log_warning(
                "authentication failed `%s` from `%s`)",
                e->msg,
                ti_stream_name(stream));
        resp = ti_pkg_err(pkg->id, e);
    }
    else
    {
        assert (user != NULL);
        if (stream->via.user)
            ti_user_drop(stream->via.user);
        stream->via.user = ti_grab(user);
        resp = ti_pkg_new(pkg->id, TI_PROTO_CLIENT_RES_AUTH, NULL, 0);
    }

finish:
    if (!resp || ti_clients_write(stream, resp))
    {
        free(resp);
        log_error(EX_ALLOC_S);
    }
}

static void clients__on_query(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ti_pkg_t * resp = NULL;
    ti_query_t * query = NULL;
    ex_t * e = ex_use();
    ti_node_t * node = ti()->node;
    ti_user_t * user = stream->via.user;
    vec_t * access_;

    if (!user)
    {
        ex_set(e, EX_AUTH_ERROR, "connection is not authenticated");
        goto finish;
    }

    if (node->status <= TI_NODE_STAT_CONNECTING)
    {
        ex_set(e, EX_NODE_ERROR,
                "node `%s` is not ready to handle query requests",
                ti()->hostname);
        goto finish;
    }

    if (node->status < TI_NODE_STAT_READY)
    {

        node = ti_nodes_random_ready_node();
        if (!node)
        {
            ex_set(e, EX_NODE_ERROR,
                    "node `%s` is unable to handle query requests",
                    ti()->hostname);
            goto finish;
        }

        if (clients__fwd_query(node, stream, pkg))
        {
            ex_set_internal(e);
            goto finish;
        }
        /* the response to the client will be done on a callback on the
         * query forward so we simply return;
         */
        return;
    }

    query = ti_query_create(stream);
    if (!query)
    {
        ex_set_alloc(e);
        goto finish;
    }

    if (ti_query_unpack(query, pkg, e))
        goto finish;

    access_ = query->target ? query->target->access : ti()->access;
    if (!ti_access_check(access_, user, TI_AUTH_ACCESS|TI_AUTH_READ))
    {
        ex_set(e, EX_FORBIDDEN,
                "access denied (requires `%s`)",
                ti_auth_mask_to_str(TI_AUTH_ACCESS|TI_AUTH_READ));
        goto finish;
    }

    if (ti_query_parse(query, e))
        goto finish;

    if (ti_query_investigate(query, e))
        goto finish;

    if (ti_query_will_update(query))
    {
        if (!ti_access_check(access_, user, TI_AUTH_MODIFY))
        {
            ex_set(e, EX_FORBIDDEN,
                    "access denied (requires `%s`)",
                    ti_auth_mask_to_str(TI_AUTH_MODIFY));
            goto finish;
        }

        if (ti_events_create_new_event(query, e))
            goto finish;

        return;
    }

    ti_query_run(query);
    return;

finish:
    ti_query_destroy(query);

    if (e->nr)
        resp = ti_pkg_err(pkg->id, e);

    if (!resp || ti_clients_write(stream, resp))
    {
        free(resp);
        log_error(EX_ALLOC_S);
    }
}

static void clients__on_watch(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ti_node_t * node = ti()->node;
    ti_user_t * user = stream->via.user;
    ti_wareq_t * wareq = NULL;
    ex_t * e = ex_use();
    vec_t * access_;
    ti_pkg_t * resp = NULL;

    if (!user)
    {
        ex_set(e, EX_AUTH_ERROR, "connection is not authenticated");
        goto finish;
    }

    if (node->status <= TI_NODE_STAT_CONNECTING)
    {
        ex_set(e, EX_NODE_ERROR,
                "node `%s` is not ready to handle query requests",
                ti()->hostname);
        goto finish;
    }

    wareq = ti_wareq_create(stream);
    if (!wareq)
    {
        ex_set_alloc(e);
        goto finish;
    }

    if (ti_wareq_unpack(wareq, pkg, e))
        goto finish;

    access_ = wareq->target ? wareq->target->access : ti()->access;
    if (!ti_access_check(access_, user, TI_AUTH_WATCH))
    {
        ex_set(e, EX_FORBIDDEN,
                "access denied (requires `%s`)",
                ti_auth_mask_to_str(TI_AUTH_WATCH));
        goto finish;
    }

    resp = ti_pkg_new(pkg->id, TI_PROTO_CLIENT_RES_WATCH, NULL, 0);
    if (!resp)
        ex_set_alloc(e);

finish:
    if (e->nr)
    {
        ti_wareq_destroy(wareq);
        resp = ti_pkg_err(pkg->id, e);
    }

    if (!resp || ti_clients_write(stream, resp))
    {
        free(resp);
        log_error(EX_ALLOC_S);
    }
}

static void clients__write_cb(ti_write_t * req, ex_enum status)
{
    (void)(status);     /* errors are logged by ti__write_cb() */
    free(req->pkg);
    ti_write_destroy(req);
}

static int clients__fwd_query(
        ti_node_t * to_node,
        ti_stream_t * src_stream,
        ti_pkg_t * orig_pkg)
{
    qpx_packer_t * packer;
    ti_fwd_t * fwd;
    ti_pkg_t * pkg_req;

    packer = qpx_packer_create(orig_pkg->n + 19, 1);
    if (!packer)
        goto fail0;

    fwd = ti_fwd_create(orig_pkg->id, src_stream);
    if (!fwd)
        goto fail1;

    (void) qp_add_array(&packer);
    (void) qp_add_int64(packer, src_stream->via.user->id);
    (void) qp_add_raw(packer, orig_pkg->data, orig_pkg->n);
    (void) qp_close_array(packer);

    /* this cannot fail */
    pkg_req = qpx_packer_pkg(packer, TI_PROTO_NODE_REQ_QUERY);

    if (ti_req_create(
            to_node->stream,
            pkg_req,
            TI_PROTO_NODE_REQ_QUERY_TIMEOUT,
            clients__fwd_query_cb,
            fwd))
        goto fail1;

    return 0;

fail1:
    ti_fwd_destroy(fwd);
    qp_packer_destroy(packer);
fail0:
    return -1;
}

static void clients__fwd_query_cb(ti_req_t * req, ex_enum status)
{
    ti_pkg_t * resp;
    ti_fwd_t * fwd = req->data;
    if (status)
    {
        ex_t * e = ex_use();
        ex_set(e, status, ex_str(status));
        resp = ti_pkg_err(fwd->orig_pkg_id, e);
        if (!resp)
            log_error(EX_ALLOC_S);
        goto finish;
    }

    resp = req->pkg_res;
    req->pkg_res = NULL;

    resp->id = fwd->orig_pkg_id;
    resp->tp = resp->tp - 0x80;
    resp->ntp = resp->tp ^ 255;

finish:
    free(req->pkg_res);
    if (resp && ti_clients_write(fwd->stream, resp))
    {
        free(resp);
        log_error(EX_ALLOC_S);
    }

    ti_fwd_destroy(fwd);
    ti_req_destroy(req);
}
