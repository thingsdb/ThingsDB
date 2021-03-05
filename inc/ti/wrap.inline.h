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

static inline const char * ti_wrap_str(ti_wrap_t * wrap)
{
    ti_type_t * type = ti_wrap_maybe_type(wrap);
    return type ? type->wname : "<thing>";
}

/*
 * Returns the wrapped name as a a raw type with a new reference.
 */
static inline ti_raw_t * ti_wrap_strv(ti_wrap_t * wrap)
{
    ti_type_t * type = ti_wrap_maybe_type(wrap);
    if (type)
    {
        ti_incref(type->rwname);
        return type->rwname;
    }
    return (ti_raw_t *) ti_val_wthing_str();
}

static inline int ti_wrap_to_pk(
        ti_wrap_t * wrap,
        ti_vp_t * vp,
        int options)
{
    return  /* for a client */
            options > 0
            ? ti__wrap_field_thing(wrap->thing, vp, wrap->type_id, options)

            /* no nesting, just the id */
            : options == 0
            ? ti_thing_id_to_pk(wrap->thing, &vp->pk)

            /* when packing for an event or store to disk */
            : (
                    msgpack_pack_map(&vp->pk, 1) ||
                    mp_pack_strn(&vp->pk, TI_KIND_S_WRAP, 1) ||
                    msgpack_pack_array(&vp->pk, 2) ||
                    msgpack_pack_uint16(&vp->pk, wrap->type_id) ||
                    ti_thing_to_pk(wrap->thing, vp, options)
    );
}

#endif  /* TI_WRAP_INLINE_H_ */
