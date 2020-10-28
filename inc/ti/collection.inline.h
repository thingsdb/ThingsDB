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
        msgpack_pack_map(pk, 4) ||

        mp_pack_str(pk, "collection_id") ||
        msgpack_pack_uint64(pk, collection->root->id) ||

        mp_pack_str(pk, "name") ||
        mp_pack_strn(pk, collection->name->data, collection->name->n) ||

        mp_pack_str(pk, "created_at") ||
        msgpack_pack_uint64(pk, collection->created_at) ||

        mp_pack_str(pk, "things") ||
        msgpack_pack_uint64(pk, collection->things->n)
    );
}

/*
 * Return a thing with a borrowed reference from the collection, or,
 * if not found, tries to restore the thing from the garbage collector.
 */
static inline ti_thing_t * ti_collection_find_thing(
        ti_collection_t * collection,
        uint64_t thing_id)
{
    ti_thing_t * thing = imap_get(collection->things, thing_id);
    return thing
            ? thing
            : ti_collection_thing_restore_gc(collection, thing_id);
}

/*
 * Return a thing with a borrowed reference from the collection but
 * does not look in the garbage collector.
 */
static inline void * ti_collection_thing_by_id(
        ti_collection_t * collection,
        uint64_t thing_id)
{
    return imap_get(collection->things, thing_id);
}

#endif  /* TI_COLLECTION_INLINE_H_ */
