/*
 * ti/collection.t.h
 */
#ifndef TI_COLLECTION_T_H_
#define TI_COLLECTION_T_H_

typedef struct ti_collection_s  ti_collection_t;

#include <uv.h>
#include <inttypes.h>
#include <ti/enums.t.h>
#include <ti/commit.h>
#include <ti/raw.t.h>
#include <ti/thing.t.h>
#include <ti/types.t.h>
#include <ti/tz.h>
#include <util/guid.h>
#include <util/imap.h>
#include <util/queue.h>

struct ti_collection_s
{
    uint32_t ref;
    long:24;
    uint8_t deep;
    uint64_t id;            /* collection Id (>= 2) */
    uint64_t next_free_id;
    uint64_t created_at;    /* UNIX time-stamp in seconds */
    ti_tz_t * tz;
    ti_raw_t * name;
    imap_t * things;        /* weak map for ti_thing_t */
    imap_t * rooms;         /* weak map for ti_room_t */
    queue_t * gc;           /* ti_gc_t */
    vec_t * access;         /* ti_auth_t */
    smap_t * procedures;    /* ti_procedure_t */
    smap_t * named_rooms;   /* weak map for ti_room_t (only named rooms) */
    ti_thing_t * root;      /* without extra reference */
    ti_types_t * types;
    ti_enums_t * enums;
    uv_mutex_t * lock;      /* only for watch/ unwatch/ away-mode */
    vec_t * futures;        /* no reference, type: ti_future_t */
    vec_t * vtasks;         /* tasks, type: ti_vtask_t */
    vec_t * commits;        /* migration changes ti_commit_t */
    guid_t guid;            /* derived from collection->id */
};

#endif /* TI_COLLECTION_T_H_ */
