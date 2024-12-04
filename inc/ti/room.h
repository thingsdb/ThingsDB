/*
 * ti/room.h
 */
#ifndef TI_ROOM_H_
#define TI_ROOM_H_

#include <inttypes.h>
#include <ti/collection.t.h>
#include <ti/pkg.t.h>
#include <ti/query.t.h>
#include <ti/raw.t.h>
#include <ti/room.t.h>
#include <ti/stream.t.h>
#include <util/vec.h>
#include <stdlib.h>

/* collection may be NULL */
ti_room_t * ti_room_create(uint64_t id, ti_collection_t * collection);

/* Remove name from room if set */
void ti_room_unset_name(ti_room_t * room);
/*
 * Removes previous name and attach the new name and increases the reference
 * count of name by one.
 */
int ti_room_set_name(ti_room_t * room, ti_name_t * name);

void ti_room_destroy(ti_room_t * room);
int ti_room_gen_id(ti_room_t * room);
int ti_room_join(ti_room_t * room, ti_stream_t * stream);
int ti_room_leave(ti_room_t * room, ti_stream_t * stream);
void ti_room_emit_data(ti_room_t * room, const void * data, size_t sz);
void ti_room_emit_node_status(ti_room_t * room, const char * status);
int ti_room_copy(ti_room_t ** room);
int ti_room_emit(
        ti_room_t * room,
        ti_query_t * query,
        vec_t * args,
        const char * event,
        size_t event_n,
        int deep,
        int flags);
int ti_room_emit_from_pkg(
        ti_collection_t * collection,
        ti_pkg_t * pkg,
        ex_t * e);

static inline int ti_room_emit_raw(
        ti_room_t * room,
        ti_query_t * query,
        ti_raw_t * event,
        vec_t * args,
        int deep,
        int flags)
{
    return ti_room_emit(
        room, query, args, (const char *) event->data, event->n, deep, flags);
}

#endif  /* TI_ROOM_H_ */
