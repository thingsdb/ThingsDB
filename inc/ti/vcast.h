/*
 * ti/vcast.h
 */
#ifndef TI_VCAST_H_
#define TI_VCAST_H_

#include <inttypes.h>
#include <ti/thing.h>

struct ti_vcast_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    uint16_t class;         /* to class */
    ti_thing_t * thing;
};

#endif  /* TI_VCAST_H_ */
