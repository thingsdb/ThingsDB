/*
 * sock.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_SOCK_H_
#define RQL_SOCK_H_

#define RQL_SOCK_FLAG_INIT 1
#define RQL_SOCK_FLAG_AUTH 2

typedef enum
{
    RQL_SOCK_BACK,  /* listen to nodes */
    RQL_SOCK_CONN,  /* connections to nodes */
    RQL_SOCK_FRONT, /* listen to clients */
} rql_sock_e;

typedef struct rql_sock_s  rql_sock_t;
typedef union rql_sock_u rql_sock_via_t;

#include <uv.h>
#include <inttypes.h>
#include <rql/rql.h>
#include <rql/user.h>
#include <rql/node.h>
#include <rql/pkg.h>

typedef void (*rql_sock_cb)(rql_sock_t * sock, rql_pkg_t * pkg);


rql_sock_t * rql_sock_create(rql_sock_e tp, rql_t * rql);
rql_sock_t * rql_sock_grab(rql_sock_t * sock);
void rql_sock_drop(rql_sock_t * sock);
int rql_sock_init(rql_sock_t * sock);
void rql_sock_close(rql_sock_t * sock);
void rql_sock_alloc_buf(uv_handle_t * handle, size_t sugsz, uv_buf_t * buf);
void rql_sock_on_data(uv_stream_t * clnt, ssize_t n, const uv_buf_t * buf);
const char * rql_sock_ip_support_str(uint8_t ip_support);

union rql_sock_u
{
    rql_user_t * user;
    rql_node_t * node;
};

struct rql_sock_s
{
    uint64_t ref;
    uint32_t n;
    uint32_t sz;
    rql_sock_e tp;
    uint8_t flags;
    rql_t * rql;
    rql_sock_via_t via;
    rql_sock_cb cb;
    uv_tcp_t tcp;
    char * buf;
};

#endif /* RQL_SOCK_H_ */


