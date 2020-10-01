/*
 * ti/store/things.c
 */
#include <assert.h>
#include <ti.h>
#include <ti/types.inline.h>
#include <ti/thing.inline.h>
#include <ti/prop.h>
#include <ti/val.inline.h>
#include <ti/store/storethings.h>
#include <ti/things.h>
#include <util/fx.h>
#include <util/mpack.h>

static int store__things_cb(ti_thing_t * thing, msgpack_packer * pk)
{
    return (
            msgpack_pack_uint64(pk, thing->id) ||
            msgpack_pack_uint16(pk, thing->type_id)
    );
}

int ti_store_things_store(imap_t * things, const char * fn)
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
        mp_pack_str(&pk, "things") ||
        msgpack_pack_map(&pk, things->n)
    ) goto fail;

    if (imap_walk(things, (imap_cb) store__things_cb, &pk))
        goto fail;

    log_debug("stored thing id's and type to file: `%s`", fn);
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

static int store__walk_data(ti_thing_t * thing, msgpack_packer * pk)
{
    if (msgpack_pack_uint64(pk, thing->id))
        return -1;

    if (ti_thing_is_object(thing))
    {
        if (msgpack_pack_map(pk, thing->items->n))
            return -1;

        for (vec_each(thing->items, ti_prop_t, prop))
        {
            uintptr_t p = (uintptr_t) prop->name;
            if (msgpack_pack_uint64(pk, p) ||
                ti_val_to_pk(prop->val, pk, TI_VAL_PACK_FILE)
            ) return -1;
        }
    }
    else
    {
        if (msgpack_pack_array(pk, thing->items->n))
            return -1;

        for (vec_each(thing->items, ti_val_t, val))
            if (ti_val_to_pk(val, pk, TI_VAL_PACK_FILE))
                return -1;
    }
    return 0;
}

int ti_store_things_store_data(imap_t * things, const char * fn)
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
        mp_pack_str(&pk, "data") ||
        msgpack_pack_map(&pk, things->n)
    ) goto fail;

    if (imap_walk(things, (imap_cb) store__walk_data, &pk))
        goto fail;

    log_debug("stored things data to file: `%s`", fn);
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

int ti_store_things_restore(ti_collection_t * collection, const char * fn)
{
    int rc = -1;
    size_t i;
    uint16_t type_id;
    ssize_t n;
    mp_obj_t obj, mp_ver, mp_thing_id, mp_type_id;
    mp_unp_t up;
    ti_type_t * type;
    uchar * data = fx_read(fn, &n);
    if (!data)
        return -1;

    mp_unp_init(&up, data, (size_t) n);

    if (
        mp_next(&up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_next(&up, &mp_ver) != MP_STR ||
        mp_next(&up, &obj) != MP_MAP
    ) goto fail;

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(&up, &mp_thing_id) != MP_U64 ||
            mp_next(&up, &mp_type_id) != MP_U64
        ) goto fail;

        type_id = mp_type_id.via.u64;
        if (type_id == TI_SPEC_OBJECT)
        {
            if (!ti_things_create_thing_o(mp_thing_id.via.u64, 0, collection))
                goto fail;
            continue;
        }

        type = ti_types_by_id(collection->types, type_id);
        if (!type)
        {
            log_critical("cannot find type with id %u", type_id);
            goto fail;
        }
        if (!ti_things_create_thing_t(mp_thing_id.via.u64, type, collection))
            goto fail;
    }

    rc = 0;
fail:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);

    free(data);
    return rc;
}

int ti_store_things_restore_data(
        ti_collection_t * collection,
        imap_t * names,
        const char * fn)
{
    int rc = -1;
    ex_t e = {0};
    fx_mmap_t fmap;
    ti_thing_t * thing;
    ti_name_t * name;
    ti_type_t * type;
    ti_val_t * val;
    mp_obj_t obj, mp_ver, mp_thing_id, mp_name_id;
    mp_unp_t up;
    size_t i, ii;
    ti_vup_t vup = {
            .isclient = false,
            .collection = collection,
            .up = &up,
    };

    fx_mmap_init(&fmap, fn);
    if (fx_mmap_open(&fmap))  /* fx_mmap_open() is a log function */
        goto fail0;

    mp_unp_init(&up, fmap.data, fmap.n);

    if (
        mp_next(&up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_next(&up, &mp_ver) != MP_STR ||
        mp_next(&up, &obj) != MP_MAP
    ) goto fail1;

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(&up, &mp_thing_id) != MP_U64)
            goto fail1;

        thing = imap_get(collection->things, mp_thing_id.via.u64);
        if (!thing)
        {
            log_critical(
                    "cannot find thing with id: %"PRIu64,
                    mp_thing_id.via.u64);
            goto fail1;
        }
        if (ti_thing_is_object(thing))
        {
            if (mp_next(&up, &obj) != MP_MAP)
            {
                log_critical("expecting a `thing-object` to have a map");
                goto fail1;
            }

            for (ii = obj.via.sz; ii--;)
            {
                if (mp_next(&up, &mp_name_id) != MP_U64)
                    goto fail1;

                name = imap_get(names, mp_name_id.via.u64);
                if (!name)
                {
                    log_critical(
                            "cannot find name with id: %"PRIu64,
                            mp_name_id.via.u64);
                    goto fail1;
                }

                val = ti_val_from_vup(&vup);

                if (!val || ti_val_make_assignable(&val, thing, name, &e) ||
                    !ti_thing_o_prop_add(thing, name, val))
                    goto fail1;

                ti_incref(name);
            }
        }
        else
        {
            type = ti_thing_type(thing);

            if (mp_next(&up, &obj) != MP_ARR || type->fields->n != obj.via.sz)
            {
                log_critical(
                        "expecting a `thing-type` to have an array "
                        "with %"PRIu32" items", type->fields->n);
                goto fail1;
            }

            for (vec_each(type->fields, ti_field_t, field))
            {
                val = ti_val_from_vup(&vup);
                if (!val ||
                    ti_val_make_assignable(&val, thing, field->name, &e))
                    goto fail1;
                VEC_push(thing->items, val);
            }
        }
    }

    rc = 0;

fail1:
    if (fx_mmap_close(&fmap))
        rc = -1;
fail0:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);

    return rc;
}
