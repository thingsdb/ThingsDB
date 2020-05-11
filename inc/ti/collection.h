/*
 * ti/collection.h
 */
#ifndef TI_COLLECTION_H_
#define TI_COLLECTION_H_

typedef struct ti_collection_s  ti_collection_t;
typedef struct ti_thing_s  ti_thing_t;

#include <uv.h>
#include <stdint.h>
#include <ti/raw.h>
#include <util/imap.h>
#include <util/guid.h>
#include <ex.h>
#include <ti/type.h>
#include <ti/types.h>
#include <ti/enums.h>
#include <ti/val.h>

ti_collection_t * ti_collection_create(
        guid_t * guid,
        const char * name,
        size_t n,
        uint64_t created_at);
void ti_collection_destroy(ti_collection_t * collection);
void ti_collection_drop(ti_collection_t * collection);
_Bool ti_collection_name_check(const char * name, size_t n, ex_t * e);
int ti_collection_rename(
        ti_collection_t * collection,
        ti_raw_t * rname,
        ex_t * e);
ti_val_t * ti_collection_as_mpval(ti_collection_t * collection);
size_t ti_collection_ntype(ti_collection_t * collection, ti_type_t * type);

struct ti_collection_s
{
    uint32_t ref;
    guid_t guid;            /* derived from collection->root->id */
    uint64_t created_at;    /* UNIX time-stamp in seconds */
    ti_raw_t * name;
    imap_t * things;        /* weak map for ti_thing_t */
    vec_t * access;         /* ti_auth_t */
    vec_t * procedures;     /* ti_procedure_t */
    ti_thing_t * root;
    ti_types_t * types;
    ti_enums_t * enums;
    uv_mutex_t * lock;      /* only for watch/ unwatch/ away-mode */
};

/* returns a borrowed reference */
static inline void * ti_collection_thing_by_id(
        ti_collection_t * collection,
        uint64_t thing_id)
{
    return imap_get(collection->things, thing_id);
}

#endif /* TI_COLLECTION_H_ */
