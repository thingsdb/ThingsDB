/*
 * ti/store/storeenums.h
 */
#include <assert.h>
#include <ti.h>
#include <ti/enum.h>
#include <ti/enums.inline.h>
#include <ti/field.h>
#include <ti/member.h>
#include <ti/prop.h>
#include <ti/raw.inline.h>
#include <ti/store/storeenums.h>
#include <ti/things.h>
#include <ti/val.inline.h>
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
    size_t i;
    mp_obj_t obj, mp_id, mp_name, mp_created, mp_modified;
    mp_unp_t up;

    if (!fx_file_exist(fn))
    {
        /*
         * TODO: (COMPAT) This check may be removed when we no longer require
         *       backwards compatibility with previous versions of ThingsDB
         *       where the enum file did not exist.
         */
        log_warning(
                "no enumerations found for collection `%.*s`; "
                "file `%s` is missing",
                (int) enums->collection->name->n,
                (const char *) enums->collection->name->data,
                fn);
        return ti_store_enums_store(enums, fn);
    }

    fx_mmap_init(&fmap, fn);
    if (fx_mmap_open(&fmap))  /* fx_mmap_open() is a log function */
        goto fail0;

    mp_unp_init(&up, fmap.data, fmap.n);

    if (mp_next(&up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_skip(&up) != MP_STR ||
        mp_next(&up, &obj) != MP_ARR
    ) goto fail1;

    for (i = obj.via.sz; i--;)
    {
        ti_enum_t * enum_;

        if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 5 ||
            mp_next(&up, &mp_id) != MP_U64 ||
            mp_next(&up, &mp_created) != MP_U64 ||
            mp_next(&up, &mp_modified) != MP_U64 ||
            mp_next(&up, &mp_name) != MP_STR ||
            mp_skip(&up) != MP_MAP
        ) goto fail1;

        enum_ = ti_enum_create(
                mp_id.via.u64,
                mp_name.via.str.data,
                mp_name.via.str.n,
                mp_created.via.u64,
                mp_modified.via.u64);
        if (!enum_)
        {
            log_critical("cannot create enum `%.*s`",
                    (int) mp_name.via.str.n,
                    mp_name.via.str.data);
            goto fail1;
        }

        if (ti_enums_add(enums, enum_))
        {
            ti_enum_destroy(enum_);
            goto fail1;
        }
    }

    rc = 0;

fail1:
    if (fx_mmap_close(&fmap))
        rc = -1;
fail0:
    if (rc)
        log_critical("failed to restore enums from file: `%s`", fn);

    return rc;
}

/*
 * Restore enum data without values so they can be referenced to by types
 */
int ti_store_enums_restore_members(
        ti_enums_t * enums,
        imap_t * names,
        const char * fn)
{
    int rc = -1;
    fx_mmap_t fmap;
    ex_t e = {0};
    ti_name_t * name;
    ti_enum_t * enum_;
    ti_val_t * val;
    size_t i, ii;
    mp_obj_t obj, mp_id;
    mp_unp_t up;
    ti_vup_t vup = {
            .isclient = false,
            .collection = enums->collection,
            .up = &up,
    };

    if (!enums->imap->n)
        return 0;  /* no enum to unpack */

    fx_mmap_init(&fmap, fn);
    if (fx_mmap_open(&fmap))  /* fx_mmap_open() is a log function */
        goto fail0;

    mp_unp_init(&up, fmap.data, fmap.n);

    if (mp_next(&up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_skip(&up) != MP_STR ||
        mp_next(&up, &obj) != MP_ARR
    ) goto fail1;

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

        if (ti_enum_prealloc(enum_, obj.via.sz, &e))
        {
            log_critical(e.msg);
            goto fail1;
        }

        for (ii = obj.via.sz; ii--;)
        {
            if (mp_next(&up, &mp_id) != MP_U64)
                goto fail1;

            name = imap_get(names, mp_id.via.u64);
            if (!name)
                goto fail1;

            val = ti_val_from_vup(&vup);
            if (!val)
                goto fail1;

            if (!ti_member_create(enum_, name, val, &e))
            {
                ti_val_unsafe_drop(val);
                log_critical(e.msg);
                goto fail1;
            }

            ti_decref(val);  /* we do not need the reference anymore */
        }
    }

    rc = 0;

fail1:
    if (fx_mmap_close(&fmap))
        rc = -1;
fail0:
    if (rc)
        log_critical("failed to restore members from file: `%s`", fn);

    return rc;
}
