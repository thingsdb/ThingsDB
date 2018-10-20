/*
 * clients.h
 */
#ifndef THINGSDB_CLIENTS_H_
#define THINGSDB_CLIENTS_H_

#include <util/vec.h>
#include <uv.h>
#include <ti/stream.h>
#include <ti/pkg.h>

typedef struct thingsdb_clients_s thingsdb_clients_t;

int thingsdb_clients_create(void);
void thingsdb_clients_destroy(void);
int thingsdb_clients_listen(void);
int thingsdb_clients_write(ti_stream_t * stream, ti_pkg_t * pkg);

struct thingsdb_clients_s
{
    uv_tcp_t tcp;
    uv_pipe_t pipe;
};

#endif /* THINGSDB_CLIENTS_H_ */
