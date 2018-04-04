/*
 * back.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <sys/socket.h>
#include <rql/back.h>
#include <rql/ref.h>

static void rql__back_on_connect(uv_tcp_t * tcp, int status);
static void rql__back_on_pkg(rql_sock_t * sock, rql_pkg_t * pkg);

rql_back_t * rql_back_create(rql_t * rql)
{
    rql_back_t * back = malloc(sizeof(rql_back_t));
    if (!back) return NULL;

    back->sock = rql_sock_create(RQL_SOCK_BACK, rql);
    if (!back->sock)
    {
        rql_back_destroy(back);
        return NULL;
    }

    return back;
}

void rql_back_destroy(rql_back_t * back)
{
    if (!back) return;
    rql_sock_drop(back->sock);
    free(back);
}

int rql_back_listen(rql_back_t * back)
{
    int rc;
    rql_t * rql = back->sock->rql;

    if (rql_sock_init(back->sock)) return -1;
    RQL_ref_inc(back->sock);

    struct sockaddr_storage addr;

    if (rql->cfg->ip_support == AF_INET)
    {
        uv_ip4_addr("0.0.0.0", rql->cfg->port, (struct sockaddr_in *) &addr);
    }
    else
    {
        uv_ip6_addr("::", rql->cfg->port, (struct sockaddr_in6 *) &addr);
    }

    if ((rc = uv_tcp_bind(
            &back->sock->tcp,
            (const struct sockaddr *) &addr,
            (rql->cfg->ip_support == AF_INET6) ?
                    UV_TCP_IPV6ONLY : 0)) ||

        (rc = uv_listen(
            (uv_stream_t *) &back->sock->tcp,
            RQL_MAX_NODES,
            (uv_connection_cb) rql__back_on_connect)))
    {
        log_error("error listening for nodes: %s", uv_strerror(rc));
        return -1;
    }

    log_info("start listening for nodes on port %d", rql->cfg->port);
    return 0;
}

const char * rql_back_req_str(rql_back_req_e tp)
{
    switch (tp)
    {
    case RQL_BACK_PING: return "REQ_PING";
    case RQL_BACK_AUTH: return "REQ_AUTH";
    case RQL_BACK_EVENT_REG: return "REQ_EVENT_REG";
    case RQL_BACK_EVENT_UPD: return "REQ_EVENT_UPD";
    case RQL_BACK_EVENT_READY: return "REQ_EVENT_READY";
    case RQL_BACK_EVENT_CANCEL: return "REQ_EVENT_CANCEL";
    case RQL_BACK_MAINT_REG: return "REQ_MAINT_REG";
    }
    return "REQ_UNKNOWN";
}

static void rql__back_on_connect(uv_tcp_t * tcp, int status)
{
    int rc;

    if (status < 0)
    {
        log_error("node connection error: %s", uv_strerror(status));
        return;
    }

    rql_sock_t * sock = (rql_sock_t *) tcp->data;

    rql_sock_t * nsock = rql_sock_create(RQL_SOCK_NODE, sock->rql);
    if (!nsock || rql_sock_init(nsock)) return;
    nsock->cb = rql__back_on_pkg;
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
    log_info("node connected: %s", rql_sock_addr(nsock));
}

static void rql__back_on_pkg(rql_sock_t * sock, rql_pkg_t * pkg)
{
    switch (pkg->tp)
    {
    default:
        log_error("test sock flags: %u", sock->flags);
    }
}
