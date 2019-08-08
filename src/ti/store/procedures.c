/*
 * ti/store/procedures.c
 */
#include <assert.h>
#include <fcntl.h>
#include <qpack.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ti.h>
#include <ti/procedure.h>
#include <ti/procedures.h>
#include <ti/store/procedures.h>
#include <unistd.h>
#include <util/fx.h>
#include <util/qpx.h>

int ti_store_procedures_store(const vec_t * procedures, const char * fn)
{
    int rc = -1;
    qp_packer_t * packer = qp_packer_create2(1024, 2);
    if (!packer)
        return -1;

    if (qp_add_map(&packer))
        goto stop;

    for (vec_each(procedures, ti_procedure_t, procedure))
        if (qp_add_raw(packer, procedure->name->data, procedure->name->n) ||
            ti_closure_to_packer(procedure->closure, &packer))
            goto stop;

    if (qp_close_map(packer))
        goto stop;

    rc = fx_write(fn, packer->buffer, packer->len);

stop:
    if (rc)
        log_error("failed to write file: `%s`", fn);
    else
        log_debug("stored procedures to file: `%s`", fn);

    qp_packer_destroy(packer);
    return rc;
}

int ti_store_procedures_restore(
        vec_t ** procedures,
        const char * fn,
        imap_t * things)  /* things may be NULL */
{
    int rc = -1;
    int pagesize = getpagesize();
    qp_obj_t qp_name;
    struct stat st;
    ssize_t size;
    uchar * data;
    ti_raw_t * rname;
    ti_closure_t * closure;
    qp_unpacker_t unp;
    ti_procedure_t * procedure;
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

    while (qp_is_raw(qp_next(&unp, &qp_name)))
    {
        rname = ti_raw_create(qp_name.via.raw, qp_name.len);
        closure = (ti_closure_t *) ti_val_from_unp(&unp, things);
        procedure = NULL;

        if (!rname || !closure || !ti_val_is_closure((ti_val_t *) closure) ||
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
