/*
 * ti/collection.h
 */
#ifndef TI_COLLECTION_H_
#define TI_COLLECTION_H_

#include <ex.h>
#include <stdint.h>
#include <ti/collection.t.h>
#include <ti/raw.t.h>
#include <ti/val.t.h>
#include <util/guid.h>
#include <util/imap.h>

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

/* returns a borrowed reference */
static inline void * ti_collection_thing_by_id(
        ti_collection_t * collection,
        uint64_t thing_id)
{
    return imap_get(collection->things, thing_id);
}

#endif /* TI_COLLECTION_H_ */
