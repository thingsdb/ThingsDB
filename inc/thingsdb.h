/*
 * thingsdb.h
 */
#ifndef THINGSDB_H_
#define THINGSDB_H_

#define THINGSDB_MAX_NODES 64

#define THINGSDB_FLAG_SIGNAL 1
#define THINGSDB_FLAG_INDEXING 2
#define THINGSDB_FLAG_LOCKED 4

typedef struct thingsdb_s thingsdb_t;

#include <uv.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>
#include <ti/args.h>
#include <ti/cfg.h>
#include <ti/node.h>
#include <ti/back.h>
#include <ti/front.h>
#include <ti/lookup.h>
#include <ti/events.h>
#include <ti/maint.h>
#include <util/logger.h>
#include <util/vec.h>
#include <util/smap.h>

#define ti_term(signum__) {\
    log_critical("raise at: %s:%d,%s (%s)", \
    __FILE__, __LINE__, __func__, strsignal(signum__)); \
    raise(signum__);}

int thingsdb_init(void);
void thingsdb_close(void);
thingsdb_t * thingsdb_get(void);
void thingsdb_init_logger(void);
int thingsdb_init_fn(void);
int thingsdb_build(void);
int thingsdb_read(void);
int thingsdb_run(void);
int thingsdb_save(void);
int thingsdb_lock(void);
int thingsdb_unlock(void);
int thingsdb_store(const char * fn);
int thingsdb_restore(const char * fn);
uint64_t thingsdb_get_next_id(void);
_Bool thingsdb_has_id(uint64_t id);

struct thingsdb_s
{
    char * fn;
    ti_node_t * node;
    ti_args_t * args;
    ti_cfg_t * cfg;
    ti_back_t * back;
    ti_front_t * front;
    ti_events_t * events;
    ti_maint_t * maint;
    ti_lookup_t * lookup;
    vec_t * dbs;
    vec_t * nodes;
    vec_t * users;
    vec_t * access;
    smap_t * props;
    uv_loop_t * loop;
    uint64_t next_id_;   /* used for assigning id's to objects */
    uint8_t redundancy;     /* value 1..64 */
    uint8_t flags;
};

#endif /* THINGSDB_H_ */
