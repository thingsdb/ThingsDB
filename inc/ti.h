/*
 * thingsdb.h
 */
#ifndef TI_H_
#define TI_H_

#define TI_MAX_NODES 64

#define TI_FLAG_SIGNAL 1
#define TI_FLAG_INDEXING 2
#define TI_FLAG_LOCKED 4

typedef struct ti_s ti_t;

#include <uv.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>
#include <ti/args.h>
#include <ti/cfg.h>
#include <ti/events.h>
#include <ti/nodes.h>
#include <ti/clients.h>
#include <ti/node.h>
#include <ti/lookup.h>
#include <ti/maint.h>
#include <util/logger.h>
#include <util/vec.h>
#include <util/smap.h>

#define ti_term(signum__) do {\
    log_critical("raise at: %s:%d,%s (%s)", \
    __FILE__, __LINE__, __func__, strsignal(signum__)); \
    raise(signum__);} while(0)

int ti_create(void);
void ti_destroy(void);
ti_t * ti_get(void);
void ti_init_logger(void);
int ti_init_fn(void);
int ti_build(void);
int ti_read(void);
int ti_run(void);
int ti_save(void);
int ti_lock(void);
int ti_unlock(void);
int ti_store(const char * fn);      /* call ti_store_store() for storing */
int ti_restore(const char * fn);    /* call ti_store_restore() for restoring */
uint64_t ti_next_thing_id(void);
_Bool ti_manages_id(uint64_t id);

struct ti_s
{
    char * fn;
    ti_node_t * node;
    ti_args_t * args;
    ti_cfg_t * cfg;
    ti_maint_t * maint;
    ti_lookup_t * lookup;
    ti_clients_t * clients;
    ti_events_t * events;
    ti_nodes_t * nodes;
    vec_t * dbs;
    vec_t * users;
    vec_t * access;
    smap_t * props;
    uv_loop_t * loop;
    uint64_t next_thing_id;   /* used for assigning id's to objects */
    uint8_t redundancy;     /* value 1..64 */
    uint8_t flags;
};

#endif /* TI_H_ */
