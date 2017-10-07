/*
 * sock.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <sys/socket.h>
#include <rql/sock.h>


rql_sock_t * rql_sock_create(rql_sock_e tp, rql_t * rql)
{
    rql_sock_t * sock = (rql_sock_t *) malloc(sizeof(rql_sock_t));
    if (!sock) return NULL;

    sock->ref = 1;
    sock->n = 0;
    sock->sz = 0;
    sock->tp = tp;
    sock->flags = 0;
    sock->rql = rql;
    sock->via.user = NULL;
    sock->cb = NULL;
    sock->tcp = (uv_tcp_t *) malloc(sizeof(uv_tcp_t));
    sock->buf = NULL;

    if (!sock->tcp)
    {
        rql_sock_drop(sock);
        return NULL;
    }

    sock->tcp->data = sock;

    return sock;
}

rql_sock_t * rql_sock_grab(rql_sock_t * sock)
{
    sock->ref++;
    return sock;
}

void rql_sock_drop(rql_sock_t * sock)
{
    if (sock && !--sock->ref)
    {
        if (sock->flags & RQL_SOCK_FLAG_INIT)
        {
            uv_close(sock->tcp, (uv_close_cb) rql__sock_destroy);
            return;
        }
        rql__sock_destroy(sock->tcp);
    }
}

void rql_sock_init(rql_sock_t * sock)
{
    uv_tcp_init(sock->rql->loop, sock->tcp);
    sock->flags |= RQL_SOCK_FLAG_INIT;
}

void rql_sock_close(rql_sock_t * sock)
{
    sock->n = 0;
    sock->flags |= RQL_SOCK_FLAG_CLOSE;
    rql_sock_drop(sock);
}

void rql_sock_alloc_buf(uv_handle_t * handle, size_t sugsz, uv_buf_t * buf)
{
    rql_sock_t * sock  = (rql_sock_t *) handle->data;

    if (sock->n == 0 && sock->sz != sugsz)
    {
        free(sock->buf);
        sock->buf = (char *) malloc(sugsz);
        if (sock->buf == NULL)
        {
            log_error("error allocating memory for buffer");
            buf->base = NULL;
            buf->len = 0;
            return;
        }
        sock->sz = sugsz;
        sock->n = 0;
    }

    buf->base = sock->buf + sock->n;
    buf->len = sock->sz - sock->n;
}

void rql_sock_on_data(uv_stream_t * clnt, ssize_t n, const uv_buf_t * buf)
{
    rql_sock_t * sock  = (rql_sock_t *) clnt->data;
    rql_pkg_t * pkg;
    size_t total_sz;
    int rc;

    if (sock->flags & RQL_SOCK_FLAG_CLOSE) return;

    if (n < 0)
    {
        if (n != UV_EOF) log_error(uv_strerror(n));
        rql_sock_close(sock);
        return;
    }

    sock->n += n;

    if (sock->n < sizeof(rql_pkg_t)) return;

    pkg = (rql_pkg_t *) sock->buf;
    if (!siridb_pkg_check_bit(pkg) || pkg->n > RQL_PKG_MAX_SIZE)
    {
        log_error("invalid package; closing connection");
        rql_sock_drop(sock);
        return;
    }

    total_sz = sizeof(rql_pkg_t) + pkg->n;

    if (sock->n < total_sz)
    {
        if (sock->sz < total_sz)
        {
            char * tmp = realloc(sock->buf, total_sz);
            if (!tmp)
            {
                rql_sock_close(sock);
                return;
            }
            sock->buf = tmp;
            sock->sz = total_sz;
        }
        return;
    }

    sock->cb(sock, pkg);

    sock->n -= total_sz;

    if (sock->n > 0)
    {
        /* move data and call rql_sock_on_data() again */
        memmove(sock->buf, sock->buf + total_sz, sock->n);
        rql_sock_on_data(clnt, 0, buf);
    }
}

const char * rql_sock_ip_support_str(uint8_t ip_support)
{
    switch (ip_support)
    {
    case AF_UNSPEC: return "ALL";
    case AF_INET: return "IPV4ONLY";
    case AF_INET6: return "IPV6ONLY";
    default: return "UNKNOWN";
    }
}

static void rql__sock_destroy(uv_tcp_t * tcp)
{
    rql_sock_t * sock = (rql_sock_t * ) tcp->data;
    switch (sock->tp)
    {
    case RQL_SOCK_BACK:
    case RQL_SOCK_CONN:
        rql_node_drop(sock->via.node);
        break;
    case RQL_SOCK_FRONT:
        rql_user_drop(sock->via.user);

    }
    free(sock);
    free(tcp);
}


