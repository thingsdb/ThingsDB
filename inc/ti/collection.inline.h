/*
 * ti/collection.inline.h
 */
#ifndef TI_COLLECTION_INLINE_H_
#define TI_COLLECTION_INLINE_H_

#include <util/mpack.h>
#include <ti/collection.h>
#include <ti/thing.h>

static inline int ti_collection_to_pk(
        ti_collection_t * collection,
        msgpack_packer * pk)
{
    return -(
        msgpack_pack_map(pk, 7) ||

        mp_pack_str(pk, "collection_id") ||
        msgpack_pack_uint64(pk, collection->root->id) ||

        mp_pack_str(pk, "name") ||
        mp_pack_strn(pk, collection->name->data, collection->name->n) ||

        mp_pack_str(pk, "things") ||
        msgpack_pack_uint64(pk, collection->things->n) ||

        mp_pack_str(pk, "quota_things") ||
        ti_quota_val_to_pk(pk, collection->quota->max_things) ||

        mp_pack_str(pk, "quota_properties") ||
        ti_quota_val_to_pk(pk, collection->quota->max_props) ||

        mp_pack_str(pk, "quota_array_size") ||
        ti_quota_val_to_pk(pk, collection->quota->max_array_size) ||

        mp_pack_str(pk, "quota_raw_size") ||
        ti_quota_val_to_pk(pk, collection->quota->max_raw_size)
    );
}

#endif  /* TI_COLLECTION_INLINE_H_ */
