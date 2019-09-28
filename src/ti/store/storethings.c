/*
 * ti/store/things.c
 */
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ti.h>
#include <ti/typesi.h>
#include <ti/thingi.h>
#include <ti/prop.h>
#include <ti/store/storethings.h>
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
        log_error(EX_MEMORY_S);
        goto stop;
    }

    for (vec_each(things_vec, ti_thing_t, thing))
    {

        if (fwrite(&thing->id, sizeof(uint64_t), 1, f) != 1 ||
            fwrite(&thing->type_id, sizeof(uint16_t), 1, f) != 1)
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
        if (ti_thing_is_object(thing))
        {
            if (!thing->items->n)
                continue;

            if (qp_fadd_int(f, thing->id) || qp_fadd_type(f, QP_MAP_OPEN))
                goto stop;

            for (vec_each(thing->items, ti_prop_t, prop))
            {
                p = (intptr_t) prop->name;
                if (qp_fadd_int(f, p) || ti_val_to_file(prop->val, f))
                    goto stop;
            }

            if (qp_fadd_type(f, QP_MAP_CLOSE))
                goto stop;
        }
        else
        {
            if (qp_fadd_int(f, thing->id) ||
                qp_fadd_type(f, QP_ARRAY_OPEN))
                goto stop;

            for (vec_each(thing->items, ti_val_t, val))
                if (ti_val_to_file(val, f))
                    goto stop;


            if (qp_fadd_type(f, QP_ARRAY_CLOSE))
                goto stop;
        }
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

int ti_store_things_restore(ti_collection_t * collection, const char * fn)
{
    uint16_t type_id;
    uint64_t thing_id;
    ti_type_t * type;
    int rc = 0;
    ssize_t sz;
    uchar * pt;
    uchar * end;
    uchar * data = fx_read(fn, &sz);
    if (!data)
        goto failed;

    pt = data;
    end = data + sz - (sizeof(uint64_t) + sizeof(uint16_t));
    while (pt <= end)
    {
        memcpy(&thing_id, pt, sizeof(uint64_t));
        pt +=  sizeof(uint64_t);
        memcpy(&type_id, pt, sizeof(uint16_t));
        pt +=  sizeof(uint16_t);

        if (type_id == TI_SPEC_OBJECT)
        {
            if (!ti_things_create_thing_o(thing_id, collection))
                goto failed;
        }
        else
        {
            type = ti_types_by_id(collection->types, type_id);
            if (!type)
            {
                log_critical("cannot find type with id %u", type_id);
                goto failed;
            }
            if (!ti_things_create_thing_t(thing_id, type, collection))
                goto failed;
        }
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
        ti_collection_t * collection,
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
        thing = imap_get(collection->things, thing_id);
        if (!thing)
        {
            log_critical("cannot find thing with id: %"PRIu64, thing_id);
            goto fail2;
        }

        if (ti_thing_is_object(thing))
        {
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

                val = ti_val_from_unp(&unp, collection);
                if (!val)
                {
                    log_critical("cannot read value for `%s`", name->str);
                    goto fail2;
                }

                if (!ti_thing_o_prop_add(thing, name, val))
                    goto fail2;

                ti_incref(name);
            }
        }
        else
        {
            ti_type_t * type = ti_thing_type(thing);
            qp_types_t arrsz;

            if (!qp_is_array(((arrsz = qp_next(&unp, NULL)))))
                goto fail2;

            assert (thing->items->sz == type->fields->n);

            for (uint32_t i = 0, n = type->fields->n; i < n; ++i)
            {
                val = ti_val_from_unp(&unp, collection);
                if (!val)
                {
                    log_critical("cannot read value for type `%s`", type->name);
                    goto fail2;
                }

                VEC_push(thing->items, val);
            }

            if (arrsz == QP_ARRAY_OPEN && !qp_is_close( qp_next(&unp, NULL)))
                goto fail2;
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
