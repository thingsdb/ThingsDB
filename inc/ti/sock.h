/*
 * sock.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef TI_SOCK_H_
#define TI_SOCK_H_

#define TI_SOCK_FLAG_INIT 1
#define TI_SOCK_FLAG_AUTH 2

typedef enum
{
    TI_SOCK_BACK,      /* listen to nodes */
    TI_SOCK_NODE,      /* connections to nodes */
    TI_SOCK_FRONT,     /* listen to clients */
    TI_SOCK_CLIENT,    /* connections to clients */
} ti_sock_e;

typedef struct ti_sock_s  ti_sock_t;
typedef union ti_sock_u ti_sock_via_t;

#include <uv.h>
#include <stdint.h>
#include <ti/user.h>#include <<ti/node.h>include <t<ti/pkg.h>ypedef void (*ti_sock_ondata_cb)(ti_sock_t * sock, ti_pkg_t * pkg);

ti_sock_t * ti_sock_create(ti_sock_e tp);
ti_sock_t * ti_sock_grab(ti_sock_t * sock);
void ti_sock_drop(ti_sock_t * sock);
int ti_sock_init(ti_sock_t * sock);
void ti_sock_close(ti_sock_t * sock);
void ti_sock_alloc_buf(uv_handle_t * handle, size_t sugsz, uv_buf_t * buf);
void ti_sock_on_data(uv_stream_t * clnt, ssize_t n, const uv_buf_t * buf);
const char * ti_sock_ip_support_str(uint8_t ip_support);
const char * ti_sock_addr(ti_sock_t * sock);

union ti_sock_u
{
    ti_user_t * user;
    ti_node_t * node;
};

struct ti_sock_s
{
    uint32_t ref;
    uint32_t n;     /* buffer n */
    uint32_t sz;    /* buffer sz */
    ti_sock_e tp;
    uint8_t flags;
    ti_sock_via_t via;
    ti_sock_ondata_cb cb;
    uv_tcp_t tcp;
    char * buf;
    char * addr_;
};

struct ti_sock_req_s
{
    ti_sock_t * sock;
    ti_pkg_t * pkg;
};

#endif /* TI_SOCK_H_ */


