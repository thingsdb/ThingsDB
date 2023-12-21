/*
 * ti/store/collections.c
 */
#include <stdlib.h>
#include <string.h>
#include <ti.h>
#include <ti/collection.h>
#include <ti/tz.h>
#include <ti/raw.inline.h>
#include <ti/store/storecollections.h>
#include <util/fx.h>
#include <util/mpack.h>
#include <util/vec.h>

int ti_store_collections_store(const char * fn)
{
    msgpack_packer pk;
    vec_t * vec = ti.collections->vec;
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
            msgpack_pack_array(&pk, 6) ||
            mp_pack_strn(&pk, collection->guid.guid, sizeof(guid_t)) ||
            mp_pack_strn(&pk, collection->name->data, collection->name->n) ||
            msgpack_pack_uint64(&pk, collection->created_at) ||
            msgpack_pack_uint64(&pk, collection->id) ||
            msgpack_pack_uint64(&pk, collection->tz->index) ||
            msgpack_pack_uint8(&pk, collection->deep)
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
    (void) ti_sleep(5);
    return 0;
}

int ti_store_collections_restore(const char * fn)
{
    int rc = -1;
    size_t i;
    ssize_t n;
    mp_obj_t obj, mp_guid, mp_name, mp_created, mp_deep, mp_tz, mp_id;
    mp_unp_t up;
    guid_t guid;
    ti_collection_t * collection;
    ti_tz_t * tz;
    uchar * data = fx_read(fn, &n);
    if (!data)
        return -1;

    /* must drop existing collections if we want to restore */
    ti_collections_clear();

    mp_unp_init(&up, data, (size_t) n);

    if (
        mp_next(&up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_skip(&up) != MP_STR ||
        mp_next(&up, &obj) != MP_ARR
    ) goto fail;

    for (i = obj.via.sz; i--;)
    {
        if (
            mp_next(&up, &obj) != MP_ARR || obj.via.sz < 3 ||
            mp_next(&up, &mp_guid) != MP_STR ||
            mp_guid.via.str.n != sizeof(guid_t) ||
            mp_next(&up, &mp_name) != MP_STR ||
            mp_next(&up, &mp_created) != MP_U64
        ) goto fail;

        /* copy and check GUID, must be null terminated */
        memcpy(guid.guid, mp_guid.via.str.data, sizeof(guid_t));
        if (guid.guid[sizeof(guid_t) - 1])
            goto fail;

        switch(obj.via.sz)
        {
        case 6:
            if (mp_next(&up, &mp_id) != MP_U64 ||
                mp_next(&up, &mp_tz) != MP_U64 ||
                mp_next(&up, &mp_deep) != MP_U64 ||
                mp_deep.via.u64 > TI_MAX_DEEP)
                goto fail;
            tz = ti_tz_from_index(mp_tz.via.u64);
            break;
        case 5:
            /* TODO: (COMPAT) This check is for compatibility with ThingsDB
             *       versions before v1.5.0
             */
            if (mp_next(&up, &mp_tz) != MP_U64 ||
                mp_next(&up, &mp_deep) != MP_U64 ||
                mp_deep.via.u64 > TI_MAX_DEEP)
                goto fail;

            tz = ti_tz_from_index(mp_tz.via.u64);
            mp_id.via.u64 = 0;  /* in previous version of ThingsDB, the root id
                                 * was the collection id, therefore we will set
                                 * this to 0 here and overwrite when reading
                                 * the root Id in ti_store_collection_restore()
                                 */
            break;
        case 4:
            /* TODO: (COMPAT) This check is for compatibility with ThingsDB
             *       versions before v1.1.3
             */
            if (mp_next(&up, &mp_tz) != MP_U64)
                goto fail;

            tz = ti_tz_from_index(mp_tz.via.u64);
            mp_deep.via.u64 = 1;
            break;
        case 3:
            /* TODO: (COMPAT) This check is for compatibility with ThingsDB
             *       versions before v0.10.0
             */
            tz = ti_tz_utc();
            mp_deep.via.u64 = 1;
            break;
        default:
            goto fail;
        }

        collection = ti_collection_create(
                mp_id.via.u64,
                &guid,
                mp_name.via.str.data,
                mp_name.via.str.n,
                mp_created.via.u64,
                tz,
                mp_deep.via.u64);
        if (!collection || vec_push(&ti.collections->vec, collection))
            goto fail;  /* might leak a few bytes for the time zone */

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
