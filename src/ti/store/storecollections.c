/*
 * ti/store/collections.c
 */
#include <stdlib.h>
#include <string.h>
#include <ti.h>
#include <ti/collection.h>
#include <ti/store/storecollections.h>
#include <util/fx.h>
#include <util/mpack.h>
#include <util/vec.h>

int ti_store_collections_store(const char * fn)
{
    msgpack_packer pk;
    vec_t * vec = ti()->collections->vec;
    FILE * f = fopen(fn, "w");
    if (!f)
    {
        log_errno_file("cannot open file", errno, fn);
        return -1;
    }

    msgpack_packer_init(&pk, f, msgpack_fbuffer_write);

    if (
        msgpack_pack_map(&pk, 1) ||
        mp_pack_str(&pk, "collections") ||
        msgpack_pack_array(&pk, vec->n)
    ) goto fail;

    for (vec_each(vec, ti_collection_t, collection))
    {
        if (
            msgpack_pack_array(&pk, 3) ||
            mp_pack_strn(&pk, collection->guid.guid, sizeof(guid_t)) ||
            mp_pack_strn(&pk, collection->name->data, collection->name->n) ||
            msgpack_pack_uint64(&pk, collection->created_at)
        ) goto fail;
    }

    log_debug("stored collections to file: `%s`", fn);
    goto done;

fail:
    log_error("failed to write file: `%s`", fn);
done:
    if (fclose(f))
    {
        log_errno_file("cannot close file", errno, fn);
        return -1;
    }
    return 0;
}

int ti_store_collections_restore(const char * fn)
{
    int rc = -1;
    size_t i;
    ssize_t n;
    mp_obj_t obj, mp_ver, mp_guid, mp_name, mp_created;
    mp_unp_t up;
    guid_t guid;
    ti_collection_t * collection;
    uchar * data = fx_read(fn, &n);
    if (!data)
        return -1;

    /* must drop existing collections if we want to restore */
    ti_collections_clear();

    mp_unp_init(&up, data, (size_t) n);

    if (
        mp_next(&up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_next(&up, &mp_ver) != MP_STR ||
        mp_next(&up, &obj) != MP_ARR
    ) goto fail;

    for (i = obj.via.sz; i--;)
    {
        if (
            mp_next(&up, &obj) != MP_ARR || obj.via.sz != 3 ||
            mp_next(&up, &mp_guid) != MP_STR ||
            mp_guid.via.str.n != sizeof(guid_t) ||
            mp_next(&up, &mp_name) != MP_STR ||
            mp_next(&up, &mp_created) != MP_U64
        ) goto fail;

        /* copy and check GUID, must be null terminated */
        memcpy(guid.guid, mp_guid.via.str.data, sizeof(guid_t));
        if (guid.guid[sizeof(guid_t) - 1])
            goto fail;

        collection = ti_collection_create(
                &guid,
                mp_name.via.str.data,
                mp_name.via.str.n,
                mp_created.via.u64);
        if (!collection || vec_push(&ti()->collections->vec, collection))
            goto fail;

    }
    goto done;


done:
    rc = 0;
fail:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);

    free(data);
    return rc;
}
