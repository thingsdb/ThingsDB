/*
 * write.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <rql/write.h>
#include <rql/ref.h>

static void rql__write_cb(uv_write_t * req, int status);

int rql_write(rql_sock_t * sock, rql_pkg_t * pkg, void * data, rql_write_cb cb)
{
    rql_write_t * req = (rql_write_t *) malloc(sizeof(uv_write_t));
    if (!req) return -1;

    RQL_ref_inc(sock);
    req->req_.data = req;
    req->sock = sock;
    req->pkg = pkg;
    req->data = data;
    req->cb_ = cb;

    uv_buf_t wrbuf = uv_buf_init((char *) pkg, sizeof(rql_pkg_t) + pkg->n);
    uv_write(&req->req_, (uv_stream_t *) &sock->tcp, &wrbuf, 1, &rql__write_cb);

    return 0;
}

static void rql__write_cb(uv_write_t * req, int status)
{
    if (status) log_error(uv_strerror(status));

    rql_write_t * rq = (rql_write_t *) req->data;

    rql_sock_drop(rq->sock);
    rq->cb_(rq, status);
}
