/*
 * collection.h
 */
#ifndef TI_COLLECTION_H_
#define TI_COLLECTION_H_

typedef struct ti_collection_s  ti_collection_t;

#include <uv.h>
#include <stdint.h>
#include <ti/thing.h>
#include <ti/raw.h>
#include <util/imap.h>
#include <util/guid.h>
#include <ti/ex.h>
#include <ti/quota.h>

ti_collection_t * ti_collection_create(guid_t * guid, const char * name, size_t n);
void ti_collection_drop(ti_collection_t * collection);
_Bool ti_collection_name_check(const char * name, size_t n, ex_t * e);
int ti_collection_store(ti_collection_t * collection, const char * fn);
int ti_collection_restore(ti_collection_t * collection, const char * fn);
static inline void * ti_collection_thing_by_id(ti_collection_t * collection, uint64_t thing_id);

struct ti_collection_s
{
    uint32_t ref;
    guid_t guid;            /* derived from collection->root->id */
    ti_raw_t * name;
    imap_t * things;        /* weak map for ti_thing_t */
    vec_t * access;
    ti_thing_t * root;
    ti_quota_t * quota;
    uv_mutex_t * lock;      /* only for watch/unwatch/away-mode */
};

/* returns a borrowed reference */
static inline void * ti_collection_thing_by_id(ti_collection_t * collection, uint64_t thing_id)
{
    return imap_get(collection->things, thing_id);
}
#endif /* TI_COLLECTION_H_ */
