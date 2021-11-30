/*
 * ti/store/storethings.c
 */
#include <assert.h>
#include <ti.h>
#include <ti/collection.inline.h>
#include <ti/prop.h>
#include <ti/store/storethings.h>
#include <ti/thing.inline.h>
#include <ti/things.h>
#include <ti/types.inline.h>
#include <ti/val.inline.h>
#include <util/fx.h>
#include <util/mpack.h>

/*
 * No suffix was added for the first version of this file.
 * Thus "things" and "data" are considered to be the first versions (v0).
 */
const char * things_v0 = "things";
const char * things_v1 = "things_v1";

const char * data_v0 = "data";

static int store__things_cb(ti_thing_t * thing, msgpack_packer * pk)
{
    /*
     * Store both the `spec` and `type_id` in a single value. This way we can
     * use two values for both typed and non-typed things and still store info
     * about an optional spec.
     */
    uint32_t value;
    if (thing->type_id == TI_SPEC_OBJECT)
    {
        value = thing->via.spec;
        value <<= 16;
        value += thing->type_id;
    }
    else
        value = thing->type_id;

    return (
            msgpack_pack_uint64(pk, thing->id) ||
            msgpack_pack_uint32(pk, value)
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
        mp_pack_str(&pk, things_v1) ||
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

static int store__walk_i(ti_item_t * item, ti_vp_t * vp)
{
    uintptr_t p = (uintptr_t) item->key;

    return -((ti_raw_is_name(item->key)
        ? msgpack_pack_uint64(&vp->pk, p)
        : mp_pack_strn(&vp->pk, item->key->data, item->key->n)) ||
        ti_val_to_pk(item->val, vp, TI_VAL_PACK_FILE));
}

static int store__walk_data(ti_thing_t * thing, ti_vp_t * vp)
{
    if (msgpack_pack_uint64(&vp->pk, thing->id))
        return -1;

    if (ti_thing_is_object(thing))
    {
        if (msgpack_pack_map(&vp->pk, ti_thing_n(thing)))
            return -1;

        if (ti_thing_is_dict(thing))
            return smap_values(
                    thing->items.smap,
                    (smap_val_cb) store__walk_i,
                    vp);

        for (vec_each(thing->items.vec, ti_prop_t, prop))
        {
            uintptr_t p = (uintptr_t) prop->name;
            if (msgpack_pack_uint64(&vp->pk, p) ||
                ti_val_to_pk(prop->val, vp, TI_VAL_PACK_FILE)
            ) return -1;
        }
        return 0;
    }
    /* type */
    if (msgpack_pack_array(&vp->pk, ti_thing_n(thing)))
        return -1;

    for (vec_each(thing->items.vec, ti_val_t, val))
        if (ti_val_to_pk(val, vp, TI_VAL_PACK_FILE))
            return -1;
    return 0;
}

int ti_store_things_store_data(imap_t * things, const char * fn)
{
    ti_vp_t vp;
    FILE * f = fopen(fn, "w");
    if (!f)
    {
        log_errno_file("cannot open file", errno, fn);
        return -1;
    }

    msgpack_packer_init(&vp.pk, f, msgpack_fbuffer_write);

    if (
        msgpack_pack_map(&vp.pk, 1) ||
        mp_pack_str(&vp.pk, data_v0) ||
        msgpack_pack_map(&vp.pk, things->n)
    ) goto fail;

    if (imap_walk(things, (imap_cb) store__walk_data, &vp))
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
    uint16_t type_id, spec;
    ssize_t n;
    mp_obj_t obj, mp_ver, mp_thing_id, mp_type_id;
    mp_unp_t up;
    ti_type_t * type;
    uchar * data = fx_read(fn, &n);
    if (!data)
        return -1;
    int is_v0;

    mp_unp_init(&up, data, (size_t) n);

    if (
        mp_next(&up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_next(&up, &mp_ver) != MP_STR ||
        mp_next(&up, &obj) != MP_MAP
    ) goto fail;

    is_v0 = mp_str_eq(mp_ver, things_v0);

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(&up, &mp_thing_id) != MP_U64 ||
            mp_next(&up, &mp_type_id) != MP_U64
        ) goto fail;

        type_id = mp_type_id.via.u64;
        if (type_id == TI_SPEC_OBJECT)
        {
            /* TODO (COMPAT) For compatibility with versions before  v1.1.1 */
            spec = is_v0 ? TI_SPEC_ANY : (mp_type_id.via.u64 >> 16);

            if (!ti_things_create_thing_o(
                    mp_thing_id.via.u64,
                    spec,
                    0,
                    collection))
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
    ti_raw_t * key;
    ti_type_t * type;
    ti_val_t * val;
    mp_obj_t obj, mp_ver, mp_thing_id, mp_key;
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

        thing = ti_collection_thing_by_id(collection, mp_thing_id.via.u64);
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
                if (mp_next(&up, &mp_key) <= MP_END)
                    goto fail1;

                switch(mp_key.tp)
                {
                case MP_U64:
                    key = imap_get(names, mp_key.via.u64);
                    if (!key)
                    {
                        log_critical("failed to load key");
                        goto fail1;
                    }
                    ti_incref(key);
                    break;
                case MP_STR:
                    key = ti_str_create(mp_key.via.str.data, mp_key.via.str.n);
                    if (key)
                        break;
                    /* fall through */
                default:
                    goto fail1;
                }


                val = ti_val_from_vup(&vup);

                if (!val || ti_val_make_assignable(&val, thing, key, &e) ||
                    ti_thing_o_add(thing, key, val))
                    goto fail1;  /* may leak a few bytes for key */
            }
        }
        else
        {
            type = thing->via.type;

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
                    ti_val_make_assignable(&val, thing, field, &e))
                    goto fail1;
                VEC_push(thing->items.vec, val);
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
