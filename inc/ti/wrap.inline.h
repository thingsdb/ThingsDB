/*
 * ti/wrap.inline.h
 */
#ifndef TI_WRAP_INLINE_H_
#define TI_WRAP_INLINE_H_

#include <ti/wrap.h>
#include <ti/thing.inline.h>
#include <util/mpack.h>

static inline int ti_wrap_to_pk(ti_wrap_t * wrap, msgpack_packer * pk)
{
    return (
            msgpack_pack_map(pk, 1) ||
            mp_pack_strn(pk, TI_KIND_S_WRAP, 1) ||
            msgpack_pack_array(pk, 2) ||
            msgpack_pack_uint16(pk, wrap->type_id) ||
            msgpack_pack_map(pk, 1) ||
            mp_pack_strn(pk, TI_KIND_S_THING, 1) ||
            msgpack_pack_uint64(pk, wrap->thing->id)
    );
}

static inline ti_type_t * ti_wrap_maybe_type(ti_wrap_t * wrap)
{
    ti_type_t * type = imap_get(
            wrap->thing->collection->types->imap,
            wrap->type_id);
    return type;
}

static inline const char * ti_wrap_str(ti_wrap_t * wrap)
{
    ti_type_t * type = ti_wrap_maybe_type(wrap);
    return type ? type->wname : "<thing>";
}

static inline int ti_wrap_to_pk(
        ti_wrap_t * wrap,
        qp_packer_t ** pckr,
        int options)
{
    assert (options > 0);
    return ti__wrap_field_thing(wrap->type_id, wrap->thing, pckr, options);
}

#endif  /* TI_WRAP_INLINE_H_ */
