/*
 * front.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <rql/front.h>
#include <rql/rql.h>
#include <rql/user.h>
#include <rql/users.h>
#include <rql/write.h>
#include <rql/ref.h>
#include <rql/proto.h>

static void rql__front_on_connect(uv_tcp_t * tcp, int status);
static void rql__front_on_pkg(rql_sock_t * sock, rql_pkg_t * pkg);
static void rql__front_on_auth(rql_sock_t * sock, rql_pkg_t * pkg);
static void rql__front_write_cb(rql_write_t * req, int status);

rql_front_t * rql_front_create(rql_t * rql)
{
    rql_front_t * front = (rql_front_t *) malloc(sizeof(rql_front_t));
    if (!front) return NULL;

    front->sock = rql_sock_create(RQL_SOCK_FRONT, rql);
    if (!front->sock)
    {
        rql_front_destroy(front);
        return NULL;
    }

    return front;
}

void rql_front_destroy(rql_front_t * front)
{
    if (!front) return;
    rql_sock_drop(front->sock);
    free(front);
}

int rql_front_listen(rql_front_t * front)
{
    int rc;
    rql_t * rql = front->sock->rql;

    if (rql_sock_init(front->sock)) return -1;
    RQL_ref_inc(front->sock);

    struct sockaddr_storage addr;

    if (rql->cfg->ip_support == AF_INET)
    {
        uv_ip4_addr(
                "0.0.0.0",
                rql->cfg->client_port,
                (struct sockaddr_in *) &addr);
    }
    else
    {
        uv_ip6_addr(
                "::",
                rql->cfg->client_port,
                (struct sockaddr_in6 *) &addr);
    }

    if ((rc = uv_tcp_bind(
            &front->sock->tcp,
            (const struct sockaddr *) &addr,
            (rql->cfg->ip_support == AF_INET6) ?
                    UV_TCP_IPV6ONLY : 0)) ||

        (rc = uv_listen(
            (uv_stream_t *) &front->sock->tcp,
            RQL_MAX_NODES,
            (uv_connection_cb) rql__front_on_connect)))
    {
        log_error("error listening for nodes: %s", uv_strerror(rc));
        return -1;
    }

    log_info("start listening for clients on port %d", rql->cfg->client_port);
    return 0;
}

int rql_front_write(rql_sock_t * sock, rql_pkg_t * pkg)
{
    return rql_write(sock, pkg, NULL, rql__front_write_cb);
}

static void rql__front_write_cb(rql_write_t * req, ex_e status)
{
    (void)(status);
    free(req->pkg);
    rql_write_destroy(req);
}

static void rql__front_on_connect(uv_tcp_t * tcp, int status)
{
    int rc;

    if (status < 0)
    {
        log_error("client connection error: %s", uv_strerror(status));
        return;
    }

    rql_sock_t * sock = (rql_sock_t *) tcp->data;

    rql_sock_t * nsock = rql_sock_create(RQL_SOCK_CLIENT, sock->rql);
    if (!nsock || rql_sock_init(nsock)) return;
    nsock->cb = rql__front_on_pkg;
    if ((rc = uv_accept((uv_stream_t *) tcp, (uv_stream_t *) &nsock->tcp)) ||
        (rc = uv_read_start(
                (uv_stream_t *) &nsock->tcp,
                rql_sock_alloc_buf,
                rql_sock_on_data)))
    {
        log_error(uv_strerror(rc));
        rql_sock_close(nsock);
        return;
    }
    log_info("client connected: %s", rql_sock_addr(nsock));
}


static void rql__front_on_pkg(rql_sock_t * sock, rql_pkg_t * pkg)
{
    switch ((rql_front_req_e) pkg->tp)
    {
    case RQL_FRONT_AUTH:
        rql__front_on_auth(sock, pkg);
        break;
    default:
        log_error("unexpected package type: %u (source: %s)",
                pkg->tp, rql_sock_addr(sock));
    }
}

static void rql__front_on_auth(rql_sock_t * sock, rql_pkg_t * pkg)
{
    rql_pkg_t * resp;
    qp_unpacker_t unpacker;
    qp_obj_t name, pass;
    qp_unpacker_init(&unpacker, pkg->data, pkg->n);
    if (!qp_is_array(qp_next(&unpacker, NULL)) ||
        !qp_is_raw(qp_next(&unpacker, &name)) ||
        !qp_is_raw(qp_next(&unpacker, &pass))) return;
    ex_ptr(e);
    rql_user_t * user = rql_users_auth(sock->rql->users, &name, &pass, e);
    if (e->errnr)
    {
        log_error("authentication failed: %s (source: %s)",
                e->errmsg, rql_sock_addr(sock));
        resp = rql_pkg_e(e, pkg->id);
    }
    else
    {
        /* only set new authentication when successful */
        sock->via.user = rql_user_grab(user);
        resp = rql_pkg_new(pkg->id, RQL_PROTO_ACK, NULL, 0);
    }

    if (!resp || rql_front_write(sock, resp))
    {
        free(resp);
        log_error(EX_ALLOC);
    }
}
