/*
 * front.c
 */
#include <stdlib.h>
#include <thingsdb.h>
#include <ti/dbs.h>
#include <ti/event.h>
#include <ti/front.h>
#include <ti/proto.h>
#include <ti/ref.h>
#include <ti/user.h>
#include <ti/users.h>
#include <ti/write.h>
void ti__front_on_connect(uv_tcp_t * tcp, int status);
static void ti__front_on_pkg(ti_sock_t * sock, ti_pkg_t * pkg);
static void ti__front_on_ping(ti_sock_t * sock, ti_pkg_t * pkg);
static void ti__front_on_auth(ti_sock_t * sock, ti_pkg_t * pkg);
static void ti__front_on_event(ti_sock_t * sock, ti_pkg_t * pkg);
static void ti__front_on_get(ti_sock_t * sock, ti_pkg_t * pkg);
static void ti__front_write_cb(ti_write_t * req, int status);

ti_front_t * ti_front_create(void)
{
    ti_front_t * front = malloc(sizeof(ti_front_t));
    if (!front)
        return NULL;

    front->sock = ti_sock_create(TI_SOCK_FRONT);
    if (!front->sock)
    {
        ti_front_destroy(front);
        return NULL;
    }

    return front;
}

void ti_front_destroy(ti_front_t * front)
{
    if (!front)
        return;
    ti_sock_drop(front->sock);
    free(front);
}

int ti_front_listen(ti_front_t * front)
{
    int rc;
    ti_cfg_t * cfg = thingsdb_get()->cfg;

    if (ti_sock_init(front->sock)) return -1;
    TI_ref_inc(front->sock);

    struct sockaddr_storage addr;

    if (cfg->ip_support == AF_INET)
    {
        uv_ip4_addr(
                "0.0.0.0",
                cfg->client_port,
                (struct sockaddr_in *) &addr);
    }
    else
    {
        uv_ip6_addr("::", cfg->client_port, (struct sockaddr_in6 *) &addr);
    }

    if ((rc = uv_tcp_bind(
            &front->sock->tcp,
            (const struct sockaddr *) &addr,
            (cfg->ip_support == AF_INET6) ? UV_TCP_IPV6ONLY : 0)) ||

        (rc = uv_listen(
            (uv_stream_t *) &front->sock->tcp,
            THINGSDB_MAX_NODES,
            (uv_connection_cb) ti__front_on_connect)))
    {
        log_error("error listening for nodes: `%s`", uv_strerror(rc));
        return -1;
    }

    log_info("start listening for clients on port %d", cfg->client_port);
    return 0;
}

int ti_front_write(ti_sock_t * sock, ti_pkg_t * pkg)
{
    return ti_write(sock, pkg, NULL, ti__front_write_cb);
}

static void ti__front_write_cb(ti_write_t * req, ex_e status)
{
    (void)(status);
    free(req->pkg);
    ti_write_destroy(req);
}

static void ti__front_on_connect(uv_tcp_t * tcp, int status)
{
    int rc;

    if (status < 0)
    {
        log_error("client connection error: `%s`", uv_strerror(status));
        return;
    }

    ti_sock_t * sock = (ti_sock_t *) tcp->data;

    ti_sock_t * nsock = ti_sock_create(TI_SOCK_CLIENT);
    if (!nsock || ti_sock_init(nsock)) return;
    nsock->cb = ti__front_on_pkg;
    if ((rc = uv_accept((uv_stream_t *) tcp, (uv_stream_t *) &nsock->tcp)) ||
        (rc = uv_read_start(
                (uv_stream_t *) &nsock->tcp,
                ti_sock_alloc_buf,
                ti_sock_on_data)))
    {
        log_error(uv_strerror(rc));
        ti_sock_close(nsock);
        return;
    }
    log_info("client connected: `%s`", ti_sock_addr(nsock));
}


static void ti__front_on_pkg(ti_sock_t * sock, ti_pkg_t * pkg)
{
    switch ((ti_front_req_e) pkg->tp)
    {
    case TI_FRONT_PING:
        ti__front_on_ping(sock, pkg);
        break;
    case TI_FRONT_AUTH:
        ti__front_on_auth(sock, pkg);
        break;
    case TI_FRONT_EVENT:
        ti__front_on_event(sock, pkg);
        break;
    case TI_FRONT_GET:
        ti__front_on_get(sock, pkg);
        break;
    default:
        log_error("unexpected package type: `%u` (source: `%s`)",
                pkg->tp, ti_sock_addr(sock));
    }
}

static void ti__front_on_ping(ti_sock_t * sock, ti_pkg_t * pkg)
{
    ti_pkg_t * resp = ti_pkg_new(pkg->id, TI_PROTO_ACK, NULL, 0);
    if (!resp || ti_front_write(sock, resp))
    {
        free(resp);
        log_error(EX_ALLOC);
    }
}

static void ti__front_on_auth(ti_sock_t * sock, ti_pkg_t * pkg)
{
    ti_pkg_t * resp;
    qp_unpacker_t unpacker;
    qp_obj_t name, pass;
    qp_unpacker_init(&unpacker, pkg->data, pkg->n);
    if (!qp_is_array(qp_next(&unpacker, NULL)) ||
        !qp_is_raw(qp_next(&unpacker, &name)) ||
        !qp_is_raw(qp_next(&unpacker, &pass))) return;
    ex_t * e = ex_use();
    ti_user_t * user = ti_users_auth(thingsdb_get()->users, &name, &pass, e);
    if (e->nr)
    {
        log_error("authentication failed: `%s` (source: `%s`)",
                e->msg, ti_sock_addr(sock));
        resp = ti_pkg_err(pkg->id, e->nr, e->msg);
    }
    else
    {
        /* only set new authentication when successful */
        sock->via.user = ti_user_grab(user);
        resp = ti_pkg_new(pkg->id, TI_PROTO_ACK, NULL, 0);
    }

    if (!resp || ti_front_write(sock, resp))
    {
        free(resp);
        log_error(EX_ALLOC);
    }
}

static void ti__front_on_event(ti_sock_t * sock, ti_pkg_t * pkg)
{
    ex_t * e = ex_use();
    ti_pkg_t * resp;

    if (!sock->via.user)
    {
        ex_set(e, TI_PROTO_AUTH_ERR, "connection is not authenticated");
        goto failed;
    }

    if (thingsdb_get()->node->status != TI_NODE_STAT_READY)
    {
        ex_set(e, TI_PROTO_NODE_ERR,
                "node `%s` is not ready to handle events",
                thingsdb_get()->node->addr);
        goto failed;
    }

    ti_event_new(sock, pkg, e);
    if (e->nr) goto failed;

    return;

failed:
    log_error(e->msg);
    resp = ti_pkg_err(pkg->id, e->nr, e->msg);
    if (!resp || ti_front_write(sock, resp))
    {
        free(resp);
        log_error(EX_ALLOC);
    }
}

static void ti__front_on_get(ti_sock_t * sock, ti_pkg_t * pkg)
{
    ex_t * e = ex_use();
    ti_pkg_t * resp;

    if (!sock->via.user)
    {
        ex_set(e, TI_PROTO_AUTH_ERR, "connection is not authenticated");
        goto failed;
    }

    if (thingsdb_get()->node->status != TI_NODE_STAT_READY)
    {
        ex_set(e, TI_PROTO_NODE_ERR,
                "node `%s` is not ready to handle events",
                thingsdb_get()->node->addr);
        goto failed;
    }

    ti_dbs_get(sock, pkg, e);
    if (e->nr) goto failed;

    return;

failed:
    log_error(e->msg);
    resp = ti_pkg_err(pkg->id, e->nr, e->msg);
    if (!resp || ti_front_write(sock, resp))
    {
        free(resp);
        log_error(EX_ALLOC);
    }
}
