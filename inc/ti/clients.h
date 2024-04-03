/*
 * clients.h
 */
#ifndef TI_CLIENTS_H_
#define TI_CLIENTS_H_

#include <ti/pkg.t.h>
#include <ti/req.t.h>
#include <ti/rpkg.t.h>
#include <uv.h>

typedef struct ti_clients_s ti_clients_t;

int ti_clients_create(void);
void ti_clients_destroy(void);
int ti_clients_listen(void);
void ti_clients_write_rpkg(ti_rpkg_t * rpkg);
_Bool ti_clients_is_fwd_req(ti_req_t * req);
void ti_clients_pkg_cb(ti_stream_t * stream, ti_pkg_t * pkg);

struct ti_clients_s
{
    uv_tcp_t tcp;
    uv_pipe_t pipe;
};

#endif /* TI_CLIENTS_H_ */
