/*
 * ti/store/storeprocedures.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/collection.inline.h>
#include <ti/raw.inline.h>
#include <ti/room.t.h>
#include <ti/room.h>
#include <ti/store/storeprocedures.h>
#include <ti/val.inline.h>
#include <util/fx.h>
#include <util/mpack.h>

static int named_room__store_cb(ti_room_t * room, msgpack_packer * pk)
{
    return -(
        msgpack_pack_array(pk, 2) ||
        msgpack_pack_uint64(pk, room->id) ||
        mp_pack_strn(pk, room->name->str, room->name->n)
    );
}

int ti_store_named_rooms_store(smap_t * named_rooms, const char * fn)
{
    msgpack_packer pk;
    FILE * f = fopen(fn, "w");
    if (!f)
    {
        log_errno_file("cannot open file", errno, fn);
        return -1;
    }

    msgpack_packer_init(&pk, f, msgpack_fbuffer_write);

    if (
        msgpack_pack_map(&pk, 1) ||
        mp_pack_str(&pk, "named_rooms") ||
        msgpack_pack_array(&pk, named_rooms->n)
    ) goto fail;

    if (smap_values(named_rooms, (smap_val_cb) named_room__store_cb, &pk))
        goto fail;

    log_debug("stored named_rooms to file: `%s`", fn);
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

int ti_store_named_rooms_restore(const char * fn, ti_collection_t * collection)
{
    int rc = -1;
    fx_mmap_t fmap;
    size_t i;
    mp_obj_t obj, mp_ver, mp_name, mp_id;
    mp_unp_t up;
    ti_name_t * name;
    ti_room_t * room;

    if (!fx_file_exist(fn))
        return 0;  /* until version v1.7.0 named rooms did not exist */

    fx_mmap_init(&fmap, fn);
    if (fx_mmap_open(&fmap))  /* fx_mmap_open() is a log function */
        goto fail0;

    mp_unp_init(&up, fmap.data, fmap.n);

    if (
        mp_next(&up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_next(&up, &mp_ver) != MP_STR ||
        mp_next(&up, &obj) != MP_ARR
    ) goto fail1;

    for (i = obj.via.sz; i--;)
    {
        if (
            mp_next(&up, &obj) != MP_ARR || obj.via.sz != 2 ||
            mp_next(&up, &mp_id) != MP_U64 ||
            mp_next(&up, &mp_name) != MP_STR
        ) goto fail1;

        name = ti_names_weak_get_strn(mp_name.via.str.data, mp_name.via.str.n);
        room = ti_collection_room_by_id(collection, mp_id.via.u64);
        if (!name || !room || ti_room_set_name(room, name))
            goto fail1;
    }

    rc = 0;
    goto done;

done:
fail1:
    if (fx_mmap_close(&fmap))
        rc = -1;
fail0:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);

    return rc;
}
