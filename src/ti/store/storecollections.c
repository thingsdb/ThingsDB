/*
 * ti/store/collections.c
 */
#include <util/qpx.h>
#include <ti/collection.h>
#include <ti.h>
#include <util/fx.h>
#include <util/vec.h>
#include <stdlib.h>
#include <string.h>
#include <ti/store/storecollections.h>
#include <util/mpack.h>

int ti_store_collections_store(const char * fn)
{
    msgpack_packer pk;
    vec_t * vec = ti()->collections->vec;
    FILE * f = fopen(fn, "w");
    if (!f)
    {
        log_error("cannot open file `%s` (%s)", fn, strerror(errno));
        return -1;
    }

    msgpack_packer_init(&pk, f, msgpack_fbuffer_write);

    if (
        msgpack_pack_map(&pk, 2) ||
        mp_pack_str(&pk, "collections") ||
        msgpack_pack_array(&pk, vec->n)
    ) goto fail;

    for (vec_each(vec, ti_collection_t, collection))
    {
        if (
            msgpack_pack_array(&pk, 3) ||
            mp_pack_strn(&pk, collection->guid.guid, sizeof(guid_t)) ||
            mp_pack_strn(&pk, collection->name->data, collection->name->n) ||
            msgpack_pack_array(&pk, 4) ||
            msgpack_pack_uint64(&pk, collection->quota->max_things) ||
            msgpack_pack_uint64(&pk, collection->quota->max_props) ||
            msgpack_pack_uint64(&pk, collection->quota->max_array_size) ||
            msgpack_pack_uint64(&pk, collection->quota->max_raw_size)
        ) goto fail;
    }

    log_debug("stored collections to file: `%s`", fn);
    goto done;

fail:
    log_error("failed to write file: `%s`", fn);
done:
    if (fclose(f))
    {
        log_error("cannot close file `%s` (%s)", fn, strerror(errno));
        return -1;
    }
    return 0;
}

int ti_store_collections_restore(const char * fn)
{
    int rc = -1;
    size_t i, m;
    ssize_t n;
    mp_obj_t obj, mp_guid, mp_name, mp_qthings, mp_qprops, mp_qarr, mp_qraw;
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
        mp_next(&up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(&up) != MP_STR ||
        mp_next(&up, &obj) != MP_ARR
    ) goto fail;

    LOGC("here");

    for (i = 0, m = obj.via.sz; i < m; ++i)
    {
        LOGC("read collection");
        if (
            mp_next(&up, &obj) != MP_ARR || obj.via.sz != 3 ||
            mp_next(&up, &mp_guid) != MP_STR
        ) goto fail;
        LOGC("1");
        if (
            mp_guid.via.str.n != sizeof(guid_t) ||
            mp_next(&up, &mp_name) != MP_STR
        ) goto fail;
        LOGC("2");
        if (
            mp_next(&up, &obj) != MP_ARR || obj.via.sz != 4 ||
            mp_next(&up, &mp_qthings) != MP_U64 ||
            mp_next(&up, &mp_qprops) != MP_U64 ||
            mp_next(&up, &mp_qarr) != MP_U64 ||
            mp_next(&up, &mp_qraw) != MP_U64
        ) goto fail;

        LOGC("data ok");

        /* copy and check guid, must be null terminated */
        memcpy(guid.guid, mp_guid.via.str.data, sizeof(guid_t));
        if (guid.guid[sizeof(guid_t) - 1])
            goto fail;

        collection = ti_collection_create(
                &guid,
                mp_name.via.str.data,
                mp_name.via.str.n);
        if (!collection || vec_push(&ti()->collections->vec, collection))
            goto fail;

        collection->quota->max_things = mp_qthings.via.u64;
        collection->quota->max_props = mp_qprops.via.u64;
        collection->quota->max_array_size = mp_qarr.via.u64;
        collection->quota->max_raw_size = mp_qraw.via.u64;
    }

    rc = 0;
fail:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);

    free(data);
    return rc;
}
