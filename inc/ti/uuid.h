/*
 * ti/uuid.h
 */
#ifndef TI_UUID_H_
#define TI_UUID_H_

#include <ti/uuid.t.h>
#include <ti/val.t.h>
#include <ti/collection.t.h>
#include <util/mpack.h>
#include <ex.h>

ti_uuid_t * ti_uuid_new(void);
void ti_uuid_to_raw(ti_uuid_t * uuid, char * raw);
ti_raw_t * ti_uuid_str(ti_uuid_t * uuid);

static inline int ti_uuid_to_store_pk(ti_uuid_t * uuid, msgpack_packer * pk)
{
    return mp_pack_ext(
        pk,
        MPACK_EXT_UUID,
        uuid->id,
        sizeof(uuid->id));
}

static inline int ti_uuid_to_client_pk(
        ti_uuid_t * uuid,
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

#endif /* TI_UUID_H_ */
