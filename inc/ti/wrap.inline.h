/*
 * ti/wrap.inline.h
 */
#ifndef TI_WRAP_INLINE_H_
#define TI_WRAP_INLINE_H_

#include <ti/wrap.h>
#include <ti/thing.inline.h>
#include <util/mpack.h>

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
        msgpack_packer * pk,
        int options)
{
    return  /* for a client */
            options > 0
            ? ti__wrap_field_thing(wrap->type_id, wrap->thing, pk, options)

            /* no nesting, just the id */
            : options == 0
            ? ti_thing_id_to_pk(wrap->thing, pk)

            /* when packing for an event or store to disk */
            : (
                    msgpack_pack_map(pk, 1) ||
                    mp_pack_strn(pk, TI_KIND_S_WRAP, 1) ||
                    msgpack_pack_array(pk, 2) ||
                    msgpack_pack_uint16(pk, wrap->type_id) ||
                    ti_thing_to_pk(wrap->thing, pk, options)
    );
}

#endif  /* TI_WRAP_INLINE_H_ */
