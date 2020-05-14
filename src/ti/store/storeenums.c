/*
 * ti/store/storeenums.h
 */
#include <assert.h>
#include <ti.h>
#include <ti/field.h>
#include <ti/prop.h>
#include <ti/raw.inline.h>
#include <ti/store/storetypes.h>
#include <ti/things.h>
#include <ti/enum.h>
#include <ti/enums.inline.h>
#include <util/fx.h>
#include <util/mpack.h>

static int mkenum_cb(ti_enum_t * enum_, msgpack_packer * pk)
{
    uintptr_t p;
    if (msgpack_pack_array(pk, 5) ||
        msgpack_pack_uint16(pk, enum_->enum_id) ||
        msgpack_pack_uint64(pk, enum_->created_at) ||
        msgpack_pack_uint64(pk, enum_->modified_at) ||
        mp_pack_strn(pk, enum_->rname->data, enum_->rname->n) ||
        msgpack_pack_map(pk, enum_->members->n)
    ) return -1;

    for (vec_each(enum_->members, ti_member_t, member))
    {
        p = (uintptr_t) member->name;
        if (msgpack_pack_uint64(pk, p) ||
            ti_val_to_pk(member->val, pk, TI_VAL_PACK_FILE)
        ) return -1;
    }

    return 0;
}

int ti_store_enums_store(ti_enums_t * enums, const char * fn)
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

    if (msgpack_pack_map(&pk, 1) ||
        /* active enums */
        mp_pack_str(&pk, "enums") ||
        msgpack_pack_array(&pk, enums->imap->n) ||
        imap_walk(enums->imap, (imap_cb) mkenum_cb, &pk)
    ) goto fail;

    log_debug("stored enumerators to file: `%s`", fn);
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

/*
 * Restore enums without values so they can be referenced to by types
 */
int ti_store_enums_restore(ti_enums_t * enums, const char * fn)
{
    int rc = -1;
    fx_mmap_t fmap;
    ex_t e = {0};
    size_t i;
    mp_obj_t obj, mp_id, mp_name, mp_created, mp_modified;
    mp_unp_t up;

    fx_mmap_init(&fmap, fn);
    if (fx_mmap_open(&fmap))  /* fx_mmap_open() is a log function */
        goto fail0;

    mp_unp_init(&up, fmap.data, fmap.n);

    if (mp_next(&up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_skip(&up) != MP_STR ||
        mp_next(&up, &obj) != MP_MAP
    ) goto fail1;

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 5 ||
            mp_next(&up, &mp_id) != MP_U64 ||
            mp_next(&up, &mp_created) != MP_U64 ||
            mp_next(&up, &mp_modified) != MP_U64 ||
            mp_next(&up, &mp_name) != MP_STR ||
            mp_skip(&up) != MP_MAP
        ) goto fail1;

        if (!ti_enum_create(
                enums,
                mp_id.via.u64,
                mp_name.via.str.data,
                mp_name.via.str.n,
                mp_created.via.u64,
                mp_modified.via.u64))
        {
            log_critical("cannot create enum `%.*s`",
                    (int) mp_name.via.str.n,
                    mp_name.via.str.data);
            goto fail1;
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

/*
 * Restore enum data without values so they can be referenced to by types
 */
int ti_store_enums_restore_data(ti_enums_t * enums, imap_t * names, const char * fn)
{
    const char * enums_position;
    char namebuf[TI_NAME_MAX+1];
    int rc = -1;
    fx_mmap_t fmap;
    ex_t e = {0};
    ti_name_t * name;
    ti_enum_t * enum_;
    size_t i;
    uint16_t enum_id;
    mp_obj_t obj, mp_id, mp_name, mp_created, mp_modified;
    mp_unp_t up;

    /* restore unpacker to enums start */
    up.pt = enums_position;

    for (i = enums->imap->n; i--;)
    {
        if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 5 ||
            mp_next(&up, &mp_id) != MP_U64 ||
            mp_skip(&up) != MP_U64 ||  /* created */
            mp_skip(&up) != MP_U64 ||  /* modified */
            mp_skip(&up) != MP_STR ||  /* name */
            mp_next(&up, &obj) != MP_MAP
        ) goto fail1;

        enum_ = ti_enums_by_id(enums, mp_id.via.u64);
        assert (enum_);

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

            if (!ti_member_create(name, spec, enum_, &e))
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
