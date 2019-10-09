/*
 * ti/store/procedures.c
 */
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ti.h>
#include <ti/procedure.h>
#include <ti/procedures.h>
#include <ti/raw.inline.h>
#include <ti/store/storeprocedures.h>
#include <unistd.h>
#include <util/fx.h>
#include <util/mpack.h>

int ti_store_procedures_store(const vec_t * procedures, const char * fn)
{
    msgpack_packer pk;
    FILE * f = fopen(fn, "w");
    if (!f)
    {
        log_error("cannot open file `%s` (%s)", fn, strerror(errno));
        return -1;
    }

    msgpack_packer_init(&pk, f, msgpack_fbuffer_write);

    if (msgpack_pack_map(&pk, procedures->n))
        goto fail;

    for (vec_each(procedures, ti_procedure_t, procedure))
        if (mp_pack_strn(&pk, procedure->name->data, procedure->name->n) ||
            ti_closure_to_pk(procedure->closure, &pk)
        ) goto fail;

    log_debug("stored procedures to file: `%s`", fn);
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

int ti_store_procedures_restore(
        vec_t ** procedures,
        const char * fn,
        ti_collection_t * collection)  /* collection may be NULL */
{
    int rc = -1;
    int pagesize = getpagesize();
    size_t i, m;
    mp_obj_t obj, mp_name;
    mp_unp_t up;
    struct stat st;
    ssize_t size;
    uchar * data;
    ti_raw_t * rname;
    ti_closure_t * closure;
    ti_procedure_t * procedure;
    ti_vup_t vup = {
            .isclient = false,
            .collection = collection,
            .up = &up,
    };

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

    mp_unp_init(&up, data, size);

    if (mp_next(&up, &obj) != MP_MAP)
        goto fail2;

    for (i = 0, m = obj.via.sz; i< m; ++i)
    {
        if (mp_next(&up, &mp_name) != MP_STR)
            goto fail2;

        rname = ti_str_create(mp_name.via.str.data, mp_name.via.str.n);
        closure = (ti_closure_t *) ti_val_from_unp(&vup);
        procedure = NULL;

        if (!rname ||
            !closure ||
            !ti_val_is_closure((ti_val_t *) closure) ||
            !(procedure = ti_procedure_create(rname, closure)) ||
            ti_procedures_add(procedures, procedure))
            goto fail3;

        ti_decref(rname);
        ti_decref(closure);

    }

    rc = 0;
    goto done;

fail3:
    ti_procedure_destroy(procedure);
    ti_val_drop((ti_val_t *) rname);
    ti_val_drop((ti_val_t *) closure);

done:
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
