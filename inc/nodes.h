/*
 * nodes.h
 */
#ifndef THINGSDB_NODES_H_
#define THINGSDB_NODES_H_

#include <util/vec.h>
#include <uv.h>

typedef struct thingsdb_nodes_s thingsdb_nodes_t;

int thingsdb_nodes_create(void);
void thingsdb_nodes_destroy(void);
int thingsdb_nodes_listen(void);
_Bool thingsdb_nodes_has_quorum(void);

struct thingsdb_nodes_s
{
    vec_t * vec;
    uv_tcp_t tcp;
    uv_pipe_t pipe;
};

#endif /* THINGSDB_NODES_H_ */
