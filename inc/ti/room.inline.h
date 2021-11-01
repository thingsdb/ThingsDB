/*
 * ti/room.inline.h
 */
#ifndef TI_ROOM_INLINE_H_
#define TI_ROOM_INLINE_H_

#include <stdlib.h>
#include <inttypes.h>
#include <ti/room.t.h>
#include <ti/raw.t.h>
#include <ti/raw.inline.h>
#include <util/imap.h>
#include <util/mpack.h>

/* returns IMAP_ERR_EXIST if the thing is already in the map */
static inline int ti_room_to_map(ti_room_t * room)
{
    return imap_add(room->collection->rooms, room->id, room);
}

static inline _Bool ti_room_has_listeners(ti_room_t * room)
{
    return room->listeners->n;
}

static inline ti_raw_t * ti_room_str(ti_room_t * room)
{
    return room->id
            ? ti_str_from_fmt("room:%"PRIu64, room->id)
            : ti_str_from_str("room:nil");
}

static inline int ti_room_to_pk(
        ti_room_t * room,
        msgpack_packer * pk,
        int options)
{
    unsigned char buf[8];
    mp_store_uint64(room->id, buf);
    return options >= 0
            ? room->id
            ? mp_pack_fmt(pk, "<room:%"PRIu64">", room->id)
            : mp_pack_str(pk, "<room:nil>")
            : mp_pack_ext(pk, MPACK_EXT_ROOM, buf, sizeof(buf));
}

#endif  /* TI_ROOM_INLINE_H_ */
