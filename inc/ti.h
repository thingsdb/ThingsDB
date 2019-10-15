/*
 * ti.h
 */
#ifndef TI_H_
#define TI_H_

#include <assert.h>
#include <tiinc.h>
#include <cleri/cleri.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <ti/archive.h>
#include <ti/args.h>
#include <ti/away.h>
#include <ti/build.h>
#include <ti/sync.h>
#include <ti/cfg.h>
#include <ti/clients.h>
#include <ti/collections.h>
#include <ti/connect.h>
#include <ti/counters.h>
#include <ti/events.h>
#include <ti/node.h>
#include <ti/nodes.h>
#include <ti/store.h>
#include <ti/tcp.h>
#include <ti/users.h>
#include <ti/val.h>
#include <unistd.h>
#include <util/logger.h>
#include <util/mpack.h>
#include <util/smap.h>
#include <util/vec.h>
#include <uv.h>

#define ti_term(signum__) do {\
    if (signum__ != SIGINT) log_critical("raise at: %s:%d,%s (%s)", \
    __FILE__, __LINE__, __func__, strsignal(signum__)); \
    raise(signum__);} while(0)

#define ti_panic(fmt__, ...) do {                   \
    log_critical("panic at: %s:%d,%s ("fmt__")",    \
    __FILE__, __LINE__, __func__, ##__VA_ARGS__);   \
    abort();} while(0)

int ti_create(void);
void ti_destroy(void);
int ti_init_logger(void);
int ti_init(void);
int ti_build(void);
int ti_rebuild(void);
int ti_write_node_id(uint32_t * node_id);
int ti_read_node_id(uint32_t * node_id);
int ti_read(void);
int ti_unpack(uchar * data, size_t n);
int ti_run(void);
void ti_shutdown(void);
void ti_stop(void);
int ti_save(void);
int ti_lock(void);
int ti_unlock(void);
_Bool ti_ask_continue(void);
void ti_print_connect_info(void);
ti_rpkg_t * ti_node_status_rpkg(void);  /* returns package with next_thing_id,
                                           cevid, ti_node->status
                                        */
void ti_set_and_broadcast_node_status(ti_node_status_t status);
void ti_set_and_broadcast_node_zone(uint8_t zone);
void ti_broadcast_node_info(void);
int ti_this_node_to_pk(msgpack_packer * pk);
ti_val_t * ti_this_node_as_mpval(void);
static inline ti_t * ti(void);
static inline uint64_t ti_next_thing_id(void);
static inline int ti_sleep(int ms);
static inline const char * ti_name(void);
static inline int ti_to_pk(msgpack_packer * pk);

struct ti_s
{
    struct timespec boottime;
    char * fn;                  /* ti__fn */
    char * node_fn;             /* ti__node_fn */
    uint64_t last_event_id;     /* when `ti__fn` was saved */
    ti_archive_t * archive;     /* committed events archive */
    ti_args_t * args;
    ti_away_t * away;
    ti_build_t * build;         /* only when using --secret */
    ti_cfg_t * cfg;
    ti_clients_t * clients;
    ti_collections_t * collections;
    ti_connect_t * connect_loop;
    ti_counters_t * counters;   /* counters for statistics */
    ti_events_t * events;
    ti_node_t * node;
    ti_nodes_t * nodes;
    ti_store_t * store;
    ti_sync_t * sync;
    ti_thing_t * thing0;        /* thing with id 0 */
    ti_users_t * users;
    vec_t * access_node;        /* ti_access_t */
    vec_t * access_thingsdb;    /* ti_access_t */
    vec_t * procedures;         /* ti_procedure_t */
    smap_t * names;             /* weak map for ti_name_t */
    uv_loop_t * loop;
    cleri_grammar_t * langdef;
    uint8_t flags;
    char hostname[256];
};

static inline ti_t * ti(void)
{
    return &ti_;
}

/* Return the next thing id and increment by one. */
static inline uint64_t ti_next_thing_id(void)
{
    return ti_.node->next_thing_id++;
}

/*
 * Sleep in milliseconds (value must be between 0 and 999)
 * This function is used within the `away` thread to intentionally slow
 * things down. */
static inline int ti_sleep(int ms)
{
    assert (ms < 1000);
    return (ti_.flags & TI_FLAG_SIGNAL)
            ? -2
            : nanosleep((const struct timespec[]){{0, ms * 1000000L}}, NULL);
}

static inline const char * ti_name(void)
{
    return ti_.hostname;
}

static inline int ti_to_pk(msgpack_packer * pk)
{
    return -(
        msgpack_pack_map(pk, 4) ||

        mp_pack_str(pk, "schema") ||
        msgpack_pack_uint8(pk, TI_FN_SCHEMA) ||

        mp_pack_str(pk, "event_id") ||
        msgpack_pack_uint64(pk, ti_.last_event_id) ||

        mp_pack_str(pk, "next_node_id") ||
        msgpack_pack_uint64(pk, ti_.nodes->next_id) ||

        mp_pack_str(pk, "nodes") ||
        ti_nodes_to_pk(pk)
    );
}

#endif /* TI_H_ */
