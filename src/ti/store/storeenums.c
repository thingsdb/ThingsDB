/*
 * ti/store/storeenums.h
 */
#include <assert.h>
#include <ti.h>
#include <ti/enum.h>
#include <ti/enums.inline.h>
#include <ti/enum.inline.h>
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
    if (msgpack_pack_array(pk, 6) ||
        msgpack_pack_uint16(pk, enum_->enum_id) ||
        msgpack_pack_uint64(pk, enum_->created_at) ||
        msgpack_pack_uint64(pk, enum_->modified_at) ||
        mp_pack_strn(pk, enum_->rname->data, enum_->rname->n) ||
        msgpack_pack_map(pk, enum_->methods->n)
    ) return -1;

    for (vec_each(enum_->methods, ti_method_t, method))
    {
        p = (uintptr_t) method->name;
        if (msgpack_pack_uint64(pk, p) ||
            ti_val_to_store_pk((ti_val_t *) method->closure, pk)
        ) return -1;
    }

    if (msgpack_pack_map(pk, enum_->members->n))
        return -1;

    for (vec_each(enum_->members, ti_member_t, member))
    {
        p = (uintptr_t) member->name;
        if (msgpack_pack_uint64(pk, p) ||
            ti_val_to_store_pk(member->val, pk)
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
    (void) ti_sleep(5);
    return 0;
}

/*
 * Restore enums without values so they can be referenced to by types
 */
int ti_store_enums_restore(ti_enums_t * enums, imap_t * names, const char * fn)
{
    int rc = -1;
    fx_mmap_t fmap;
    size_t i, ii;
    mp_obj_t obj, mp_id, mp_name, mp_created, mp_modified;
    mp_unp_t up;
    ex_t e = {0};

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
                enums->collection->name->n,
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

        if (mp_next(&up, &obj) != MP_ARR ||
                (obj.via.sz != 5 && obj.via.sz != 6) ||
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
                    mp_name.via.str.n,
                    mp_name.via.str.data);
            goto fail1;
        }

        if (ti_enums_add(enums, enum_))
        {
            ti_enum_destroy(enum_);
            goto fail1;
        }

        if (obj.via.sz == 6)
        {
            ti_name_t * name;

            if (mp_next(&up, &obj) != MP_MAP)
                goto fail1;

            for (ii = obj.via.sz; ii--;)
            {
                if (mp_next(&up, &mp_id) != MP_U64)
                    goto fail1;

                name = imap_get(names, mp_id.via.u64);
                if (!name ||
                    mp_skip(&up) <= 0 || /* skip value */
                    ti_member_alloc(enum_, name, &e))
                    goto fail1;
            }
        }
    }

    rc = 0;

fail1:
    if (fx_mmap_close(&fmap))
        rc = -1;
fail0:
    if (rc)
    {
        if (e.nr)
            log_critical("failed to restore enums from file: `%s` (%s)", fn, e.msg);
        else
            log_critical("failed to restore enums from file: `%s`", fn);
    }
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
    ti_member_t * member;
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
        if (mp_next(&up, &obj) != MP_ARR ||
                (obj.via.sz != 5 && obj.via.sz != 6) ||
            mp_next(&up, &mp_id) != MP_U64 ||
            mp_skip(&up) != MP_U64 ||  /* created */
            mp_skip(&up) != MP_U64 ||  /* modified */
            mp_skip(&up) != MP_STR
        ) goto fail1;

        enum_ = ti_enums_by_id(enums, mp_id.via.u64);
        assert(enum_);

        /* TODO (COMPAT); For comptibility with < 1.7.0 */
        if (obj.via.sz == 5)
        {
            if (mp_next(&up, &obj) != MP_MAP)
                goto fail1;

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

                if (ti_val_is_closure(val))
                    (void) ti_enum_add_method(
                        enum_, name, (ti_closure_t *) val, &e);
                else
                    (void) ti_member_create(enum_, name, val, &e);

                if (e.nr)
                {
                    ti_val_unsafe_drop(val);
                    goto fail1;
                }
                ti_decref(val);  /* we do not need the reference anymore */
            }
        }
        else
        {
            if (mp_next(&up, &obj) != MP_MAP)
                goto fail1;

            for (ii = obj.via.sz; ii--;)
            {
                if (mp_next(&up, &mp_id) != MP_U64)
                    goto fail1;

                name = imap_get(names, mp_id.via.u64);
                if (!name)
                    goto fail1;

                val = ti_val_from_vup(&vup);
                if (!val || !ti_val_is_closure(val))
                    goto fail1;

                if (ti_enum_add_method(enum_, name, (ti_closure_t *) val, &e))
                {
                    ti_val_unsafe_drop(val);
                    goto fail1;
                }

                ti_decref(val);  /* we do not need the reference anymore */
            }
            if (mp_next(&up, &obj) != MP_MAP || obj.via.sz == 0)
                goto fail1;

            for (ii = obj.via.sz; ii--;)
            {
                if (mp_next(&up, &mp_id) != MP_U64)
                    goto fail1;

                name = imap_get(names, mp_id.via.u64);
                if (!name)
                    goto fail1;

                member = ti_enum_member_by_raw(enum_, (ti_raw_t *) name);
                if (!member || member->val)
                    goto fail1;

                val = ti_val_from_vup(&vup);
                if (!val)
                    goto fail1;

                member->val = val;
            }
            if (ti_enum_set_enum_tp(enum_, val, &e))
                goto fail1;
        }

        /* perform a sanity check for the members */
        if (ti_enum_members_check(enum_, &e))
            goto fail1;
    }

    rc = 0;

fail1:
    if (fx_mmap_close(&fmap))
        rc = -1;
fail0:
    if (rc)
    {
        if (e.nr)
            log_critical("failed to restore members from file: `%s` (%s)", fn, e.msg);
        else
            log_critical("failed to restore members from file: `%s`", fn);
    }
    return rc;
}
