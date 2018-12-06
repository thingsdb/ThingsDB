/*
 * clients.h
 */
#ifndef TI_CLIENTS_H_
#define TI_CLIENTS_H_

#include <util/vec.h>
#include <uv.h>
#include <ti/stream.h>
#include <ti/pkg.h>
#include <ti/rpkg.h>

typedef struct ti_clients_s ti_clients_t;

int ti_clients_create(void);
void ti_clients_destroy(void);
int ti_clients_listen(void);
void ti_clients_write_rpkg(ti_rpkg_t * rpkg);

struct ti_clients_s
{
    uv_tcp_t tcp;
    uv_pipe_t pipe;
};

#endif /* TI_CLIENTS_H_ */
