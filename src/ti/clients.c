/*
 * ti/clients.c
 */
#include <assert.h>
#include <ex.h>
#include <stdbool.h>
#include <doc.h>
#include <ti.h>
#include <ti/access.h>
#include <ti/auth.h>
#include <ti/clients.h>
#include <ti/fwd.h>
#include <ti/node.h>
#include <ti/proto.h>
#include <ti/qcache.h>
#include <ti/query.h>
#include <ti/query.inline.h>
#include <ti/req.h>
#include <ti/room.h>
#include <ti/scope.h>
#include <ti/thing.h>
#include <ti/users.h>
#include <ti/watch.h>
#include <ti/write.h>
#include <ti/write.h>
#include <util/mpack.h>


#define CLIENTS__UV_BACKLOG 64

static ti_clients_t * clients;
static ti_clients_t clients_;

static void clients__fwd_cb(ti_req_t * req, ex_enum status)
{
    ti_pkg_t * resp;
    ti_fwd_t * fwd = req->data;
    if (status)
    {
        ex_t e = {0};
        ex_set(&e, status, ex_str(status));
        resp = ti_pkg_client_err(fwd->orig_pkg_id, &e);
        if (!resp)
            log_error(EX_MEMORY_S);
        goto finish;
    }

    resp = ti_pkg_dup(req->pkg_res);
    if (resp)
        resp->id = fwd->orig_pkg_id;
    else
        log_error(EX_MEMORY_S);

finish:
    if (resp && ti_stream_write_pkg(fwd->stream, resp))
    {
        free(resp);
        log_error(EX_MEMORY_S);
    }

    ti_fwd_destroy(fwd);
    ti_req_destroy(req);
}

_Bool ti_clients_is_fwd_req(ti_req_t * req)
{
    return req->cb_ == clients__fwd_cb;
}

static int clients__fwd(
        ti_node_t * to_node,
        ti_stream_t * src_stream,
        ti_pkg_t * orig_pkg,
        ti_proto_enum_t proto)
{
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    ti_fwd_t * fwd;
    ti_pkg_t * pkg_req = NULL;

    fwd = ti_fwd_create(orig_pkg->id, src_stream);
    if (!fwd)
        goto fail0;

    if (mp_sbuffer_alloc_init(&buffer, orig_pkg->n + 48, sizeof(ti_pkg_t)))
        goto fail1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    /* some extra size for setting the raw size + user_id */
    msgpack_pack_array(&pk, 2);
    msgpack_pack_uint64(&pk, src_stream->via.user->id);
    mp_pack_bin(&pk, orig_pkg->data, orig_pkg->n);

    pkg_req = (ti_pkg_t *) buffer.data;
    pkg_init(pkg_req, 0, proto, buffer.size);

    if (ti_req_create(
            to_node->stream,
            pkg_req,
            TI_PROTO_NODE_REQ_FWD_TIMEOUT,
            clients__fwd_cb,
            fwd))
        goto fail1;

    return 0;

fail1:
    free(pkg_req);
    ti_fwd_destroy(fwd);
fail0:
    return -1;
}

static void clients__on_deprecated(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ti_pkg_t * resp = ti_pkg_new(pkg->id, TI_PROTO_CLIENT_RES_OK, NULL, 0);
    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_MEMORY_S);
    }
}

static void clients__on_ping(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ti_pkg_t * resp = ti_pkg_new(pkg->id, TI_PROTO_CLIENT_RES_PONG, NULL, 0);
    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_MEMORY_S);
    }
}

static void clients__on_auth(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ex_t e = {0};
    ti_pkg_t * resp;
    mp_unp_t up;
    mp_obj_t mp_name, mp_pass, mp_token;
    ti_user_t * user;

    mp_unp_init(&up, pkg->data, pkg->n);

    if (mp_next(&up, &mp_token) <= 0 || (mp_token.tp != MP_STR && (
        mp_token.tp != MP_ARR || mp_token.via.sz != 2 ||
        mp_next(&up, &mp_name) != MP_STR ||
        mp_next(&up, &mp_pass) != MP_STR)))
    {
        ex_set(&e, EX_BAD_DATA, "invalid authentication request");
        log_error("%s from `%s`", e.msg, ti_stream_name(stream));
        resp = ti_pkg_client_err(pkg->id, &e);
        goto finish;
    }

    user = mp_token.tp == MP_STR
            ? ti_users_auth_by_token(&mp_token, &e)
            : ti_users_auth(&mp_name, &mp_pass, &e);
    if (e.nr)
    {
        assert (user == NULL);
        log_warning(
                "authentication failed `%s` from `%s`)",
                e.msg,
                ti_stream_name(stream));
        resp = ti_pkg_client_err(pkg->id, &e);
    }
    else
    {
        assert (user != NULL);
        /*
         * If below fails, it's an allocation error and the only side effect
         * will be that the steam will not re
         */
        (void) ti_room_join(ti.room0, stream);

        ti_stream_set_user(stream, user);
        resp = ti_pkg_new(pkg->id, TI_PROTO_CLIENT_RES_OK, NULL, 0);
    }

finish:
    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_MEMORY_S);
    }
}

static inline int clients__check(ti_user_t * user, ex_t * e)
{
    if (!user)
    {
        ex_set(e, EX_AUTH_ERROR, "connection is not authenticated");
    }
    else if (ti.node->status <= TI_NODE_STAT_BUILDING)
    {
        ex_set(e, EX_NODE_ERROR,
                TI_NODE_ID" is not ready to handle client requests",
                ti.node->id);
    }
    return e->nr;
}

static void clients__on_query(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ex_t e = {0};
    mp_obj_t mp_query;
    mp_unp_t up;
    ti_pkg_t * resp = NULL;
    ti_query_t * query = NULL;
    ti_node_t * this_node = ti.node, * other_node;
    ti_user_t * user = stream->via.user;
    vec_t * access_;
    ti_scope_t scope;

    mp_unp_init(&up, pkg->data, pkg->n);

    if (clients__check(user, &e) || ti_scope_init_from_up(&scope, &up, &e))
        goto finish;

    if (scope.tp == TI_SCOPE_NODE)
    {
        if (scope.via.node_id == this_node->id)
            goto query;

        other_node = ti_nodes_node_by_id(scope.via.node_id);
        if (!other_node)
        {
            ex_set(&e, EX_LOOKUP_ERROR, TI_NODE_ID" does not exist",
                    scope.via.node_id);
            goto finish;
        }

        if (other_node->status <= TI_NODE_STAT_BUILDING)
        {
            ex_set(&e, EX_LOOKUP_ERROR,
                    TI_NODE_ID" is not able to handle this request",
                    scope.via.node_id);
            goto finish;
        }

        if (clients__fwd(other_node, stream, pkg, TI_PROTO_NODE_REQ_QUERY))
        {
            ex_set_internal(&e);
            goto finish;
        }
        /* the response to the client will be handled by a callback on the
         * query forward request so we simply return;
         */
        return;
    }

    if (this_node->status < TI_NODE_STAT_READY &&
        this_node->status != TI_NODE_STAT_SHUTTING_DOWN)
    {
        other_node = ti_nodes_random_ready_node();
        if (!other_node)
        {
            ti_nodes_set_not_ready_err(&e);
            goto finish;
        }

        if (clients__fwd(other_node, stream, pkg, TI_PROTO_NODE_REQ_QUERY))
        {
            ex_set_internal(&e);
            goto finish;
        }

        /* the response to the client will be handled by a callback on the
         * query forward request so we simply return;
         */
        return;
    }

query:

    if (mp_next(&up, &mp_query) != MP_STR)
    {
        ex_set(&e, EX_TYPE_ERROR,
            "expecting the code in a `query` request to be of type `string`"
            DOC_SOCKET_QUERY);
        goto finish;
    }

    query = ti_scope_is_collection(&scope)
        ? ti_qcache_get_query(mp_query.via.str.data, mp_query.via.str.n, 0)
        : ti_query_create(0);

    if (!query)
    {
        ex_set_mem(&e);
        goto finish;
    }

    query->via.stream = ti_grab(stream);
    query->user = ti_grab(user);
    query->pkg_id = pkg->id;

    if (ti_query_apply_scope(query, &scope, &e) ||
        ti_query_unpack_args(query, &up, &e))
        goto finish;

    access_ = ti_query_access(query);
    assert (access_);

    if (ti_access_check_err(access_, query->user, TI_AUTH_QUERY, &e) ||
        ti_query_parse(query, mp_query.via.str.data, mp_query.via.str.n, &e))
        goto finish;

    if (ti_query_wse(query))
    {
        assert (scope.tp != TI_SCOPE_NODE);

        if (ti_access_check_err(access_, query->user, TI_AUTH_CHANGE, &e) ||
            ti_changes_create_new_change(query, &e))
            goto finish;

        return;
    }

    ti_query_run_parseres(query);
    return;

finish:
    ti_query_destroy_or_return(query);

    if (e.nr)
    {
        ++ti.counters->queries_with_error;
        resp = ti_pkg_client_err(pkg->id, &e);
    }

    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_MEMORY_S);
    }
}

static void clients__on_join(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ex_t e = {0};
    ti_user_t * user = stream->via.user;
    ti_pkg_t * resp = NULL;
    ti_scope_t scope;
    ti_collection_t * collection;

    if (clients__check(user, &e) || ti_scope_init_pkg(&scope, pkg, &e))
        goto on_error;

    if (ti.node->status <= TI_NODE_STAT_SYNCHRONIZING)
    {
        ex_set(&e, EX_NODE_ERROR,
                TI_NODE_ID" is not ready to handle join requests",
                ti.node->id);
        goto on_error;
    }

    collection = ti_scope_get_collection(&scope, &e);
    if (!collection ||
        ti_access_check_err(collection->access, user, TI_AUTH_JOIN, &e))
        goto on_error;

    resp = ti_collection_join_rooms(collection, stream, pkg, &e);
    if (resp)
        goto done;

on_error:
    resp = ti_pkg_client_err(pkg->id, &e);
    if (!resp)
    {
        log_error(EX_MEMORY_S);
        return;
    }

done:
    if (ti_stream_write_pkg(stream, resp))
    {
        /* serious error, join cannot continue */
        free(resp);
        log_error(EX_MEMORY_S);
    }
}

static void clients__on_leave(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ex_t e = {0};
    ti_user_t * user = stream->via.user;
    ti_pkg_t * resp = NULL;
    ti_scope_t scope;
    ti_collection_t * collection;

    if (clients__check(user, &e) || ti_scope_init_pkg(&scope, pkg, &e))
        goto on_error;

    if (ti.node->status <= TI_NODE_STAT_SYNCHRONIZING)
    {
        ex_set(&e, EX_NODE_ERROR,
                TI_NODE_ID" is not ready to handle leave requests",
                ti.node->id);
        goto on_error;
    }

    collection = ti_scope_get_collection(&scope, &e);
    if (!collection ||
        ti_access_check_err(collection->access, user, TI_AUTH_JOIN, &e))
        goto on_error;

    resp = ti_collection_leave_rooms(collection, stream, pkg, &e);
    if (resp)
        goto done;

on_error:
    resp = ti_pkg_client_err(pkg->id, &e);
    if (!resp)
    {
        log_error(EX_MEMORY_S);
        return;
    }

done:
    if (ti_stream_write_pkg(stream, resp))
    {
        /* serious error, join cannot continue */
        free(resp);
        log_error(EX_MEMORY_S);
    }
}

static void clients__on_run(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ex_t e = {0};
    ti_user_t * user = stream->via.user;
    ti_node_t * this_node = ti.node;
    ti_pkg_t * resp = NULL;
    vec_t * access_;
    ti_query_t * query = NULL;
    ti_scope_t scope;

    if (clients__check(user, &e) || ti_scope_init_pkg(&scope, pkg, &e))
        goto finish;

    if (this_node->status < TI_NODE_STAT_READY &&
        this_node->status != TI_NODE_STAT_SHUTTING_DOWN)
    {
        ti_node_t * other_node = ti_nodes_random_ready_node();
        if (!other_node)
        {
            ti_nodes_set_not_ready_err(&e);
            goto finish;
        }

        if (clients__fwd(other_node, stream, pkg, TI_PROTO_NODE_REQ_RUN))
        {
            ex_set_internal(&e);
            goto finish;
        }
        /*
         * The response to the client will be handled by a callback on the
         * query forward request so we simply return;
         */
        return;
    }

    query = ti_query_create(0);
    if (!query)
    {
        ex_set_mem(&e);
        goto finish;
    }

    query->via.stream = ti_grab(stream);
    query->user = ti_grab(user);

    if (ti_query_unp_run(query, &scope, pkg->id, pkg->data, pkg->n, &e))
        goto finish;

    access_ = ti_query_access(query);
    assert (access_);

    if (ti_access_check_err(access_, query->user, TI_AUTH_RUN, &e))
        goto finish;

    if (ti_query_wse(query))
    {
        if (ti_access_check_err(access_, query->user, TI_AUTH_CHANGE, &e) ||
            ti_changes_create_new_change(query, &e))
            goto finish;
        return;
    }

    ti_query_run_procedure(query);
    return;

finish:
    ti_query_destroy(query);

    if (e.nr)
    {
        ++ti.counters->queries_with_error;
        resp = ti_pkg_client_err(pkg->id, &e);
    }

    if (!resp || ti_stream_write_pkg(stream, resp))
    {
        free(resp);
        log_error(EX_MEMORY_S);
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
    case TI_PROTO_CLIENT_REQ_RUN:
        clients__on_run(stream, pkg);
        break;
    case TI_PROTO_CLIENT_REQ_JOIN:
        clients__on_join(stream, pkg);
        break;
    case TI_PROTO_CLIENT_REQ_LEAVE:
        clients__on_leave(stream, pkg);
        break;
    case _TI_PROTO_CLIENT_DEP_35:  /* deprecated watch request */
    case _TI_PROTO_CLIENT_DEP_36:  /* deprecated watch request */
        clients__on_deprecated(stream, pkg);
        break;
    default:
        log_error(
                "unexpected package type `%u` from `%s`)",
                pkg->tp,
                ti_stream_name(stream));
    }
}

static void clients__tcp_connection(uv_stream_t * uvstream, int status)
{
    int rc;
    ti_stream_t * stream;

    if (status < 0)
    {
        log_error("client connection error: `%s`", uv_strerror(status));
        return;
    }
    if (ti.node->status == TI_NODE_STAT_SHUTTING_DOWN)
    {
        log_error(
                "ignore client connection request; "
                "node has status: %s", ti_node_status_str(ti.node->status));
        return;
    }

    log_debug("received a TCP client connection");

    stream = ti_stream_create(TI_STREAM_TCP_IN_CLIENT, &clients__pkg_cb);
    if (!stream)
    {
        log_critical(EX_MEMORY_S);
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
    log_error("cannot read client TCP stream: `%s`", uv_strerror(rc));
    ti_stream_drop(stream);
}

static void clients__pipe_connection(uv_stream_t * uvstream, int status)
{
    int rc;
    ti_stream_t * stream;

    if (status < 0)
    {
        log_error("client connection error: `%s`", uv_strerror(status));
        return;
    }
    if (ti.node->status == TI_NODE_STAT_SHUTTING_DOWN)
    {
        log_error(
                "ignore client connection request; "
                "node has status: %s", ti_node_status_str(ti.node->status));
        return;
    }

    log_debug("received a PIPE client connection");

    stream = ti_stream_create(TI_STREAM_PIPE_IN_CLIENT, &clients__pkg_cb);
    if (!stream)
    {
        log_critical(EX_MEMORY_S);
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
    log_error("cannot read client PIPE stream: `%s`", uv_strerror(rc));
    ti_stream_drop(stream);
}

int ti_clients_create(void)
{
    clients = &clients_;

    /* make sure data is set to null, we use this on close */
    clients->tcp.data = NULL;
    clients->pipe.data = NULL;

    ti.clients = clients;

    return 0;
}

void ti_clients_destroy(void)
{
    clients = ti.clients = NULL;
}

int ti_clients_listen(void)
{
    int rc;
    ti_cfg_t * cfg = ti.cfg;
    struct sockaddr_storage addr = {0};
    _Bool is_ipv6 = false;
    char * ip;

    uv_tcp_init(ti.loop, &clients->tcp);
    uv_pipe_init(ti.loop, &clients->pipe, 0);

    if (cfg->bind_client_addr != NULL)
    {
        struct in6_addr sa6;
        if (inet_pton(AF_INET6, cfg->bind_client_addr, &sa6))
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
        if (uv_ip6_addr(ip, cfg->client_port, (struct sockaddr_in6 *) &addr))
        {
            log_error(
                    "cannot create IPv6 address from `[%s]:%d`",
                    ip, cfg->client_port);
            return -1;
        }
    }
    else
    {
        if (uv_ip4_addr(ip, cfg->client_port, (struct sockaddr_in *) &addr))
        {
            log_error(
                    "cannot create IPv4 address from `%s:%d`",
                    ip, cfg->client_port);
            return -1;
        }
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
