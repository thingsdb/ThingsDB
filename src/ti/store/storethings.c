/*
 * ti/store/things.c
 */
#include <assert.h>
#include <ti.h>
#include <ti/types.inline.h>
#include <ti/thing.inline.h>
#include <ti/prop.h>
#include <ti/store/storethings.h>
#include <ti/things.h>
#include <util/fx.h>
#include <util/mpack.h>

int ti_store_things_store(imap_t * things, const char * fn)
{
    vec_t * things_vec;
    msgpack_packer pk;
    FILE * f = fopen(fn, "w");
    if (!f)
    {
        log_error("cannot open file `%s` (%s)", fn, strerror(errno));
        return -1;
    }

    msgpack_packer_init(&pk, f, msgpack_fbuffer_write);

    things_vec = imap_vec(things);
    if (!things_vec)
    {
        log_error(EX_MEMORY_S);
        goto fail;
    }

    if (msgpack_pack_map(&pk, things_vec->n))
        goto fail;

    for (vec_each(things_vec, ti_thing_t, thing))
    {
        if (msgpack_pack_uint64(&pk, thing->id) ||
            msgpack_pack_uint16(&pk, thing->type_id)
        ) goto fail;
    }

    log_debug("stored thing id's and type to file: `%s`", fn);
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

int ti_store_things_store_data(imap_t * things, const char * fn)
{
    vec_t * things_vec;
    uintptr_t p;
    msgpack_packer pk;
    FILE * f = fopen(fn, "w");
    if (!f)
    {
        log_error("cannot open file `%s` (%s)", fn, strerror(errno));
        return -1;
    }

    things_vec = imap_vec(things);
    if (!things_vec)
        goto fail;

    msgpack_packer_init(&pk, f, msgpack_fbuffer_write);

    if (msgpack_pack_map(&pk, things_vec->n))
        goto fail;

    for (vec_each(things_vec, ti_thing_t, thing))
    {
        if (msgpack_pack_uint64(&pk, thing->id))
            goto fail;

        if (ti_thing_is_object(thing))
        {
            if (msgpack_pack_map(&pk, thing->items->n))
                goto fail;

            for (vec_each(thing->items, ti_prop_t, prop))
            {
                p = (uintptr_t) prop->name;
                if (msgpack_pack_uint64(&pk, p) ||
                    ti_val_to_pk(prop->val, &pk, TI_VAL_PACK_FILE)
                ) goto fail;
            }
        }
        else
        {
            if (msgpack_pack_array(&pk, thing->items->n))
                goto fail;

            for (vec_each(thing->items, ti_val_t, val))
                if (ti_val_to_pk(val, &pk, TI_VAL_PACK_FILE))
                    goto fail;
        }
    }

    log_debug("stored things data to file: `%s`", fn);
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

int ti_store_things_restore(ti_collection_t * collection, const char * fn)
{
    int rc = -1;
    size_t i;
    uint16_t type_id;
    ssize_t n;
    mp_obj_t obj, mp_thing_id, mp_type_id;
    mp_unp_t up;
    ti_type_t * type;
    uchar * data = fx_read(fn, &n);
    if (!data)
        return -1;

    mp_unp_init(&up, data, (size_t) n);
    if (mp_next(&up, &obj) != MP_MAP)
        goto fail;

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
    fx_mmap_t fmap;
    ti_thing_t * thing;
    ti_name_t * name;
    ti_type_t * type;
    ti_val_t * val;
    mp_obj_t obj, mp_thing_id, mp_name_id;
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

    if (mp_next(&up, &obj) != MP_MAP)
        goto fail1;

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

                val = ti_val_from_unp(&vup);

                if (!val || !ti_thing_o_prop_add(thing, name, val))
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

            for (ii = obj.via.sz; ii--;)
            {
                val = ti_val_from_unp(&vup);
                if (!val)
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