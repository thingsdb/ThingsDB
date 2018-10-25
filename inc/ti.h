/*
 * ti.h
 */
#ifndef TI_H_
#define TI_H_

#define TI_URL "https://thinkdb.net"
#define TI_DOCS TI_URL"/docs/"

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

#define ti_grab(x) ((x) && ++(x)->ref ? (x) : NULL)

/* SUSv2 guarantees that "Host names are limited to 255 bytes,
 * excluding terminating null byte" */
#define TI_MAX_HOSTNAME_SZ 256

#define TI_FLAG_SIGNAL 1
#define TI_FLAG_INDEXING 2
#define TI_FLAG_LOCKED 4

typedef struct ti_s ti_t;

#include <uv.h>
#include <cleri/cleri.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>
#include <ti/args.h>
#include <ti/cfg.h>
#include <ti/events.h>
#include <ti/nodes.h>
#include <ti/store.h>
#include <ti/clients.h>
#include <ti/node.h>
#include <ti/lookup.h>
#include <ti/maint.h>
#include <util/logger.h>
#include <util/vec.h>
#include <util/smap.h>

extern ti_t ti_;

#define ti_term(signum__) do {\
    log_critical("raise at: %s:%d,%s (%s)", \
    __FILE__, __LINE__, __func__, strsignal(signum__)); \
    raise(signum__);} while(0)

int ti_create(void);
void ti_destroy(void);
void ti_init_logger(void);
int ti_init(void);
int ti_build(void);
int ti_read(void);
int ti_run(void);
int ti_save(void);
int ti_lock(void);
int ti_unlock(void);
uint64_t ti_next_thing_id(void);
_Bool ti_manages_id(uint64_t id);
static inline ti_t * ti(void);

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
    ti_store_t * store;
    vec_t * dbs;
    vec_t * users;
    vec_t * access;
    smap_t * names;
    uv_loop_t * loop;
    cleri_grammar_t * langdef;
    uint64_t next_thing_id;   /* used for assigning id's to objects */
    uint8_t redundancy;     /* value 1..64 */
    uint8_t flags;

    char hostname[256];
};

static inline ti_t * ti(void)
{
    return &ti_;
}

#endif /* TI_H_ */
