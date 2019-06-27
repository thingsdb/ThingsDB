/*
 * ti/store/things.c
 */
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ti.h>
#include <ti/prop.h>
#include <ti/store/things.h>
#include <ti/things.h>
#include <unistd.h>
#include <util/fx.h>

int ti_store_things_store(imap_t * things, const char * fn)
{
    vec_t * things_vec;
    int rc = -1;
    FILE * f = fopen(fn, "w");
    if (!f)
    {
        log_error("cannot open file `%s` (%s)", fn, strerror(errno));
        return -1;
    }

    things_vec = imap_vec(things);
    if (!things_vec)
    {
        log_error(EX_ALLOC_S);
        goto stop;
    }

    for (vec_each(things_vec, ti_thing_t, thing))
    {
        if (fwrite(&thing->id, sizeof(uint64_t), 1, f) != 1)
        {
            log_error("error writing to file `%s`", fn);
            goto stop;
        }
    }

    rc = 0;

stop:
    if (fclose(f))
    {
        log_error("cannot close file `%s` (%s)", fn, strerror(errno));
        rc = -1;
    }

    if (!rc)
        log_debug("stored things (id's) to file: `%s`", fn);

    return rc;
}

int ti_store_things_store_data(imap_t * things, const char * fn)
{
    intptr_t p;
    _Bool found;
    int rc = -1;
    FILE * f = fopen(fn, "w");
    if (!f)
    {
        log_error("cannot open file `%s` (%s)", fn, strerror(errno));
        return -1;
    }

    vec_t * things_vec = imap_vec(things);
    if (!things_vec)
        goto stop;

    if (qp_fadd_type(f, QP_MAP_OPEN))
        goto stop;

    for (vec_each(things_vec, ti_thing_t, thing))
    {
        found = 0;
        for (vec_each(thing->props, ti_prop_t, prop))
        {
            if (!found && (
                    qp_fadd_int(f, thing->id) ||
                    qp_fadd_type(f, QP_MAP_OPEN)
            ))
                goto stop;

            found = 1;
            p = (intptr_t) prop->name;

            if (qp_fadd_int(f, p) || ti_val_to_file(prop->val, f))
                goto stop;
        }

        if (found && qp_fadd_type(f, QP_MAP_CLOSE))
            goto stop;
    }

    if (qp_fadd_type(f, QP_MAP_CLOSE))
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

int ti_store_things_restore(imap_t * things, const char * fn)
{
    int rc = 0;
    ssize_t sz;
    uchar * data = fx_read(fn, &sz);
    if (!data)
        goto failed;

    uchar * pt = data;
    uchar * end = data + sz - sizeof(uint64_t);

    for (;pt <= end; pt += sizeof(uint64_t))
    {
        uint64_t id;
        memcpy(&id, pt, sizeof(uint64_t));
        if (!ti_things_create_thing(things, id))
            goto failed;
    }

    goto done;

failed:
    rc = -1;
    log_critical("failed to restore from file: `%s`", fn);
done:
    free(data);
    return rc;
}


int ti_store_things_restore_data(
        imap_t * things,
        imap_t * names,
        const char * fn)
{
    int rc = -1;
    int pagesize = getpagesize();
    ti_thing_t * thing;
    ti_name_t * name;
    ti_val_t * val;
    qp_obj_t qp_thing_id, qp_name_id;
    struct stat st;
    ssize_t size;
    uchar * data;
    qp_unpacker_t unp;
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

    while (qp_is_int(qp_next(&unp, &qp_thing_id)))
    {
        uint64_t thing_id = (uint64_t) qp_thing_id.via.int64;
        thing = imap_get(things, thing_id);
        if (!thing)
        {
            log_critical("cannot find thing with id: %"PRIu64, thing_id);
            goto fail2;
        }

        if (!qp_is_map(qp_next(&unp, NULL)))
            goto fail2;

        while (qp_is_int(qp_next(&unp, &qp_name_id)))
        {
            uint64_t name_id = (uint64_t) qp_name_id.via.int64;

            name = imap_get(names, name_id);
            if (!name)
            {
                log_critical("cannot find name with id: %"PRIu64, name_id);
                goto fail2;
            }

            val = ti_val_from_unp(&unp, things);
            if (!val)
            {
                log_critical("cannot read value for `%s`", name->str);
                goto fail2;
            }

            if (ti_thing_prop_set(thing, name, val))
                goto fail2;

            ti_incref(name);
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
