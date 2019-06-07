/*
 * ti/vset.h
 */
#ifndef TI_VSET_H_
#define TI_VSET_H_

typedef struct ti_vset_s ti_vset_t;

#include <inttypes.h>
#include <util/imap.h>

ti_vset_t * ti_vset_create(size_t sz);
void ti_vset_destroy(ti_vset_t * vset);

struct ti_vset_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _pad0;
    uint16_t _pad1;
    imap_t * imap;      /* key: thing_id / value: *ti_things_t */
};

#endif  /* TI_VSET_H_ */

