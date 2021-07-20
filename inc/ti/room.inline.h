/*
 * ti/room.inline.h
 */
#ifndef TI_ROOM_INLINE_H_
#define TI_ROOM_INLINE_H_

#include <stdlib.h>
#include <inttypes.h>
#include <room.t.h>
#include <util/imap.h>

/* returns IMAP_ERR_EXIST if the thing is already in the map */
static inline int ti_room_to_map(ti_room_t * room)
{
    return imap_add(room->collection->rooms, room->id, room);
}

static inline _Bool ti_room_has_listeners(ti_room_t * room)
{
    return room->listeners->n;
}

#endif  /* TI_ROOM_INLINE_H_ */
