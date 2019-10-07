/*
 * ti/store/types.h
 */
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ti.h>
#include <ti/field.h>
#include <ti/prop.h>
#include <ti/raw.inline.h>
#include <ti/store/storetypes.h>
#include <ti/things.h>
#include <ti/type.h>
#include <ti/types.inline.h>
#include <unistd.h>
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
    if (msgpack_pack_array(pk, 3) ||
        msgpack_pack_uint16(pk, type->type_id) ||
        mp_pack_strn(pk, type->name, type->name_n) ||
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
    char namebuf[TI_TYPE_NAME_MAX];
    FILE * f = fopen(fn, "w");
    if (!f)
    {
        log_error("cannot open file `%s` (%s)", fn, strerror(errno));
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
        log_error("cannot close file `%s` (%s)", fn, strerror(errno));
        return -1;
    }
    return 0;
}

int ti_store_types_restore(ti_types_t * types, imap_t * names, const char * fn)
{
    char * types_start;
    char namebuf[TI_TYPE_NAME_MAX+1];
    int rc = -1;
    int pagesize = getpagesize();
    ex_t e = {0};
    ti_name_t * name;
    ti_type_t * type;
    struct stat st;
    ssize_t size;
    size_t i, m, ii, mm;
    uint16_t type_id;
    uintptr_t utype_id;
    uchar * data;
    ti_raw_t * spec;
    mp_obj_t obj, mp_id, mp_name, mp_spec;
    mp_unp_t up;

    int fd = open(fn, O_RDONLY);
    if (fd < 0)
    {
        log_critical("cannot open file descriptor `%s` (%s)",
                fn, strerror(errno));
        goto fail0;
    }

    if (fstat(fd, &st) < 0)
    {
        log_critical("unable to get file statistics: `%s` (%s)",
                fn, strerror(errno));
        goto fail1;
    }

    size = st.st_size;
    size += pagesize - size % pagesize;
    data = (uchar *) mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED)
    {
        log_critical("unable to memory map file `%s` (%s)",
                fn, strerror(errno));
        goto fail1;
    }

    mp_unp_init(&up, data, (size_t) size);

    if (mp_next(&up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(&up) != MP_STR ||
        mp_next(&up, &obj) != MP_MAP
    ) goto fail2;

    for (i = 0, m = obj.via.sz; i < m; ++i)
    {
        if (mp_next(&up, &mp_name) != MP_STR ||
            mp_next(&up, &mp_id) != MP_U64
        ) goto fail2;

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
    ) goto fail2;

    types_start = up.pt;

    for (i = 0, m = obj.via.sz; i < m; ++i)
    {
        if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 3 ||
            mp_next(&up, &mp_id) != MP_U64 ||
            mp_next(&up, &mp_name) != MP_STR ||
            mp_skip(&up) != MP_MAP
        ) goto fail2;

        if (!ti_type_create(
                types,
                mp_id.via.u64,
                mp_name.via.str.data,
                mp_name.via.str.n))
        {
            log_critical("cannot create type `%.*s`",
                    (int) mp_name.via.str.n,
                    mp_name.via.str.data);
            goto fail2;
        }
    }

    /* restore unpacker to types start */
    up.pt = types_start;

    for (i = 0, m = types->imap->n; i < m; ++i)
    {
        if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 3 ||
            mp_next(&up, &mp_id) != MP_U64 ||
            mp_skip(&up) != MP_STR ||
            mp_next(&up, &obj) != MP_MAP
        ) goto fail2;

        type = ti_types_by_id(types, mp_id.via.u64);
        assert (type);

        for (ii = 0, mm = obj.via.sz; ii < mm; ++ii)
        {
            if (mp_next(&up, &mp_id) != MP_U64 ||
                mp_next(&up, &mp_spec) != MP_STR
            ) goto fail2;

            name = imap_get(names, mp_id.via.u64);
            if (!name)
                goto fail2;

            spec = ti_str_create(mp_spec.via.bin.data, mp_spec.via.str.n);
            if (!spec)
                goto fail2;

            if (!ti_field_create(name, spec, type, &e))
            {
                log_critical(e.msg);
                goto fail2;
            }

            ti_decref(spec);
        }
    }

    rc = 0;

fail2:
    if (munmap(data, size))
        log_error("memory unmap failed: `%s` (%s)", fn, strerror(errno));
fail1:
    if (close(fd))
        log_error("cannot close file descriptor `%s` (%s)",
                fn, strerror(errno));
fail0:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);

    return rc;

}
