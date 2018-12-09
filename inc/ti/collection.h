/*
 * ti/collection.h
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

ti_collection_t * ti_collection_create(
        guid_t * guid,
        const char * name,
        size_t n);
void ti_collection_drop(ti_collection_t * collection);
_Bool ti_collection_name_check(const char * name, size_t n, ex_t * e);
static inline int ti_collection_to_packer(
        ti_collection_t * collection,
        qp_packer_t ** packer);
ti_val_t * ti_collection_as_qpval(ti_collection_t * collection);
static inline void * ti_collection_thing_by_id(
        ti_collection_t * collection,
        uint64_t thing_id);


struct ti_collection_s
{
    uint32_t ref;
    guid_t guid;            /* derived from collection->root->id */
    ti_raw_t * name;
    imap_t * things;        /* weak map for ti_thing_t */
    vec_t * access;         /* ti_auth_t */
    ti_thing_t * root;
    ti_quota_t * quota;
    uv_mutex_t * lock;      /* only for watch/unwatch/away-mode */
};

/* returns a borrowed reference */
static inline void * ti_collection_thing_by_id(
        ti_collection_t * collection,
        uint64_t thing_id)
{
    return imap_get(collection->things, thing_id);
}

static inline int ti_collection_to_packer(
        ti_collection_t * collection,
        qp_packer_t ** packer)
{
    return (
        qp_add_map(packer) ||
        qp_add_raw_from_str(*packer, "id") ||
        qp_add_int64(*packer, collection->root->id) ||
        qp_add_raw_from_str(*packer, "name") ||
        qp_add_raw(*packer, collection->name->data, collection->name->n) ||
        qp_add_raw_from_str(*packer, "things") ||
        qp_add_int64(*packer, collection->things->n) ||
        qp_add_raw_from_str(*packer, "quota_max_things") ||
        (collection->quota->max_things == TI_QUOTA_NOT_SET
                ? qp_add_null(*packer)
                : qp_add_int64(*packer, collection->quota->max_things)) ||
        qp_add_raw_from_str(*packer, "quota_properties") ||
        (collection->quota->max_props == TI_QUOTA_NOT_SET
                ? qp_add_null(*packer)
                : qp_add_int64(*packer, collection->quota->max_props)) ||
        qp_add_raw_from_str(*packer, "quota_array_size") ||
        (collection->quota->max_array_size == TI_QUOTA_NOT_SET
                ? qp_add_null(*packer)
                : qp_add_int64(*packer, collection->quota->max_array_size)) ||
        qp_add_raw_from_str(*packer, "quota_raw_size") ||
        (collection->quota->max_raw_size == TI_QUOTA_NOT_SET
                ? qp_add_null(*packer)
                : qp_add_int64(*packer, collection->quota->max_raw_size)) ||
        qp_close_map(*packer)
    );
}

#endif /* TI_COLLECTION_H_ */
