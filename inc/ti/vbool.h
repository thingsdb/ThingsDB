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

ti_vbool_t * ti_vbool_get(_Bool b);
_Bool ti_vbool_no_ref(void);

static inline int ti_vbool_to_pk(ti_vbool_t * v, msgpack_packer * pk)
{
    return v->bool_ ? msgpack_pack_true(pk) : msgpack_pack_false(pk);
}

#endif  /* TI_VBOOL_H_ */
