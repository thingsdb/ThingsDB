/*
 * ti/wano.inline.h
 */
#ifndef TI_WANO_INLINE_H_
#define TI_WANO_INLINE_H_

#include <ti/raw.t.h>
#include <ti/thing.h>
#include <ti/vp.t.h>
#include <ti/wano.t.h>
#include <ti/wrap.h>

static inline ti_raw_t * ti_wano_str(ti_wano_t * wano)
{
    return wano->thing->id
        ? ti_str_from_fmt(TI_VAL_WANO_S":%"PRIu64, wano->thing->id)
        : ti_str_from_str(TI_VAL_WANO_S":nil");
}

static inline int ti_wano_to_client_pk(
        ti_wano_t * wano,
        ti_vp_t * vp,
        int deep,
        int flags)
{
    return deep > 0
        ? ti_wrap_field_thing_type(wano->thing, vp, wano->ano->type, deep, flags)
        : (!wano->thing->id || (flags & TI_FLAGS_NO_IDS))
        ? ti_thing_empty_to_client_pk(&vp->pk)
        : ti_thing_id_to_client_pk(wano->thing, &vp->pk);
}

static inline int ti_wano_to_store_pk(ti_wano_t * wano, msgpack_packer * pk)
{
    return -(
            msgpack_pack_map(pk, 1) ||
            mp_pack_strn(pk, TI_KIND_S_WANO, 1) ||
            msgpack_pack_array(pk, 2) ||
            mp_pack_bin(pk, wano->ano->spec_raw->data, wano->ano->spec_raw->n) ||
            ti_thing_to_store_pk(wano->thing, pk)
    );
}

#endif  /* TI_WANO_INLINE_H_ */
