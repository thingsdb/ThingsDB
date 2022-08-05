/*
 * ti/wrap.inline.h
 */
#ifndef TI_WRAP_INLINE_H_
#define TI_WRAP_INLINE_H_

#include <ti/thing.inline.h>
#include <ti/vp.t.h>
#include <ti/wrap.h>
#include <util/mpack.h>

/*
 * The returned type may be NULL, because a `type_id` might no longer exist.
 * If, and only if a new type with an equal name is created, then the ID will
 * be re-used and the "new" type would return for this wrap.
 */
static inline ti_type_t * ti_wrap_maybe_type(ti_wrap_t * wrap)
{
    ti_type_t * type = imap_get(
            wrap->thing->collection->types->imap,
            wrap->type_id);
    return type;
}

static inline const char * ti_wrap_type_str(ti_wrap_t * wrap)
{
    ti_type_t * type = ti_wrap_maybe_type(wrap);
    return type ? type->wname : "<thing>";
}

/*
 * Returns the wrapped name as a a raw type with a new reference.
 */
static inline ti_raw_t * ti_wrap_type_strv(ti_wrap_t * wrap)
{
    ti_type_t * type = ti_wrap_maybe_type(wrap);
    if (type)
    {
        ti_incref(type->rwname);
        return type->rwname;
    }
    return (ti_raw_t *) ti_val_wrapped_thing_str();
}

static inline int ti_wrap_to_client_pk(
        ti_wrap_t * wrap,
        ti_vp_t * vp,
        int deep,
        int flags)
{
    return deep > 0
        ? ti__wrap_field_thing(wrap->thing, vp, wrap->type_id, deep, flags)
        : (!wrap->thing->id || (flags & TI_FLAGS_NO_IDS))
        ? ti_thing_empty_to_client_pk(&vp->pk)
        : ti_thing_id_to_client_pk(wrap->thing, &vp->pk);
}

static inline int ti_wrap_to_store_pk(ti_wrap_t * wrap, msgpack_packer * pk)
{
    return -(
            msgpack_pack_map(pk, 1) ||
            mp_pack_strn(pk, TI_KIND_S_WRAP, 1) ||
            msgpack_pack_array(pk, 2) ||
            msgpack_pack_uint16(pk, wrap->type_id) ||
            ti_thing_to_store_pk(wrap->thing, pk)
    );
}

static inline ti_raw_t * ti_wrap_str(ti_wrap_t * wrap)
{
    ti_type_t * type = ti_wrap_maybe_type(wrap);
    char * wname = type ? type->wname : "<thing>";
    return wrap->thing->id
        ? ti_str_from_fmt("%s:%"PRIu64, wname, wrap->thing->id)
        : ti_str_from_fmt("%s:nil", wname);
}

#endif  /* TI_WRAP_INLINE_H_ */
