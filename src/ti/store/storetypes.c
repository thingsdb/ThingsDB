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
#include <ti/prop.h>
#include <ti/store/storetypes.h>
#include <ti/things.h>
#include <ti/type.h>
#include <ti/types.inline.h>
#include <ti/field.h>
#include <unistd.h>
#include <util/fx.h>


int ti_store_types_store(ti_types_t * types, const char * fn)
{
    intptr_t p;
    int rc = -1;
    vec_t * vtypes;
    FILE * f = fopen(fn, "w");
    if (!f)
    {
        log_error("cannot open file `%s` (%s)", fn, strerror(errno));
        return -1;
    }

    vtypes = imap_vec(types->imap);
    if (!vtypes)
    {
        log_critical(EX_MEMORY_S);
        goto stop;
    }

    if (qp_fadd_type(f, QP_ARRAY_OPEN))
        goto stop;

    for (vec_each(vtypes, ti_type_t, type))
    {
        if (qp_fadd_type(f, QP_ARRAY_OPEN) ||
            qp_fadd_int(f, type->type_id) ||
            qp_fadd_raw(f, (const uchar *) type->name, type->name_n) ||
            qp_fadd_type(f, QP_MAP_OPEN))
            goto stop;

        for (vec_each(type->fields, ti_field_t, field))
        {
            p = (intptr_t) field->name;
            if (qp_fadd_int(f, p) ||
                qp_fadd_raw(f, field->spec_raw->data, field->spec_raw->n))
            goto stop;
        }

        if (qp_fadd_type(f, QP_MAP_CLOSE) ||
            qp_fadd_type(f, QP_ARRAY_CLOSE))
            goto stop;
    }

    if (qp_fadd_type(f, QP_ARRAY_CLOSE))
        goto stop;

    rc = 0;
stop:
    if (rc)
        log_error("save failed: %s", fn);

    if (fclose(f))
    {
        log_error("cannot close file `%s` (%s)", fn, strerror(errno));
        rc = -1;
    }

    if (!rc)
        log_debug("stored properties data to file: `%s`", fn);

    return rc;
}


int ti_store_types_restore(ti_types_t * types, imap_t * names, const char * fn)
{
    int rc = -1;
    int pagesize = getpagesize();
    ex_t e = {0};
    ti_name_t * name;
    ti_type_t * type;
    qp_obj_t qp_type_id, qp_type_name, qp_field_name_id, qp_field_spec;
    struct stat st;
    ssize_t size;
    uint16_t type_id;
    uchar * data;
    ssize_t mapsz;
    qp_unpacker_t unp;
    qp_types_t isarr;
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

    qp_unpacker_init(&unp, data, size);

    if (!qp_is_map(qp_next(&unp, NULL)))
        goto fail2;

    isarr = qp_next(&unp, NULL);

    while (qp_is_array(isarr))
    {
        if (!qp_is_int(qp_next(&unp, &qp_type_id)) ||
            !qp_is_raw(qp_next(&unp, &qp_type_name)))
            goto fail2;

        qp_skip(&unp); /* skip the map */


        type_id = (uint16_t) qp_type_id.via.int64;

        if (!ti_type_create(
                types,
                type_id,
                (const char *) qp_type_name.via.raw,
                qp_type_name.len))
        {
            log_critical("cannot create type `%.*s`",
                    (int) qp_type_name.len,
                    (const char *) qp_type_name.via.raw);
            goto fail2;
        }

        isarr = qp_next(&unp, NULL);
        if (qp_is_close(isarr))
            isarr = qp_next(&unp, NULL);
    }

    qp_unpacker_init(&unp, data, size);

    if (!qp_is_array(qp_next(&unp, NULL)))
        goto fail2;

    isarr = qp_next(&unp, NULL);

    while (qp_is_array(isarr))
    {
        if (!qp_is_int(qp_next(&unp, &qp_type_id)) ||
            !qp_is_raw(qp_next(&unp, NULL)))
            goto fail2;

        type_id = (uint16_t) qp_type_id.via.int64;
        type = ti_types_by_id(types, type_id);

        assert (type);

        mapsz = qp_next(&unp, NULL);

        if (!qp_is_map(mapsz))
            goto fail2;

        mapsz = mapsz == QP_MAP_OPEN ? -1 : mapsz - QP_MAP0;

        while (mapsz--)
        {
            uint64_t name_id;
            ti_raw_t * spec;
            if (qp_is_close(qp_next(&unp, &qp_field_name_id)))
                break;

            if (!qp_is_int(qp_field_name_id.tp) ||
                !qp_is_raw(qp_next(&unp, &qp_field_spec)))
                goto fail2;

            name_id = (uint64_t) qp_field_name_id.via.int64;

            name = imap_get(names, name_id);
            if (!name)
            {
                log_critical("cannot find name with id: %"PRIu64, name_id);
                goto fail2;
            }

            spec = ti_raw_create(qp_field_spec.via.raw, qp_field_spec.len);
            if (!spec)
                goto fail2;


            if (!ti_field_create(name, spec, type, &e))
            {
                log_critical(e.msg);
                goto fail2;
            }

            ti_decref(spec);
        }

        isarr = qp_next(&unp, NULL);
        if (qp_is_close(isarr))
            isarr = qp_next(&unp, NULL);
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
