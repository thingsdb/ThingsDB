/*
 * ti/room.h
 */
#ifndef TI_ROOM_H_
#define TI_ROOM_H_

#include <inttypes.h>
#include <ti/collection.t.h>
#include <ti/query.t.h>
#include <ti/raw.t.h>
#include <ti/room.t.h>
#include <ti/stream.t.h>
#include <util/vec.h>
#include <stdlib.h>

ti_room_t * ti_room_create(uint64_t id, ti_collection_t * collection);
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
        ti_raw_t * event,
        vec_t * args,
        int deep);


#endif  /* TI_ROOM_H_ */
