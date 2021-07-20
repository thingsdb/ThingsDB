/*
 * ti/room.h
 */
#ifndef TI_ROOM_H_
#define TI_ROOM_H_

#include <collection.t.h>
#include <inttypes.h>
#include <room.t.h>
#include <stdlib.h>

ti_room_t * ti_room_create(uint64_t id, ti_collection_t * collection);
int ti_room_gen_id(ti_room_t * room);
int ti_room_gen_id(ti_room_t * room);

#endif  /* TI_ROOM_H_ */
