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
        ti_tz_t * tz,
        uint64_t created_at);
void ti_collection_destroy(ti_collection_t * collection);
void ti_collection_drop(ti_collection_t * collection);
_Bool ti_collection_name_check(const char * name, size_t n, ex_t * e);
int ti_collection_rename(
        ti_collection_t * collection,
        ti_raw_t * rname,
        ex_t * e);
ti_val_t * ti_collection_as_mpval(ti_collection_t * collection);
ti_thing_t * ti_collection_thing_restore_gc(
        ti_collection_t * collection,
        uint64_t thing_id);
void ti_collection_gc_clear(ti_collection_t * collection);
int ti_collection_gc(ti_collection_t * collection, _Bool do_mark_things);

#endif /* TI_COLLECTION_H_ */
