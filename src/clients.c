/*
 * clients.c
 */
#include <clients.h>
#include <thingsdb.h>
#include <ti/node.h>
#include <stdbool.h>

static thingsdb_clients_t * clients;

static void thingsdb__clients_pipe_connection(uv_stream_t * uvstream, int status);
static void thingsdb__clients_pkg_cb(ti_stream_t * stream, ti_pkg_t * pkg);
static void thingsdb__clients_on_ping(ti_stream_t * stream, ti_pkg_t * pkg);
static void thingsdb__clients_on_auth(ti_stream_t * stream, ti_pkg_t * pkg);
static void thingsdb__clients_write_cb(ti_write_t * req, ex_e status);

int thingsdb_clients_create(void)
{
    clients = malloc(sizeof(thingsdb_clients_t));
    if (!clients)
        return -1;

    thingsdb_get()->clients = clients;

    return -(clients == NULL);
}

void thingsdb_clients_destroy(void)
{
    free(clients);
    clients = thingsdb_get()->clients = NULL;
}

int thingsdb_clients_listen(void)
{
    int rc;
    thingsdb_t * thingsdb = thingsdb_get();
    ti_cfg_t * cfg = thingsdb->cfg;
    struct sockaddr_storage addr;
    _Bool is_ipv6 = false;
    char * ip;

    uv_tcp_init(thingsdb->loop, &clients->tcp);
    uv_pipe_init(thingsdb->loop, &clients->pipe, 0);

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
            THINGSDB_MAX_NODES,
            thingsdb__clients_tcp_connection)))
    {
        log_error("error listening for TCP clients: `%s`", uv_strerror(rc));
        return -1;
    }

    log_info("start listening for TCP clients on port %d", cfg->client_port);

    if (!cfg->pipe_support)
        return 0;

    if ((rc = uv_pipe_bind(&clients->pipe, cfg->pipe_client_name)) ||
        (rc = uv_listen(
                (uv_stream_t *) &clients->pipe,
                THINGSDB_MAX_NODES,
                thingsdb__clients_pipe_connection)))
    {
        log_error("error listening for PIPE clients: `%s`", uv_strerror(rc));
        return -1;
    }

    log_info("start listening for PIPE clients connections on `%s`",
            cfg->pipe_client_name);

    return 0;
}

int thingsdb_clients_write(ti_stream_t * stream, ti_pkg_t * pkg)
{
    return ti_write(stream, pkg, NULL, thingsdb__clients_write_cb);
}

static void thingsdb__clients_tcp_connection(uv_stream_t * uvstream, int status)
{
    ti_stream_t * stream;

    if (status < 0)
    {
        log_error("client connection error: `%s`", uv_strerror(status));
        return;
    }

    log_debug("received a TCP client connection");

    stream = ti_stream_create(TI_STREAM_TCP_IN_NODE, &thingsdb__clients_pkg_cb);

    if (!stream)
        return;

    uv_tcp_init(thingsdb_get()->loop, (uv_tcp_t *) stream->uvstream);
    if (uv_accept(uvstream, stream->uvstream) == 0)
    {
        uv_read_start(stream->uvstream, ti_stream_alloc_buf, ti_stream_on_data);
    }
    else
    {
        ti_stream_drop(stream);
    }
}

static void thingsdb__clients_pipe_connection(uv_stream_t * uvstream, int status)
{
    ti_stream_t * stream;

    if (status < 0)
    {
        log_error("client connection error: `%s`", uv_strerror(status));
        return;
    }

    log_debug("received a PIPE client connection");

    stream = ti_stream_create(TI_STREAM_PIPE_IN_NODE, &thingsdb__clients_pkg_cb);

    if (!stream)
        return;

    uv_pipe_init(thingsdb_get()->loop, (uv_pipe_t *) stream->uvstream, 0);
    if (uv_accept(uvstream, stream->uvstream) == 0)
    {
        uv_read_start(stream->uvstream, ti_stream_alloc_buf, ti_stream_on_data);
    }
    else
    {
        ti_stream_drop(stream);
    }
}

static void thingsdb__clients_pkg_cb(ti_stream_t * stream, ti_pkg_t * pkg)
{
    switch (pkg->tp)
    {
    case TI_PROTO_CLIENT_REQ_PING:
        thingsdb__clients_on_ping(stream, pkg);
        break;
    case TI_PROTO_CLIENT_REQ_AUTH:
        thingsdb__clients_on_auth(stream, pkg);
        break;
    default:
        log_error(
                "unexpected package type `%u` from source `%s`)",
                pkg->tp,
                ti_stream_name(stream));
    }
}

static void thingsdb__clients_on_ping(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ti_pkg_t * resp = ti_pkg_new(pkg->id, TI_PROTO_CLIENT_RES_PING, NULL, 0);
    if (!resp || ti_fthingsdb_write(stream, resp))
    {
        free(resp);
        log_error(EX_ALLOC);
    }
}

static void thingsdb__clients_on_auth(ti_stream_t * stream, ti_pkg_t * pkg)
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
        ex_set(e, EX_USER_AUTH, "invalid authentication request");
        log_error("%s from `%s`", e->msg, ti_stream_name(stream));
        resp = ti_pkg_err(pkg->id, e);
        goto finish;
    }
    user = thingsdb_users_auth(thingsdb_get()->users, &name, &pass, e);
    if (e->nr)
    {
        assert (user == NULL);
        log_warning(
                "authentication failed: `%s` (source: `%s`)",
                e->msg,
                ti_stream_addr(stream));
        resp = ti_pkg_err(pkg->id, e->nr, e->msg);
    }
    else
    {
        assert (user != NULL);
        if (stream->via.user)
            ti_user_drop(stream->via.user);
        stream->via.user = ti_user_grab(user);
        resp = ti_pkg_new(pkg->id, TI_PROTO_CLIENT_RES_AUTH, NULL, 0);
    }

finish:
    if (!resp || ti_front_write(stream, resp))
    {
        free(resp);
        log_error(EX_ALLOC);
    }
}

static void thingsdb__clients_write_cb(ti_write_t * req, ex_e status)
{
    (void)(status);     /* errors are logged by ti__write_cb() */
    free(req->pkg);
    ti_write_destroy(req);
}
