/*
 * ti/store/storetypes.h
 */
#include <assert.h>
#include <ti.h>
#include <ti/closure.h>
#include <ti/condition.h>
#include <ti/field.h>
#include <ti/method.h>
#include <ti/prop.h>
#include <ti/raw.inline.h>
#include <ti/store/storetypes.h>
#include <ti/things.h>
#include <ti/type.h>
#include <ti/types.inline.h>
#include <ti/val.inline.h>
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
    if (msgpack_pack_array(pk, 8) ||
        msgpack_pack_uint16(pk, type->type_id) ||
        mp_pack_bool(pk, type->flags & TI_TYPE_FLAG_WRAP_ONLY) ||
        mp_pack_bool(pk, type->flags & TI_TYPE_FLAG_HIDE_ID) ||
        msgpack_pack_uint64(pk, type->created_at) ||
        msgpack_pack_uint64(pk, type->modified_at) ||
        mp_pack_strn(pk, type->rname->data, type->rname->n) ||
        msgpack_pack_map(pk, type->fields->n + !!type->idname)
    ) return -1;

    if (type->idname)
    {
        p = (uintptr_t) type->idname;
        if (msgpack_pack_uint64(pk, p) || mp_pack_strn(pk, "#", 1))
            return -1;
    }

    for (vec_each(type->fields, ti_field_t, field))
    {
        p = (uintptr_t) field->name;
        if (msgpack_pack_uint64(pk, p) ||
            (ti_raw_is_mpdata(field->spec_raw)
              ? mp_pack_bin(pk, field->spec_raw->data, field->spec_raw->n)
              : mp_pack_strn(pk, field->spec_raw->data, field->spec_raw->n)
            )
        ) return -1;
    }

    if (msgpack_pack_map(pk, type->methods->n))
        return -1;

    for (vec_each(type->methods, ti_method_t, method))
    {
        p = (uintptr_t) method->name;
        if (msgpack_pack_uint64(pk, p) ||
            ti_closure_to_store_pk(method->closure, pk)
        ) return -1;
    }

    return 0;
}

static int count_relations_cb(ti_type_t * type, size_t * n)
{
    for (vec_each(type->fields, ti_field_t, field))
    {
        if (ti_field_has_relation(field))
        {
            ti_field_t * other = field->condition.rel->field;
            if (   field == other ||
                    field->type->type_id < other->type->type_id ||
                    (   field->type->type_id == other->type->type_id &&
                        field->idx < other->idx))
                (*n)++;
        }
    }
    return 0;
}

static int rltype_cb(ti_type_t * type, msgpack_packer * pk)
{
    for (vec_each(type->fields, ti_field_t, field))
    {
        if (ti_field_has_relation(field))
        {
            ti_field_t * other = field->condition.rel->field;
            if (   field == other ||
                    field->type->type_id < other->type->type_id ||
                    (   field->type->type_id == other->type->type_id &&
                        field->idx < other->idx))
            {
                if (msgpack_pack_array(pk, 4) ||
                    msgpack_pack_uint16(pk, field->type->type_id) ||
                    mp_pack_strn(pk, field->name->str, field->name->n) ||
                    msgpack_pack_uint16(pk, other->type->type_id) ||
                    mp_pack_strn(pk, other->name->str, other->name->n))
                    return -1;
            }
        }
    }
    return 0;
}

int ti_store_types_store(ti_types_t * types, const char * fn)
{
    msgpack_packer pk;
    char namebuf[TI_NAME_MAX];
    FILE * f = fopen(fn, "w");
    size_t n = 0;

    if (!f)
    {
        log_errno_file("cannot open file", errno, fn);
        return -1;
    }

    /* count the number of relations */
    (void) imap_walk(types->imap, (imap_cb) count_relations_cb, &n);

    msgpack_packer_init(&pk, f, msgpack_fbuffer_write);

    if (msgpack_pack_map(&pk, 3) ||
        /* removed types */
        mp_pack_str(&pk, "removed") ||
        msgpack_pack_map(&pk, types->removed->n) ||
        smap_items(types->removed, namebuf, (smap_item_cb) rmtype_cb, &pk) ||
        /* active types */
        mp_pack_str(&pk, "types") ||
        msgpack_pack_array(&pk, types->imap->n) ||
        imap_walk(types->imap, (imap_cb) mktype_cb, &pk) ||
        /* relations */
        mp_pack_str(&pk, "relations") ||
        msgpack_pack_array(&pk, n) ||
        imap_walk(types->imap, (imap_cb) rltype_cb, &pk)
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
    (void) ti_sleep(5);
    return 0;
}

int ti_store_types_restore(ti_types_t * types, imap_t * names, const char * fn)
{
    const char * types_position;
    char namebuf[TI_NAME_MAX+1];
    int rc = -1;
    _Bool with_methods = true;
    _Bool with_wrap_only = true;
    _Bool with_hide_id = true;
    _Bool with_relations = true;
    fx_mmap_t fmap;
    ex_t e = {0};
    ti_name_t * name, * oname;
    ti_type_t * type, * otype;
    size_t i, ii;
    uint16_t type_id;
    uintptr_t utype_id;
    ti_raw_t * spec_raw;
    mp_obj_t obj, mp_id, mp_name, mp_wpo, mp_hid,
             mp_spec, mp_created, mp_modified;
    mp_unp_t up;
    ti_vup_t vup = {
            .isclient = false,
            .collection = types->collection,
            .up = &up,
    };

    fx_mmap_init(&fmap, fn);
    if (fx_mmap_open(&fmap))  /* fx_mmap_open() is a log function */
        goto fail0;

    mp_unp_init(&up, fmap.data, fmap.n);

    if (mp_next(&up, &obj) != MP_MAP || (obj.via.sz != 2 && obj.via.sz != 3) ||
        mp_skip(&up) != MP_STR)
        goto fail1;

    with_relations = obj.via.sz == 3;

    if (mp_next(&up, &obj) != MP_MAP)
        goto fail1;

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
        if (mp_next(&up, &obj) != MP_ARR)
            goto fail1;

        switch (obj.via.sz)
        {
        case 5:
            /*
             * TODO: (COMPAT) This code is for compatibility with ThingsDB
             *       versions before v0.9.6.
             */
            with_methods = false;
            with_wrap_only = false;
            with_hide_id = false;
            mp_wpo.tp = MP_BOOL;
            mp_wpo.via.bool_ = false;
            mp_hid.tp = MP_BOOL;
            mp_hid.via.bool_ = false;
            if (mp_next(&up, &mp_id) != MP_U64 ||
                mp_next(&up, &mp_created) != MP_U64 ||
                mp_next(&up, &mp_modified) != MP_U64 ||
                mp_next(&up, &mp_name) != MP_STR ||
                mp_skip(&up) != MP_MAP      /* fields */
            ) goto fail1;
            break;
        case 6:
            /*
             * TODO: (COMPAT) This code is for compatibility with ThingsDB
             *       versions before v0.9.7.
             */
            with_wrap_only = false;
            with_hide_id = false;
            mp_wpo.tp = MP_BOOL;
            mp_wpo.via.bool_ = false;
            mp_hid.tp = MP_BOOL;
            mp_hid.via.bool_ = false;
            if (mp_next(&up, &mp_id) != MP_U64 ||
                mp_next(&up, &mp_created) != MP_U64 ||
                mp_next(&up, &mp_modified) != MP_U64 ||
                mp_next(&up, &mp_name) != MP_STR ||
                mp_skip(&up) != MP_MAP ||   /* fields */
                mp_skip(&up) != MP_MAP      /* methods */
           ) goto fail1;
            break;
        case 7:
            /*
             * TODO: (COMPAT) This code is for compatibility with ThingsDB
             *       versions before v1.3.2.
             */
            with_hide_id = false;
            mp_hid.tp = MP_BOOL;
            mp_hid.via.bool_ = false;
            if (mp_next(&up, &mp_id) != MP_U64 ||
                mp_next(&up, &mp_wpo) != MP_BOOL ||
                mp_next(&up, &mp_created) != MP_U64 ||
                mp_next(&up, &mp_modified) != MP_U64 ||
                mp_next(&up, &mp_name) != MP_STR ||
                mp_skip(&up) != MP_MAP ||   /* fields */
                mp_skip(&up) != MP_MAP      /* methods */
           ) goto fail1;
            break;
        case 8:
            if (mp_next(&up, &mp_id) != MP_U64 ||
                mp_next(&up, &mp_wpo) != MP_BOOL ||
                mp_next(&up, &mp_hid) != MP_BOOL ||
                mp_next(&up, &mp_created) != MP_U64 ||
                mp_next(&up, &mp_modified) != MP_U64 ||
                mp_next(&up, &mp_name) != MP_STR ||
                mp_skip(&up) != MP_MAP ||   /* fields */
                mp_skip(&up) != MP_MAP      /* methods */
           ) goto fail1;
            break;
        default:
            goto fail1;
        }

        if (!ti_type_create(
                types,
                mp_id.via.u64,
                (
                    (mp_wpo.via.bool_ ? TI_TYPE_FLAG_WRAP_ONLY : 0) |
                    (mp_hid.via.bool_ ? TI_TYPE_FLAG_HIDE_ID : 0)
                ),
                mp_name.via.str.data,
                mp_name.via.str.n,
                mp_created.via.u64,
                mp_modified.via.u64))
        {
            log_critical("cannot create type `%.*s`",
                    mp_name.via.str.n,
                    mp_name.via.str.data);
            goto fail1;
        }
    }

    /* restore unpacker to types start */
    up.pt = types_position;

    for (i = types->imap->n; i--;)
    {
        /*
         * TODO: (COMPAT) This code is for compatibility with ThingsDB version
         *       before v0.9.6 and might be changed to obj.via.sz != 8 when
         *       backwards compatibility may be dropped.
         */
        if (mp_next(&up, &obj) != MP_ARR || obj.via.sz < 5 ||
            mp_next(&up, &mp_id) != MP_U64 ||
            (with_wrap_only && mp_skip(&up) != MP_BOOL) ||
            (with_hide_id && mp_skip(&up) != MP_BOOL) ||
            mp_skip(&up) != MP_U64 ||  /* created */
            mp_skip(&up) != MP_U64 ||  /* modified */
            mp_skip(&up) != MP_STR ||  /* name */
            mp_next(&up, &obj) != MP_MAP
            /* methods will be skipped later */
        ) goto fail1;

        type = ti_types_by_id(types, mp_id.via.u64);
        assert(type);

        for (ii = obj.via.sz; ii--;)
        {
            if (mp_next(&up, &mp_id) != MP_U64 ||
                (mp_next(&up, &mp_spec) != MP_STR && mp_spec.tp != MP_BIN)
            ) goto fail1;

            name = imap_get(names, mp_id.via.u64);
            if (!name)
                goto fail1;

            if (mp_spec.tp == MP_STR &&
                mp_spec.via.str.n == 1 &&
                mp_spec.via.str.data[0] == '#')
            {
                if (type->idname)
                    goto fail1;
                type->idname = name;
                ti_incref(name);
                continue;
            }

            spec_raw = mp_spec.tp == MP_STR
                ? ti_str_create(mp_spec.via.str.data, mp_spec.via.str.n)
                : ti_mp_create(mp_spec.via.bin.data, mp_spec.via.bin.n);
            if (!spec_raw)
                goto fail1;

            if (!ti_field_create(name, spec_raw, type, &e))
            {
                log_critical(e.msg);
                goto fail1;
            }

            ti_decref(spec_raw);
        }

        if (!with_methods)
            continue;  /* TODO: (COMPAT) only for compatibility, see above */

        if (mp_next(&up, &obj) != MP_MAP)
            goto fail1;

        for (ii = obj.via.sz; ii--;)
        {
            ti_val_t * val;

            if (mp_next(&up, &mp_id) != MP_U64)
                goto fail1;

            name = imap_get(names, mp_id.via.u64);
            if (!name)
                goto fail1;

            val = ti_val_from_vup(&vup);
            if (!val || !ti_val_is_closure(val))
                goto fail1;

            if (ti_type_add_method(type, name, (ti_closure_t *) val, &e))
            {
                log_critical(e.msg);
                goto fail1;
            }

            ti_decref(val);
        }
    }

    if (with_relations)
    {
        mp_obj_t mp_id1, mp_name1, mp_id2, mp_name2;
        ti_field_t * field, * ofield;

        if (mp_skip(&up) != MP_STR ||
            mp_next(&up, &obj) != MP_ARR
        ) goto fail1;

        for (i = obj.via.sz; i--;)
        {
            if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 4 ||
                mp_next(&up, &mp_id1) != MP_U64 ||
                mp_next(&up, &mp_name1) != MP_STR ||
                mp_next(&up, &mp_id2) != MP_U64 ||
                mp_next(&up, &mp_name2) != MP_STR
            ) goto fail1;

            type = ti_types_by_id(types, mp_id1.via.u64);
            otype = ti_types_by_id(types, mp_id2.via.u64);

            name = ti_names_weak_get_strn(
                    mp_name1.via.str.data,
                    mp_name1.via.str.n);
            oname = ti_names_weak_get_strn(
                    mp_name2.via.str.data,
                    mp_name2.via.str.n);

            if (!type || !otype || !name || !oname)
                goto fail1;

            field = ti_field_by_name(type, name);
            ofield = ti_field_by_name(otype, oname);

            if (!field || !ofield)
                goto fail1;

            if (ti_condition_field_rel_init(field, ofield, &e))
            {
                log_critical(e.msg);
                goto fail1;
            }

            if (ti_field_relation_make(field, ofield, NULL))
            {
                ti_panic("unrecoverable error");
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
        log_critical("failed to restore from file: `%s`", fn);

    return rc;
}
