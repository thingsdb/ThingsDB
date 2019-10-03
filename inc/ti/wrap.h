/*
 * ti/wrap.h
 */
#ifndef TI_WRAP_H_
#define TI_WRAP_H_

typedef struct ti_wrap_s  ti_wrap_t;

#include <ti/thing.h>


ti_wrap_t * ti_wrap_create(ti_thing_t * thing, uint16_t type_id);
void ti_wrap_destroy(ti_wrap_t * wrap);
int ti_wrap_to_packer(ti_wrap_t * wrap, qp_packer_t ** pckr, int options);

struct ti_wrap_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _flags;
    uint16_t type_id;
    ti_thing_t * thing;     /* with reference */
};

#endif  /* TI_WRAP_H_ */
