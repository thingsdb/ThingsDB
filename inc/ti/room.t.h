/*
 * ti/room.t.h
 */
#ifndef TI_ROOM_T_H_
#define TI_ROOM_T_H_

#include <stdlib.h>
#include <inttypes.h>
#include <ti/collection.t.h>
#include <util/vec.h>

#define TI_ROOM_ENAME_MIN 1
#define TI_ROOM_ENAME_MAX 255

typedef struct ti_room_s ti_room_t;

struct ti_room_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _flags;
    uint16_t _pad1;
    int64_t id;
    ti_collection_t * collection;    /* NULL in non-collection scope */
    vec_t * listeners;
};

#endif  /* TI_ROOM_T_H_ */
