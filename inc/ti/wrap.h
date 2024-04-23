/*
 * ti/wrap.h
 */
#ifndef TI_WRAP_H_
#define TI_WRAP_H_

typedef struct ti_wrap_s  ti_wrap_t;

#include <inttypes.h>
#include <ti/thing.h>
#include <ti/vp.t.h>
#include <util/mpack.h>

ti_wrap_t * ti_wrap_create(ti_thing_t * thing, uint16_t type_id);
void ti_wrap_destroy(ti_wrap_t * wrap);
int ti__wrap_field_thing(
        ti_thing_t * thing,
        ti_vp_t * vp,
        uint16_t spec,
        int deep,
        int flags);
int ti_wrap_copy(ti_wrap_t ** wrap, uint8_t deep);
int ti_wrap_dup(ti_wrap_t ** wrap, uint8_t deep);

struct ti_wrap_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t unused_flags;
    uint16_t type_id;
    ti_thing_t * thing;     /* with reference */
};



#endif  /* TI_WRAP_H_ */
