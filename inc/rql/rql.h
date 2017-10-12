/*
 * rql.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_H_
#define RQL_H_

#define RQL_MAX_NODES 64

#define RQL_FLAG_SIGNAL 1
#define RQL_FLAG_SAVING 2

typedef struct rql_s rql_t;

#include <uv.h>
#include <signal.h>
#include <string.h>
#include <inttypes.h>
#include <util/vec.h>
#include <rql/args.h>
#include <rql/cfg.h>
#include <rql/node.h>
#include <rql/back.h>
#include <rql/front.h>
#include <util/logger.h>
#include <util/link.h>

#define rql_term(signum__) {\
    log_critical("critical error at: %s:%d (%s); raising %s", \
    __FILE__, __LINE__, __func__, strsignal(signum__)); \
    raise(signum__);}

rql_t * rql_create(void);
void rql_destroy(rql_t * rql);
void rql_init_logger(rql_t * rql);
int rql_init_fn(rql_t * rql);
int rql_build(rql_t * rql);
int rql_read(rql_t * rql);
int rql_run(rql_t * rql);
int rql_save(rql_t * rql);
int rql_lock(rql_t * rql);
int rql_unlock(rql_t * rql);


struct rql_s
{
    char * fn;
    rql_node_t * node;
    rql_args_t * args;
    rql_cfg_t * cfg;
    rql_back_t * back;
    rql_front_t * front;
    link_t * dbs;
    vec_t * nodes;
    vec_t * users;
    uv_loop_t * loop;
    uint64_t event_cur_id;
    uint64_t event_max_id;
    link_t * queue;         /* queued events */
    uint8_t redundancy;     /* value 1..64 */
    uint8_t flags;
};

#endif /* RQL_H_ */
