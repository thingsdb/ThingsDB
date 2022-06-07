/*
 * ti/vbool.h
 */
#ifndef TI_VBOOL_H_
#define TI_VBOOL_H_

typedef struct ti_vbool_s ti_vbool_t;

#define VBOOL(__x)  ((ti_vbool_t *) (__x))->bool_

#include <inttypes.h>

struct ti_vbool_s
{
    uint32_t ref;
    uint8_t tp;
    _Bool bool_;
};

extern ti_vbool_t vbool__true;
extern ti_vbool_t vbool__false;

_Bool ti_vbool_no_ref(void);

/*
 * This function is always successful and the result does not have to be
 * checked.
 */
static inline ti_vbool_t * ti_vbool_get(_Bool b)
{
    ti_vbool_t * vbool = b ? &vbool__true : &vbool__false;
    ti_incref(vbool);
    return vbool;
}

static inline int ti_vbool_to_pk(ti_vbool_t * v, msgpack_packer * pk)
{
    return v->bool_ ? msgpack_pack_true(pk) : msgpack_pack_false(pk);
}

#endif  /* TI_VBOOL_H_ */
