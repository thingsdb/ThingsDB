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
ti_uuid_t * ti_uuid_from_bytes(const void * bytes);
ti_uuid_t * ti_uuid_from_str(const char * str, size_t n, ex_t * e);
void ti_uuid_to_raw(uuid_t uuid, char * raw);
void ti_uuid_to_str(uuid_t uuid, char * str);
ti_raw_t * ti_uuid_str(ti_uuid_t * uuid);

static inline int ti_uuid_to_store_pk_v(ti_uuid_t * uuid, msgpack_packer * pk)
{
    return mp_pack_ext(pk, MPACK_EXT_UUID, uuid->id, sizeof(uuid_t));
}

static inline int ti_uuid_to_store_pk(uuid_t uuid, msgpack_packer * pk)
{
    return mp_pack_ext(pk, MPACK_EXT_UUID, uuid, sizeof(uuid_t));
}

static inline int ti_uuid_to_client_pk(uuid_t uuid, msgpack_packer * pk)
{
    uuid_raw_t raw;
    ti_uuid_to_raw(uuid, raw);
    return mp_pack_strn(pk, raw, sizeof(raw));
}

#endif /* TI_UUID_H_ */
