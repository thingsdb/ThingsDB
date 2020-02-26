/*
 * ti/store/types.h
 */
#include <assert.h>
#include <ti.h>
#include <ti/field.h>
#include <ti/prop.h>
#include <ti/raw.inline.h>
#include <ti/store/storetypes.h>
#include <ti/things.h>
#include <ti/type.h>
#include <ti/types.inline.h>
#include <util/fx.h>
#include <util/mpack.h>

static int rmtype_cb(
        const uchar * name,
        size_t n,
        void * data,
        msgpack_packer * pk)
{
    uintptr_t type_id = (uintptr_t) data;
    return mp_pack_strn(pk, name, n) || msgpack_pack_uint64(pk, type_id);
}

static int mktype_cb(ti_type_t * type, msgpack_packer * pk)
{
    uintptr_t p;
    if (msgpack_pack_array(pk, 5) ||
        msgpack_pack_uint16(pk, type->type_id) ||
        msgpack_pack_uint64(pk, type->created_at) ||
        msgpack_pack_uint64(pk, type->modified_at) ||
        mp_pack_strn(pk, type->rname->data, type->rname->n) ||
        msgpack_pack_map(pk, type->fields->n)
    ) return -1;

    for (vec_each(type->fields, ti_field_t, field))
    {
        p = (uintptr_t) field->name;
        if (msgpack_pack_uint64(pk, p) ||
            mp_pack_strn(pk, field->spec_raw->data, field->spec_raw->n)
        ) return -1;
    }

    return 0;
}

int ti_store_types_store(ti_types_t * types, const char * fn)
{
    msgpack_packer pk;
    char namebuf[TI_NAME_MAX];
    FILE * f = fopen(fn, "w");
    if (!f)
    {
        log_errno_file("cannot open file", errno, fn);
        return -1;
    }

    msgpack_packer_init(&pk, f, msgpack_fbuffer_write);

    if (msgpack_pack_map(&pk, 2) ||
        /* removed types */
        mp_pack_str(&pk, "removed") ||
        msgpack_pack_map(&pk, types->removed->n) ||
        smap_items(types->removed, namebuf, (smap_item_cb) rmtype_cb, &pk) ||
        /* active types */
        mp_pack_str(&pk, "types") ||
        msgpack_pack_array(&pk, types->imap->n) ||
        imap_walk(types->imap, (imap_cb) mktype_cb, &pk)
    ) goto fail;

    log_debug("stored types to file: `%s`", fn);
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

int ti_store_types_restore(ti_types_t * types, imap_t * names, const char * fn)
{
    const char * types_position;
    char namebuf[TI_NAME_MAX+1];
    int rc = -1;
    fx_mmap_t fmap;
    ex_t e = {0};
    ti_name_t * name;
    ti_type_t * type;
    size_t i, ii;
    uint16_t type_id;
    uintptr_t utype_id;
    ti_raw_t * spec;
    mp_obj_t obj, mp_id, mp_name, mp_spec, mp_created, mp_modified;
    mp_unp_t up;

    fx_mmap_init(&fmap, fn);
    if (fx_mmap_open(&fmap))  /* fx_mmap_open() is a log function */
        goto fail0;

    mp_unp_init(&up, fmap.data, fmap.n);

    if (mp_next(&up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(&up) != MP_STR ||
        mp_next(&up, &obj) != MP_MAP
    ) goto fail1;

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(&up, &mp_name) != MP_STR ||
            mp_next(&up, &mp_id) != MP_U64
        ) goto fail1;

        type_id = (uint16_t) mp_id.via.u64;
        utype_id = (uintptr_t) type_id;

        /*
         * Update next_id if required; remove the RM_FLAG to compare the real
         * type id.
         */
        type_id &= TI_TYPES_RM_MASK;
        if (type_id >= types->next_id)
            types->next_id = type_id + 1;

        memcpy(namebuf, mp_name.via.str.data, mp_name.via.str.n);
        namebuf[mp_name.via.str.n] = '\0';

        (void) smap_add(types->removed, namebuf, (void *) utype_id);
    }

    if (mp_skip(&up) != MP_STR ||     /* types key */
        mp_next(&up, &obj) != MP_ARR
    ) goto fail1;

    types_position = up.pt;

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 5 ||
            mp_next(&up, &mp_id) != MP_U64 ||
            mp_next(&up, &mp_created) != MP_U64 ||
            mp_next(&up, &mp_modified) != MP_U64 ||
            mp_next(&up, &mp_name) != MP_STR ||
            mp_skip(&up) != MP_MAP
        ) goto fail1;

        if (!ti_type_create(
                types,
                mp_id.via.u64,
                mp_name.via.str.data,
                mp_name.via.str.n,
                mp_created.via.u64,
                mp_modified.via.u64))
        {
            log_critical("cannot create type `%.*s`",
                    (int) mp_name.via.str.n,
                    mp_name.via.str.data);
            goto fail1;
        }
    }

    /* restore unpacker to types start */
    up.pt = types_position;

    for (i = types->imap->n; i--;)
    {
        if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 5 ||
            mp_next(&up, &mp_id) != MP_U64 ||
            mp_skip(&up) != MP_U64 ||  /* created */
            mp_skip(&up) != MP_U64 ||  /* modified */
            mp_skip(&up) != MP_STR ||  /* name */
            mp_next(&up, &obj) != MP_MAP
        ) goto fail1;

        type = ti_types_by_id(types, mp_id.via.u64);
        assert (type);

        for (ii = obj.via.sz; ii--;)
        {
            if (mp_next(&up, &mp_id) != MP_U64 ||
                mp_next(&up, &mp_spec) != MP_STR
            ) goto fail1;

            name = imap_get(names, mp_id.via.u64);
            if (!name)
                goto fail1;

            spec = ti_str_create(mp_spec.via.str.data, mp_spec.via.str.n);
            if (!spec)
                goto fail1;

            if (!ti_field_create(name, spec, type, &e))
            {
                log_critical(e.msg);
                goto fail1;
            }

            ti_decref(spec);
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
