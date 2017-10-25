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

extern const char * rql_name;

typedef struct rql_s rql_t;

#include <uv.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>
#include <rql/args.h>
#include <rql/cfg.h>
#include <rql/node.h>
#include <rql/back.h>
#include <rql/front.h>
#include <rql/lookup.h>
#include <rql/events.h>
#include <rql/maint.h>
#include <util/logger.h>
#include <util/vec.h>

#define rql_term(signum__) {\
    log_critical("raise at: %s:%d,%s (%s)", \
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
int rql__store(rql_t * rql, const char * fn);
int rql__restore(rql_t * rql, const char * fn);
static inline uint64_t rql_get_id(rql_t * rql);

struct rql_s
{
    char * fn;
    rql_node_t * node;
    rql_args_t * args;
    rql_cfg_t * cfg;
    rql_back_t * back;
    rql_front_t * front;
    rql_events_t * events;
    rql_maint_t * maint;
    rql_lookup_t * lookup;
    vec_t * dbs;
    vec_t * nodes;
    vec_t * users;
    vec_t * access;
    uint64_t next_id_;   /* used for assigning id's to objects */
    uint8_t redundancy;     /* value 1..64 */
    uint8_t flags;
    uv_loop_t loop;
};


static inline uint64_t rql_get_id(rql_t * rql)
{
    return rql->next_id_++;
}

#endif /* RQL_H_ */
