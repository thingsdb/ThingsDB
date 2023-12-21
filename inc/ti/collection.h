/*
 * ti/collection.h
 */
#ifndef TI_COLLECTION_H_
#define TI_COLLECTION_H_

#include <ex.h>
#include <stdint.h>
#include <ti/collection.t.h>
#include <ti/pkg.t.h>
#include <ti/raw.t.h>
#include <ti/stream.t.h>
#include <ti/val.t.h>
#include <util/guid.h>
#include <util/imap.h>

ti_collection_t * ti_collection_create(
        uint64_t collection_id,
        guid_t * guid,
        const char * name,
        size_t n,
        uint64_t created_at,
        ti_tz_t * tz,
        uint8_t deep);
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
void ti_collection_stop_futures(ti_collection_t * collection);
int ti_collection_gc(ti_collection_t * collection, _Bool do_mark_things);
ti_pkg_t * ti_collection_join_rooms(
        ti_collection_t * collection,
        ti_stream_t * stream,
        ti_pkg_t * pkg,
        ex_t * e);
ti_pkg_t * ti_collection_leave_rooms(
        ti_collection_t * collection,
        ti_stream_t * stream,
        ti_pkg_t * pkg,
        ex_t * e);

#endif /* TI_COLLECTION_H_ */
